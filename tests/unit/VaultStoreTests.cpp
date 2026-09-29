#include <windows.h>
#include <aclapi.h>
#include <sddl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <atomic>
#include "../../src/Vault/VaultStore.h"
namespace fs = std::filesystem;
using namespace aedae;

#ifdef AEDAE_VAULT_TEST_HOOKS
namespace aedae::vault_test
{
VaultStatus ValidateDescriptor(PSID owner, PACL acl);
std::string stopStage;
std::wstring readyEvent;
void (*onStage)(const char*){};
void Checkpoint(const char* stage)
{
    if (onStage) onStage(stage);
    if (stopStage != stage) return;
    HANDLE event = OpenEventW(EVENT_MODIFY_STATE, FALSE, readyEvent.c_str());
    if (!event || !SetEvent(event)) ExitProcess(80);
    CloseHandle(event);
    // Parent terminates this process after inspecting the actual commit boundary.
    Sleep(30000);
    ExitProcess(81);
}
}
#endif
void Check(bool ok, const char* name)
{
    if (!ok) throw std::runtime_error(name);
    std::cout << "PASS " << name << std::endl;
}
CredentialRecord Record(unsigned n)
{
    CredentialRecord r;
    r.id = L"synthetic-" + std::to_wstring(n);
    r.rp_id = L"example.test";
    r.account_label = L"fabricated";
    r.credential_id = {static_cast<BYTE>(n), static_cast<BYTE>(n >> 8)};
    r.protected_private_key = {7, 8, 9};
    r.created_at = r.last_used_at = 1;
    return r;
}
std::vector<char> Bytes(const fs::path& p)
{
    std::ifstream in(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
void Put(const fs::path& p, const std::vector<char>& b)
{
    std::ofstream out(p, std::ios::binary | std::ios::trunc);
    out.write(b.data(), static_cast<std::streamsize>(b.size()));
    if (!out) throw std::runtime_error("fixture write");
}
void SetVersion(std::vector<char>& b, unsigned n) { std::memcpy(b.data() + 8, &n, 4); }
#ifdef AEDAE_VAULT_TEST_HOOKS
fs::path collisionPath;
void InsertCollision(const char* stage)
{
    if (std::string(stage) == "before-temp-create") Put(collisionPath, {'n', 'o', 't', '-', 'o', 'u', 'r', 's'});
}
void CollisionCase(const fs::path& root)
{
    const auto path = root / L"collision.bin";
    VaultStore writer(path.wstring(), VaultAccess::writer);
    Check(static_cast<bool>(writer.Provision()), "collision fixture");
    const auto original = Bytes(path);
    collisionPath = path.wstring() + L".tmp";
    vault_test::onStage = InsertCollision;
    const auto status = writer.AddCredential(Record(9));
    vault_test::onStage = nullptr;
    Check(!status && Bytes(path) == original &&
        Bytes(collisionPath) == std::vector<char>({'n', 'o', 't', '-', 'o', 'u', 'r', 's'}),
        "failed exclusive create does not delete another creator's file");
    Check(static_cast<bool>(writer.Recover()), "explicit recovery can discard collision fixture");
}
#endif

#ifdef AEDAE_VAULT_TEST_HOOKS
fs::path swapPath;
int swapMode{};
HANDLE swapHeld{INVALID_HANDLE_VALUE};
bool swapSucceeded{};
void SubstituteTemp(const char* stage)
{
    if (std::string(stage) != "before-rename") return;
    const auto temp = swapPath.wstring() + L".tmp";
    swapSucceeded = MoveFileExW(temp.c_str(), (swapPath.wstring() + L".captured").c_str(), 0) != FALSE;
    if (!swapSucceeded) return;
    if (swapMode == 1)
    {
        Check(CreateSymbolicLinkW(temp.c_str(), (swapPath.wstring() + L".target").c_str(),
            SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE) != FALSE, "substitution symlink fixture");
    }
    else Put(temp, {'s', 'u', 'b', 's', 't', 'i', 't', 'u', 't', 'e'});
    if (swapMode == 2)
    {
        swapHeld = CreateFileW(swapPath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        Check(swapHeld != INVALID_HANDLE_VALUE, "substitution cleanup failure fixture");
    }
}
void ReproduceSubstitution(const fs::path& root, bool requireBlocked = false)
{
    for (int mode = 0; mode < 3; ++mode)
    {
        swapPath = root / (L"substitution-" + std::to_wstring(mode) + L".bin");
        VaultStore writer(swapPath.wstring(), VaultAccess::writer);
        Check(static_cast<bool>(writer.Provision()), "substitution provision baseline");
        Check(static_cast<bool>(writer.AddCredential(Record(4))), "substitution legitimate baseline");
        const auto original = Bytes(swapPath);
        Put(swapPath.wstring() + L".target", original);
        swapMode = mode;
        swapSucceeded = false;
        vault_test::onStage = SubstituteTemp;
        const auto status = writer.AddCredential(Record(5));
        vault_test::onStage = nullptr;
        if (swapHeld != INVALID_HANDLE_VALUE) { CloseHandle(swapHeld); swapHeld = INVALID_HANDLE_VALUE; }
        if (requireBlocked)
        {
            Check(!swapSucceeded, "SECURITY REGRESSION: temp substitution must be prevented");
            continue;
        }
        Check(swapSucceeded, "source temp can be replaced at actual boundary");
        if (mode == 0)
            Check(static_cast<bool>(status) && Bytes(swapPath) == std::vector<char>({'s','u','b','s','t','i','t','u','t','e'}),
                "REPRO commit reports success publishing substituted bytes");
        else if (mode == 1)
            Check(static_cast<bool>(status) && (GetFileAttributesW(swapPath.c_str()) & FILE_ATTRIBUTE_REPARSE_POINT) &&
                writer.ListCredentials().error == VaultError::acl_policy, "REPRO commit publishes substituted symlink");
        else
            Check(!status && Bytes(swapPath) == original && !fs::exists(swapPath.wstring() + L".tmp"),
                "REPRO failed commit deletes substituted temp");
    }
}
bool ExternalSwap(const fs::path& path)
{
    wchar_t exe[32768]{};
    if (!GetModuleFileNameW(nullptr, exe, 32768)) throw std::runtime_error("swap helper executable");
    std::wstring cmd = L"\"" + std::wstring(exe) + L"\" --swap \"" +
        path.wstring() + L".tmp\" \"" + path.wstring() + L".captured\"";
    STARTUPINFOW si{}; si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    Check(CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
        nullptr, nullptr, &si, &pi) != FALSE, "independent substitution process launched");
    CloseHandle(pi.hThread);
    Check(WaitForSingleObject(pi.hProcess, 10000) == WAIT_OBJECT_0, "independent substitution process returned");
    DWORD code{};
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    return code == 0;
}
void AttemptExternalSubstitution(const char* stage)
{
    if (std::string(stage) == "before-rename") swapSucceeded = ExternalSwap(swapPath);
}
void CommitCallerSubstitutionCases(const fs::path& root)
{
    for (unsigned action = 0; action < 5; ++action)
    {
        swapPath = root / (L"external-substitution-" + std::to_wstring(action)) / L"vault.bin";
        VaultStore writer(swapPath.wstring(), VaultAccess::writer);
        if (action != 0)
        {
            Check(static_cast<bool>(writer.Provision()) && static_cast<bool>(writer.AddCredential(Record(40 + action))),
                "commit caller baseline");
            if (action == 1)
            {
                auto legacy = Bytes(swapPath);
                legacy.erase(legacy.begin() + 16, legacy.begin() + 20);
                SetVersion(legacy, 1);
                Put(swapPath, legacy);
            }
        }
        swapSucceeded = false;
        vault_test::onStage = AttemptExternalSubstitution;
        VaultStatus status;
        switch (action)
        {
        case 0: status = writer.Provision(); break;
        case 1: status = writer.Migrate(); break;
        case 2: status = writer.AddCredential(Record(90)); break;
        case 3: status = writer.UpdateLastUsed(Record(43).credential_id, 123); break;
        default: status = writer.DeleteCredential(Record(44).credential_id); break;
        }
        vault_test::onStage = nullptr;
        Check(!swapSucceeded, "independent process cannot substitute live temp handle");
        Check(static_cast<bool>(status), "legitimate commit caller succeeds after blocked attack");
        VaultStore reader(swapPath.wstring());
        auto rows = reader.ListCredentials();
        Check(static_cast<bool>(rows), "committed result remains parseable");
        if (action == 0) Check(rows.value->empty(), "provision committed expected empty snapshot");
        if (action == 1) Check(rows.value->size() == 1, "migration retained record");
        if (action == 2) Check(rows.value->size() == 2, "add retained both records");
        if (action == 3) Check(rows.value->at(0).last_used_at == 123, "update committed expected timestamp");
        if (action == 4) Check(rows.value->empty(), "delete committed expected snapshot");
    }
}
#endif
void ForeignGrant(const fs::path& p)
{
    PACL old{}, next{};
    PSECURITY_DESCRIPTOR sd{};
    if (GetNamedSecurityInfoW(p.c_str(), SE_FILE_OBJECT, DACL_SECURITY_INFORMATION,
        nullptr, nullptr, &old, nullptr, &sd) != ERROR_SUCCESS)
        throw std::runtime_error("fixture ACL read");
    PSID everyone{};
    ConvertStringSidToSidW(L"S-1-1-0", &everyone);
    EXPLICIT_ACCESSW ace{};
    ace.grfAccessPermissions = FILE_WRITE_DATA;
    ace.grfAccessMode = GRANT_ACCESS;
    ace.Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ace.Trustee.ptstrName = static_cast<LPWSTR>(everyone);
    const DWORD built = SetEntriesInAclW(1, &ace, old, &next);
    const DWORD set = built == ERROR_SUCCESS ?
        SetNamedSecurityInfoW(const_cast<LPWSTR>(p.c_str()), SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION,
            nullptr, nullptr, next, nullptr) : built;
    LocalFree(next); LocalFree(everyone); LocalFree(sd);
    if (set != ERROR_SUCCESS) throw std::runtime_error("fixture ACL grant");
}
std::wstring LockName(const fs::path& p)
{
    HANDLE h = CreateFileW(p.parent_path().c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    BY_HANDLE_FILE_INFORMATION info{};
    if (!GetFileInformationByHandle(h, &info)) throw std::runtime_error("mutex fixture directory");
    CloseHandle(h);
    return L"Global\\AeDaeVault-" + std::to_wstring(info.dwVolumeSerialNumber) + L"-" +
        std::to_wstring(info.nFileIndexHigh) + L"-" + std::to_wstring(info.nFileIndexLow);
}
struct Child
{
    PROCESS_INFORMATION pi{};
    Child(const std::wstring& args)
    {
        wchar_t exe[32768]{};
        GetModuleFileNameW(nullptr, exe, 32768);
        std::wstring cmd = L"\"" + std::wstring(exe) + L"\" " + args;
        STARTUPINFOW si{}; si.cb = sizeof(si);
        if (!CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
            nullptr, nullptr, &si, &pi)) throw std::runtime_error("child create");
    }
    DWORD Join()
    {
        if (WaitForSingleObject(pi.hProcess, 30000) != WAIT_OBJECT_0)
        {
            TerminateProcess(pi.hProcess, 90);
            throw std::runtime_error("child timeout");
        }
        DWORD code{}; GetExitCodeProcess(pi.hProcess, &code); return code;
    }
    ~Child()
    {
        if (WaitForSingleObject(pi.hProcess, 0) == WAIT_TIMEOUT)
        {
            TerminateProcess(pi.hProcess, 90);
            WaitForSingleObject(pi.hProcess, 5000);
        }
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
    }
};

#ifdef AEDAE_VAULT_TEST_HOOKS
void CrashTests(const fs::path& root)
{
    unsigned number{};
    for (bool migration : {false, true})
    {
        for (const auto stage : {"temp-created", "partial-write", "written", "flushed", "before-rename", "renamed"})
        {
            const auto path = root / L"crash-nested" / (L"crash-" + std::to_wstring(number++) + L".bin");
            VaultStore writer(path.wstring(), VaultAccess::writer);
            Check(static_cast<bool>(writer.Provision()) && static_cast<bool>(writer.AddCredential(Record(2))),
                "crash fixture provisioned");
            const auto initial = Bytes(path);
            auto original = initial;
            auto expected = initial;
            if (migration)
            {
                original.erase(original.begin() + 16, original.begin() + 20);
                SetVersion(original, 1);
            }
            else
            {
                Check(static_cast<bool>(writer.AddCredential(Record(3))), "expected commit fixture");
                expected = Bytes(path);
            }
            Put(path, original);
            const auto name = LockName(path);
            HANDLE mutex = CreateMutexW(nullptr, FALSE, name.c_str());
            const auto eventName = L"Local\\aedae-crash-" + std::to_wstring(GetCurrentProcessId()) +
                L"-" + std::to_wstring(number);
            HANDLE event = CreateEventW(nullptr, TRUE, FALSE, eventName.c_str());
            Check(mutex && event, "crash synchronization ready");
            const std::string stageText(stage);
            const std::wstring wideStage(stageText.begin(), stageText.end());
            {
                Child child(L"--crash \"" + path.wstring() + L"\" \"" + wideStage + L"\" \"" +
                    eventName + L"\" " + (migration ? L"migrate" : L"add"));
                Check(WaitForSingleObject(event, 10000) == WAIT_OBJECT_0, "actual commit checkpoint reached");
                // The live writer pins its vault directory against replacement.
                Check(!MoveFileExW(path.parent_path().c_str(), (root / L"nested-moved").c_str(), 0),
                    "active vault directory cannot be renamed");
                const auto moved = root.wstring() + L"-moved";
                const bool ancestorMoved = MoveFileExW(root.c_str(), moved.c_str(), 0) != FALSE;
                if (ancestorMoved)
                    Check(MoveFileExW(moved.c_str(), root.c_str(), 0) != FALSE, "restore moved fixture ancestor");
                Check(!ancestorMoved, "active vault ancestor cannot be renamed");
                Check(TerminateProcess(child.pi.hProcess, 82) != FALSE && child.Join() == 82,
                    "writer killed at commit checkpoint");
            }
            const bool committed = stageText == "renamed";
            Check(Bytes(path) == (committed ? expected : original), "crash leaves exact old or new bytes");
            VaultStore reader(path.wstring());
            auto snapshot = reader.ListCredentials();
            Check(static_cast<bool>(snapshot) && snapshot.recovered_abandoned_mutex,
                "post-crash snapshot validates under abandoned mutex");
            Check(fs::exists(path.wstring() + L".tmp") == !committed, "expected crash leftover state");
            Check(static_cast<bool>(writer.Recover()) && !fs::exists(path.wstring() + L".tmp") &&
                Bytes(path) == (committed ? expected : original), "recovery discards without merge");
            if (migration) Check(static_cast<bool>(writer.Migrate()), "migration restart succeeds");
            else Check(static_cast<bool>(writer.UpdateLastUsed({2, 0}, 9)), "CRUD restart succeeds");
            CloseHandle(event);
            CloseHandle(mutex);
            std::cout << "CRASH " << (migration ? "migration " : "CRUD ") << stage << " verified" << std::endl;
        }
    }
}
#endif

void AclCases(const fs::path& root)
{
    HANDLE token{};
    Check(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token) != FALSE, "ACL fixture token");
    DWORD size{};
    GetTokenInformation(token, TokenUser, nullptr, 0, &size);
    std::vector<BYTE> bytes(size);
    Check(GetTokenInformation(token, TokenUser, bytes.data(), size, &size) != FALSE, "ACL fixture user");
    CloseHandle(token);
    LPWSTR user{};
    Check(ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(bytes.data())->User.Sid, &user) != FALSE,
        "ACL fixture SID");
    const std::wstring grant = L"(A;;FA;;;" + std::wstring(user) + L")";
    LocalFree(user);
    unsigned index{};
    struct Case { const wchar_t* ace; bool rejected; const char* label; };
    for (const auto& test : {
        Case{L"(A;;GW;;;WD)", true, "generic write refused"},
        Case{L"(A;OIIO;GW;;;WD)", true, "inherit-only foreign write refused"},
        Case{L"(A;;GR;;;WD)", false, "foreign read-only accepted"},
        Case{L"(D;;GW;;;WD)(A;;GW;;;WD)", true, "deny does not hide unexpected grant"}})
    {
        const auto path = root / (L"acl-case-" + std::to_wstring(index++)) / L"vault.bin";
        VaultStore writer(path.wstring(), VaultAccess::writer);
        Check(static_cast<bool>(writer.Provision()), "ACL case provisioned");
        PSECURITY_DESCRIPTOR sd{};
        const auto sddl = L"D:P" + grant + test.ace;
        Check(ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1,
            &sd, nullptr) != FALSE, "ACL case parsed");
        PACL dacl{}; BOOL present{}, defaulted{};
        Check(GetSecurityDescriptorDacl(sd, &present, &dacl, &defaulted) != FALSE, "ACL case DACL");
        const auto object = std::wstring(test.ace).find(L"OIIO") != std::wstring::npos ? path.parent_path() : path;
        const DWORD status = SetNamedSecurityInfoW(const_cast<LPWSTR>(object.c_str()), SE_FILE_OBJECT,
            DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION, nullptr, nullptr, dacl, nullptr);
        LocalFree(sd);
        Check(status == ERROR_SUCCESS, "ACL case installed");
        auto result = writer.ListCredentials();
        Check(test.rejected ? result.error == VaultError::acl_policy : static_cast<bool>(result), test.label);
    }
}


#ifdef AEDAE_VAULT_TEST_HOOKS
void DescriptorCases()
{
    PSID user{}, foreign{};
    HANDLE token{};
    Check(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token) != FALSE, "descriptor token");
    DWORD size{};
    GetTokenInformation(token, TokenUser, nullptr, 0, &size);
    std::vector<BYTE> tokenBytes(size);
    Check(GetTokenInformation(token, TokenUser, tokenBytes.data(), size, &size) != FALSE, "descriptor user");
    CloseHandle(token);
    user = reinterpret_cast<TOKEN_USER*>(tokenBytes.data())->User.Sid;
    Check(ConvertStringSidToSidW(L"S-1-1-0", &foreign) != FALSE, "descriptor foreign SID");
    alignas(ACL) BYTE storage[1024]{};
    const auto acl = reinterpret_cast<PACL>(storage);
    Check(InitializeAcl(acl, sizeof(storage), ACL_REVISION_DS) != FALSE &&
        AddAccessAllowedAce(acl, ACL_REVISION_DS, FILE_ALL_ACCESS, user) != FALSE, "descriptor baseline");
    Check(static_cast<bool>(vault_test::ValidateDescriptor(user, acl)), "descriptor current owner accepted");
    Check(vault_test::ValidateDescriptor(foreign, acl).error == VaultError::acl_policy,
        "foreign owner rejected by production validator");
    Check(vault_test::ValidateDescriptor(user, nullptr).error == VaultError::acl_policy, "null DACL refused");
    Check(AddAccessAllowedAce(acl, ACL_REVISION_DS, GENERIC_WRITE, foreign) != FALSE, "callback fixture ACE");
    void* raw{};
    Check(GetAce(acl, 1, &raw) != FALSE, "callback fixture address");
    static_cast<ACE_HEADER*>(raw)->AceType = ACCESS_ALLOWED_CALLBACK_ACE_TYPE;
    Check(IsValidAcl(acl) != FALSE, "callback fixture structurally valid");
    Check(vault_test::ValidateDescriptor(user, acl).error == VaultError::acl_policy,
        "callback ACE rejected by production validator");
    Check(InitializeAcl(acl, sizeof(storage), ACL_REVISION_DS) != FALSE, "object ACL fixture");
    GUID objectType{0x12345678, 0x1234, 0x4321, {0x80, 0, 0, 0, 0, 0, 0, 1}};
    Check(AddAccessAllowedObjectAce(acl, ACL_REVISION_DS, 0, GENERIC_WRITE, &objectType, nullptr, foreign) != FALSE,
        "object-specific ACE fixture");
    Check(IsValidAcl(acl) != FALSE && vault_test::ValidateDescriptor(user, acl).error == VaultError::acl_policy,
        "object ACE rejected by production validator");
    LocalFree(foreign);
}
#endif
void OwnerAndReparseCases(const fs::path& root)
{
    const auto target = root / L"reparse-target.bin";
    VaultStore writer(target.wstring(), VaultAccess::writer);
    Check(static_cast<bool>(writer.Provision()) && static_cast<bool>(writer.AddCredential(Record(7))),
        "reparse fixture provisioned");
    const auto original = Bytes(target);
    const auto link = root / L"reparse-link.bin";
    if (CreateSymbolicLinkW(link.c_str(), target.c_str(), SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE))
    {
        VaultStore reader(link.wstring());
        Check(reader.ListCredentials().error == VaultError::acl_policy && Bytes(target) == original,
            "file reparse rejected without changing target");
        Check(DeleteFileW(link.c_str()) != FALSE, "remove synthetic reparse link");
        const auto temp = target.wstring() + L".tmp";
        Check(CreateSymbolicLinkW(temp.c_str(), target.c_str(), SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE) != FALSE,
            "temp reparse fixture");
        Check(writer.Recover().error == VaultError::acl_policy && Bytes(target) == original,
            "reparse leftover refused without deleting target");
        Check(DeleteFileW(temp.c_str()) != FALSE, "remove synthetic temp link");
    }
    else
    {
        const DWORD error = GetLastError();
        Check(error == ERROR_PRIVILEGE_NOT_HELD || error == ERROR_ACCESS_DENIED,
            "symlink fixture failure is a known permission limit");
        std::cout << "LIMITATION file symlink fixture unavailable, Win32 " << error << std::endl;
    }
    PSID foreign{};
    Check(ConvertStringSidToSidW(L"S-1-1-0", &foreign) != FALSE, "foreign owner SID");
    const DWORD status = SetNamedSecurityInfoW(const_cast<LPWSTR>(target.c_str()), SE_FILE_OBJECT,
        OWNER_SECURITY_INFORMATION, foreign, nullptr, nullptr, nullptr);
    LocalFree(foreign);
    if (status == ERROR_SUCCESS)
        Check(writer.ListCredentials().error == VaultError::acl_policy, "foreign owner refused");
    else
    {
        Check(status == ERROR_INVALID_OWNER || status == ERROR_PRIVILEGE_NOT_HELD || status == ERROR_ACCESS_DENIED,
            "foreign owner fixture failure is a known permission limit");
        std::cout << "LIMITATION foreign ownership fixture unavailable, Win32 " << status << std::endl;
    }
}
int wmain(int argc, wchar_t** argv)
{
    try
    {
#ifdef AEDAE_VAULT_TEST_HOOKS
        if (argc == 4 && std::wstring(argv[1]) == L"--swap")
            return MoveFileExW(argv[2], argv[3], 0) ? 0 : 86;
#endif
#ifdef AEDAE_VAULT_TEST_HOOKS
        if (argc == 6 && std::wstring(argv[1]) == L"--crash")
        {
            const std::wstring stage(argv[3]);
            for (const wchar_t c : stage)
            {
                if (c > 127) return 85;
                vault_test::stopStage.push_back(static_cast<char>(c));
            }
            vault_test::readyEvent = argv[4];
            VaultStore writer(argv[2], VaultAccess::writer);
            const auto status = std::wstring(argv[5]) == L"migrate" ? writer.Migrate() : writer.AddCredential(Record(3));
            return status ? 83 : 84;
        }
#endif
        if (argc >= 3 && std::wstring(argv[1]) == L"--write")
        {
            VaultStore v(argv[2], VaultAccess::writer);
            for (unsigned n = 100; n < 130; ++n)
                if (!v.AddCredential(Record(n))) return 30;
            return 0;
        }
        if (argc >= 4 && std::wstring(argv[1]) == L"--abandon")
        {
            HANDLE h = OpenMutexW(SYNCHRONIZE | MUTEX_MODIFY_STATE, FALSE, argv[2]);
            if (!h || WaitForSingleObject(h, 3000) != WAIT_OBJECT_0) return 31;
            HANDLE e = OpenEventW(EVENT_MODIFY_STATE, FALSE, argv[3]);
            SetEvent(e); CloseHandle(e);
            Sleep(30000);
            return 32;
        }
        const auto root = fs::temp_directory_path() /
            (L"aedae-review-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(GetTickCount64()));
        const auto path = root / L"vault.bin";
#ifdef AEDAE_VAULT_TEST_HOOKS
        if (argc == 2 && std::wstring(argv[1]) == L"--collision-only")
        {
            CollisionCase(root);
            return 0;
        }
        if (argc == 2 && std::wstring(argv[1]) == L"--reproduce-substitution")
        {
            ReproduceSubstitution(root, true);
            std::wcout << L"Synthetic reproduction fixtures: " << root << std::endl;
            return 0;
        }
        if (argc == 2 && std::wstring(argv[1]) == L"--test-substitution")
        {
            ReproduceSubstitution(root, true);
            return 0;
        }
#endif
        VaultStore writer(path.wstring(), VaultAccess::writer);
        VaultStore reader(path.wstring());
        Check(!reader.ListCredentials() && !fs::exists(root), "snapshot does not provision");
        Check(static_cast<bool>(writer.Provision()), "restrictive creation");
        Check(static_cast<bool>(reader.ListCredentials()), "trusted owner and ACL accepted");
        Check(reader.AddCredential(Record(1)).error == VaultError::read_only, "snapshot rejects writes");
        Check(static_cast<bool>(writer.AddCredential(Record(1))), "add");
        auto other = Record(1); other.rp_id = L"other.test"; other.id = L"other";
        Check(writer.AddCredential(other).error == VaultError::duplicate, "global credential identity");
        Check(static_cast<bool>(reader.GetCredentialById(L"example.test", {1, 0})), "find by id");
        Check(reader.GetCredentialById(L"other.test", {1, 0}).error == VaultError::not_found, "RP isolation");
        Check(reader.FindCredentialsForRp(L"example.test").value->size() == 1, "find RP");
        Check(reader.ListCredentials().value->at(0).protected_private_key.empty(), "metadata projection");
        Check(static_cast<bool>(writer.UpdateLastUsed({1, 0}, 44)) &&
            reader.ListCredentials().value->at(0).last_used_at == 44, "update");
        Check(writer.DeleteCredential({99}).error == VaultError::not_found &&
            static_cast<bool>(writer.DeleteCredential({1, 0})), "delete after error");
        Check(reader.ListCredentials().value->empty(), "empty is successful result");

        Check(static_cast<bool>(writer.AddCredential(Record(2))), "corruption fixture");
        const auto good = Bytes(path);
        auto bad = good; bad.push_back(0); Put(path, bad);
        Check(!reader.ListCredentials() && Bytes(path) == bad, "trailing bytes rejected intact");
        bad = good; bad.resize(bad.size() - 1); Put(path, bad);
        Check(!reader.ListCredentials() && Bytes(path) == bad, "truncation rejected intact");
        bad = good; SetVersion(bad, 99); Put(path, bad);
        Check(reader.ListCredentials().error == VaultError::unsupported_schema &&
            Bytes(path) == bad, "unknown schema intact");
        bad = good; unsigned huge = 100001; std::memcpy(bad.data() + 12, &huge, 4); Put(path, bad);
        Check(!reader.ListCredentials(), "record count bound");
        bad = good; huge = 0xffffffff; std::memcpy(bad.data() + 20, &huge, 4); Put(path, bad);
        Check(!reader.ListCredentials(), "string bound");
        bad = good; huge = 99; std::memcpy(bad.data() + 16, &huge, 4); Put(path, bad);
        Check(reader.ListCredentials().error == VaultError::corrupt, "record schema checked");
        bad = good;
        size_t blobOffset = 20;
        for (unsigned field = 0; field < 3; ++field)
        {
            unsigned length{};
            std::memcpy(&length, good.data() + blobOffset, 4);
            blobOffset += 4 + length * sizeof(wchar_t);
        }
        huge = 0xffffffff; std::memcpy(bad.data() + blobOffset, &huge, 4); Put(path, bad);
        Check(reader.ListCredentials().error == VaultError::corrupt, "blob checked before allocation");
        Put(path, good);
        const auto hardlink = root / L"alias.bin";
        Check(CreateHardLinkW(hardlink.c_str(), path.c_str(), nullptr) != FALSE, "hardlink fixture");
        Check(reader.ListCredentials().error == VaultError::acl_policy, "hardlinked vault rejected");
        Check(DeleteFileW(hardlink.c_str()) != FALSE && static_cast<bool>(reader.ListCredentials()),
            "lock released after constructor validation failure");
        // True legacy layout: remove the version-3 per-record field, then stamp legacy schema.
        auto legacy = good; legacy.erase(legacy.begin() + 16, legacy.begin() + 20); SetVersion(legacy, 1);
        Put(path, legacy);
        Check(static_cast<bool>(reader.ListCredentials()) && Bytes(path) == legacy, "legacy read does not migrate");
        Check(writer.UpdateLastUsed({2, 0}, 50).error == VaultError::unsupported_schema,
            "CRUD requires explicit migration");
        HANDLE held = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
        Check(held != INVALID_HANDLE_VALUE, "failure injection active");
        auto failed = writer.Migrate();
        CloseHandle(held);
        Check(failed.error == VaultError::migration_failed && Bytes(path) == legacy &&
            !fs::exists(path.wstring() + L".tmp"), "migration failure preserves original and cleans temp");
        Check(static_cast<bool>(writer.Migrate()) && Bytes(path) == good, "real legacy migration");
        Put(path.wstring() + L".tmp", good);
        const auto before = Bytes(path);
        Check(static_cast<bool>(reader.ListCredentials()) && fs::exists(path.wstring() + L".tmp"),
            "snapshot never deletes temp");
        Check(static_cast<bool>(writer.Recover()) && !fs::exists(path.wstring() + L".tmp") &&
            Bytes(path) == before, "writer discards leftover without merge");

        const auto name = LockName(path);
        HANDLE mutex = CreateMutexW(nullptr, FALSE, name.c_str());
        std::atomic<bool> acquired{false};
        std::thread holder([&]
        {
            WaitForSingleObject(mutex, 3000); acquired = true;
            Sleep(3000); ReleaseMutex(mutex);
        });
        while (!acquired) SwitchToThread();
        auto timed = reader.ListCredentials();
        holder.join();
        Check(timed.error == VaultError::timeout, "bounded lock timeout");
        const auto eventName = L"Local\\aedae-test-" + std::to_wstring(GetCurrentProcessId());
        HANDLE event = CreateEventW(nullptr, TRUE, FALSE, eventName.c_str());
        {
            Child child(L"--abandon \"" + name + L"\" \"" + eventName + L"\"");
            Check(WaitForSingleObject(event, 5000) == WAIT_OBJECT_0, "child owns mutex");
            TerminateProcess(child.pi.hProcess, 33);
            Check(child.Join() == 33, "holder terminated");
        }
        auto recovered = reader.ListCredentials();
        Check(static_cast<bool>(recovered) && recovered.recovered_abandoned_mutex, "abandoned owner reported");
        CloseHandle(event); CloseHandle(mutex);

        std::wstring alias = path.wstring();
        wchar_t shortPath[32768]{};
        DWORD shortSize = GetShortPathNameW(root.c_str(), shortPath, 32768);
        if (shortSize && shortSize < 32768 && std::wstring(shortPath) != root.wstring())
        {
            alias = (fs::path(shortPath) / path.filename()).wstring();
            std::cout << "ALIAS short path enabled" << std::endl;
        }
        else std::cout << "LIMITATION short-name alias unavailable on this volume" << std::endl;
        Child child(L"--write \"" + alias + L"\"");
        for (unsigned n = 200; n < 230; ++n)
        {
            Check(static_cast<bool>(writer.AddCredential(Record(n))), "parent commit");
            auto snapshot = reader.ListCredentials();
            Check(static_cast<bool>(snapshot) && !snapshot.value->empty(), "concurrent snapshot");
        }
        Check(child.Join() == 0, "second writer completed");
        auto all = reader.ListCredentials();
        Check(static_cast<bool>(all) && all.value->size() == 61, "no lost cross-process commits");
        for (unsigned n = 100; n < 130; ++n)
            Check(static_cast<bool>(reader.GetCredentialById(L"example.test", Record(n).credential_id)), "child record retained");

        const auto longFile = root / L"long-vault-filename-for-alias.bin";
        VaultStore longWriter(longFile.wstring(), VaultAccess::writer);
        Check(static_cast<bool>(longWriter.Provision()) && static_cast<bool>(longWriter.AddCredential(Record(5))),
            "filename alias fixture");
        shortSize = GetShortPathNameW(longFile.c_str(), shortPath, 32768);
        if (shortSize && shortSize < 32768 && fs::path(shortPath).filename() != longFile.filename())
        {
            VaultStore aliasWriter(shortPath, VaultAccess::writer);
            Check(static_cast<bool>(aliasWriter.UpdateLastUsed({5, 0}, 77)) &&
                longWriter.ListCredentials().value->at(0).last_used_at == 77,
                "short filename resolves to original commit destination");
        }
        else std::cout << "LIMITATION short filename unavailable" << std::endl;
#ifdef AEDAE_VAULT_TEST_HOOKS
        CrashTests(root);
        CommitCallerSubstitutionCases(root);
        DescriptorCases();
        CollisionCase(root);
#endif
        AclCases(root);
        OwnerAndReparseCases(root);
        ForeignGrant(longFile);
        Check(longWriter.ListCredentials().error == VaultError::acl_policy,
            "foreign file writer rejected with caller access preserved");
        ForeignGrant(root);
        auto denied = reader.ListCredentials();
        Check(denied.error == VaultError::acl_policy, "foreign writer rejected with caller access preserved");
        std::cout << "VaultStoreTests passed; synthetic fixtures retained at " << root.string() << std::endl;
        return 0;
    }
    catch (const std::exception& e) { std::cerr << "FAIL " << e.what() << std::endl; return 1; }
}
