#include "VaultStore.h"
#include <windows.h>
#include <aclapi.h>
#include <sddl.h>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <set>
#include <stdexcept>
#pragma comment(lib, "Advapi32.lib")

namespace aedae
{
#ifdef AEDAE_VAULT_TEST_HOOKS
namespace vault_test { void Checkpoint(const char* stage); }
#endif
namespace
{
void Checkpoint([[maybe_unused]] const char* stage)
{
#ifdef AEDAE_VAULT_TEST_HOOKS
    vault_test::Checkpoint(stage);
#endif
}
constexpr DWORD kWaitMs = 2000;
constexpr size_t kMaxFile = 16 * 1024 * 1024;
constexpr size_t kMaxField = 1024 * 1024;
constexpr unsigned kSchema = 3;
struct Failure { VaultError error; };
void Require(bool ok, VaultError error = VaultError::io)
{
    if (!ok) throw Failure{error};
}
struct Handle
{
    HANDLE value{INVALID_HANDLE_VALUE};
    explicit Handle(HANDLE h = INVALID_HANDLE_VALUE) : value(h) {}
    ~Handle() { if (value != INVALID_HANDLE_VALUE && value != nullptr) CloseHandle(value); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
};
struct LocalMemory
{
    void* value{};
    ~LocalMemory() { if (value) LocalFree(value); }
};
struct Security
{
    LocalMemory descriptor;
    SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), nullptr, FALSE};
    Security()
    {
        Handle token;
        Require(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token.value) != FALSE);
        DWORD size{};
        GetTokenInformation(token.value, TokenUser, nullptr, 0, &size);
        std::vector<BYTE> bytes(size);
        Require(GetTokenInformation(token.value, TokenUser, bytes.data(), size, &size) != FALSE);
        LocalMemory sid;
        Require(ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER*>(bytes.data())->User.Sid,
            reinterpret_cast<LPWSTR*>(&sid.value)) != FALSE);
        const std::wstring text = L"D:P(A;OICI;FA;;;" +
            std::wstring(static_cast<LPWSTR>(sid.value)) + L")(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)";
        Require(ConvertStringSecurityDescriptorToSecurityDescriptorW(text.c_str(),
            SDDL_REVISION_1, &descriptor.value, nullptr) != FALSE);
        attributes.lpSecurityDescriptor = descriptor.value;
    }
};
bool Trusted(PSID sid)
{
    if (!sid || !IsValidSid(sid)) return false;
    if (IsWellKnownSid(sid, WinLocalSystemSid) || IsWellKnownSid(sid, WinBuiltinAdministratorsSid))
        return true;
    Handle token;
    Require(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token.value) != FALSE);
    DWORD size{};
    GetTokenInformation(token.value, TokenUser, nullptr, 0, &size);
    std::vector<BYTE> bytes(size);
    Require(GetTokenInformation(token.value, TokenUser, bytes.data(), size, &size) != FALSE);
    return EqualSid(sid, reinterpret_cast<TOKEN_USER*>(bytes.data())->User.Sid) != FALSE;
}
void CheckDescriptor(PSID owner, PACL acl)
{
    // ADR-002 section 5, owner-approved correction: user, SYSTEM and Administrators.
    Require(Trusted(owner) && acl && IsValidAcl(acl), VaultError::acl_policy);
    constexpr DWORD write = FILE_WRITE_DATA | FILE_APPEND_DATA | FILE_WRITE_EA |
        FILE_WRITE_ATTRIBUTES | FILE_DELETE_CHILD | DELETE | WRITE_DAC | WRITE_OWNER |
        GENERIC_WRITE | GENERIC_ALL | MAXIMUM_ALLOWED;
    for (DWORD index = 0; index < acl->AceCount; ++index)
    {
        void* raw{};
        Require(GetAce(acl, index, &raw) != FALSE, VaultError::acl_policy);
        const auto header = static_cast<ACE_HEADER*>(raw);
        if (header->AceType == ACCESS_DENIED_ACE_TYPE) continue;
        // Unsupported callback/object ACE semantics are rejected conservatively.
        Require(header->AceType == ACCESS_ALLOWED_ACE_TYPE, VaultError::acl_policy);
        Require(header->AceSize >= sizeof(ACCESS_ALLOWED_ACE), VaultError::acl_policy);
        const auto ace = static_cast<ACCESS_ALLOWED_ACE*>(raw);
        // Include inherit-only grants: children must not inherit unexpected write access.
        if (ace->Mask & write)
            Require(Trusted(&ace->SidStart), VaultError::acl_policy);
    }
}
void CheckAcl(HANDLE handle)
{
    LocalMemory sd;
    PSID owner{};
    PACL acl{};
    const DWORD status = GetSecurityInfo(handle, SE_FILE_OBJECT,
        OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
        &owner, nullptr, &acl, nullptr, &sd.value);
    Require(status == ERROR_SUCCESS, VaultError::access_denied);
    CheckDescriptor(owner, acl);
}
void CheckFile(HANDLE h, bool directory)
{
    FILE_ATTRIBUTE_TAG_INFO info{};
    Require(GetFileInformationByHandleEx(h, FileAttributeTagInfo, &info, sizeof(info)) != FALSE);
    Require(!(info.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT), VaultError::acl_policy);
    Require(((info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) == directory, VaultError::io);
    if (!directory)
    {
        BY_HANDLE_FILE_INFORMATION links{};
        Require(GetFileInformationByHandle(h, &links) != FALSE);
        Require(links.nNumberOfLinks == 1, VaultError::acl_policy);
    }
    CheckAcl(h);
}
std::wstring FinalPath(HANDLE h)
{
    const DWORD size = GetFinalPathNameByHandleW(h, nullptr, 0, FILE_NAME_NORMALIZED | VOLUME_NAME_GUID);
    Require(size != 0);
    std::wstring path(size, L'\0');
    const DWORD written = GetFinalPathNameByHandleW(h, path.data(), size,
        FILE_NAME_NORMALIZED | VOLUME_NAME_GUID);
    Require(written != 0 && written < size);
    path.resize(written);
    return path;
}
struct Context
{
    Handle directory;
    Handle mutex;
    std::wstring path;
    bool owns{}, abandoned{};
    Context(const std::wstring& input)
    {
        const std::filesystem::path supplied(input);
        const auto leaf = supplied.filename().wstring();
        Require(!leaf.empty() && leaf != L"." && leaf != L".." &&
            leaf.find_first_of(L":/\\") == std::wstring::npos &&
            leaf.back() != L'.' && leaf.back() != L' ', VaultError::io);
        // FILE_LIST_DIRECTORY is intentional: a metadata-only handle did not block
        // directory rename in the crash harness. Keep this read handle without delete sharing.
        // Parent aliases resolve to the same opened directory; file reparse points are refused.
        directory.value = CreateFileW(supplied.parent_path().c_str(), READ_CONTROL | FILE_READ_ATTRIBUTES | FILE_LIST_DIRECTORY,
            FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        Require(directory.value != INVALID_HANDLE_VALUE, VaultError::access_denied);
        CheckFile(directory.value, true);
        path = FinalPath(directory.value) + L"\\" + leaf;
        // Directory identity is stable across aliases and atomic file replacements.
        BY_HANDLE_FILE_INFORMATION identity{};
        Require(GetFileInformationByHandle(directory.value, &identity) != FALSE);
        // Deliberately serialize the entire dedicated directory, including filename aliases.
        const auto name = L"Global\\AeDaeVault-" + std::to_wstring(identity.dwVolumeSerialNumber) +
            L"-" + std::to_wstring(identity.nFileIndexHigh) + L"-" +
            std::to_wstring(identity.nFileIndexLow);
        Require(name.size() < 250, VaultError::resource_limit);
        mutex.value = CreateMutexW(nullptr, FALSE, name.c_str());
        Require(mutex.value != nullptr);
        switch (WaitForSingleObject(mutex.value, kWaitMs))
        {
        case WAIT_OBJECT_0: owns = true; break;
        case WAIT_ABANDONED: owns = true; abandoned = true; break;
        case WAIT_TIMEOUT: throw Failure{VaultError::timeout};
        default: throw Failure{VaultError::io};
        }
        try
        {
            Handle existing(CreateFileW(path.c_str(), READ_CONTROL | FILE_READ_ATTRIBUTES,
                FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
            if (existing.value != INVALID_HANDLE_VALUE)
            {
                CheckFile(existing.value, false);
                path = FinalPath(existing.value);
            }
            else Require(GetLastError() == ERROR_FILE_NOT_FOUND, VaultError::access_denied);
        }
        catch (...)
        {
            ReleaseMutex(mutex.value);
            owns = false;
            throw;
        }
    }
    ~Context() { if (owns) ReleaseMutex(mutex.value); }
};
struct Snapshot { std::vector<CredentialRecord> records; unsigned version{kSchema}; };
struct Decoder
{
    const std::vector<BYTE>& bytes;
    size_t at{};
    template<class T> T Number()
    {
        Require(sizeof(T) <= bytes.size() - at, VaultError::corrupt);
        T value{};
        std::memcpy(&value, bytes.data() + at, sizeof(T));
        at += sizeof(T);
        return value;
    }
    std::vector<BYTE> Blob()
    {
        const auto count = Number<unsigned>();
        Require(count <= kMaxField && count <= bytes.size() - at, VaultError::corrupt);
        std::vector<BYTE> result(bytes.begin() + at, bytes.begin() + at + count);
        at += count;
        return result;
    }
    std::wstring Text()
    {
        const auto count = Number<unsigned>();
        Require(count <= 32768 && count <= (bytes.size() - at) / sizeof(wchar_t), VaultError::corrupt);
        std::wstring result(count, L'\0');
        std::memcpy(result.data(), bytes.data() + at, count * sizeof(wchar_t));
        at += count * sizeof(wchar_t);
        return result;
    }
};
struct Encoder
{
    std::vector<BYTE> bytes;
    template<class T> void Number(T value)
    {
        Require(bytes.size() + sizeof(T) <= kMaxFile, VaultError::resource_limit);
        const auto p = reinterpret_cast<const BYTE*>(&value);
        bytes.insert(bytes.end(), p, p + sizeof(T));
    }
    void Blob(const std::vector<BYTE>& value)
    {
        Require(value.size() <= kMaxField && bytes.size() + value.size() + 4 <= kMaxFile,
            VaultError::resource_limit);
        Number(static_cast<unsigned>(value.size()));
        bytes.insert(bytes.end(), value.begin(), value.end());
    }
    void Text(const std::wstring& value)
    {
        Require(value.size() <= 32768 && bytes.size() + value.size() * 2 + 4 <= kMaxFile,
            VaultError::resource_limit);
        Number(static_cast<unsigned>(value.size()));
        const auto p = reinterpret_cast<const BYTE*>(value.data());
        bytes.insert(bytes.end(), p, p + value.size() * sizeof(wchar_t));
    }
};
void Validate(const std::vector<CredentialRecord>& records)
{
    Require(records.size() <= 10000, VaultError::resource_limit);
    std::set<std::vector<BYTE>> ids;
    std::set<std::wstring> names;
    for (const auto& r : records)
    {
        Require(!r.id.empty() && !r.rp_id.empty() && !r.credential_id.empty(), VaultError::corrupt);
        Require(ids.insert(r.credential_id).second && names.insert(r.id).second, VaultError::duplicate);
    }
}
Snapshot Load(const Context& context)
{
    Handle file(CreateFileW(context.path.c_str(), GENERIC_READ | READ_CONTROL,
        FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    Require(file.value != INVALID_HANDLE_VALUE, VaultError::access_denied);
    CheckFile(file.value, false);
    LARGE_INTEGER size{};
    Require(GetFileSizeEx(file.value, &size) != FALSE);
    Require(size.QuadPart >= 16 && size.QuadPart <= kMaxFile, VaultError::corrupt);
    std::vector<BYTE> bytes(static_cast<size_t>(size.QuadPart));
    DWORD read{};
    Require(ReadFile(file.value, bytes.data(), static_cast<DWORD>(bytes.size()), &read, nullptr) != FALSE);
    Require(read == bytes.size(), VaultError::corrupt);
    Require(std::memcmp(bytes.data(), "AEDVAULT", 8) == 0, VaultError::corrupt);
    Decoder d{bytes, 8};
    Snapshot result;
    result.version = d.Number<unsigned>();
    Require(result.version >= 1 && result.version <= kSchema, VaultError::unsupported_schema);
    const auto count = d.Number<unsigned>();
    Require(count <= 10000, VaultError::corrupt);
    for (unsigned i = 0; i < count; ++i)
    {
        CredentialRecord r;
        // Legacy 1/2 used identical layouts. Version 3 adds an explicit per-record schema.
        if (result.version == 3)
            Require(d.Number<unsigned>() == 3, VaultError::corrupt);
        r.id = d.Text(); r.rp_id = d.Text(); r.account_label = d.Text();
        r.user_handle = d.Blob(); r.credential_id = d.Blob(); r.public_key_cose = d.Blob();
        r.protected_private_key = d.Blob();
        r.created_at = d.Number<std::uint64_t>(); r.last_used_at = d.Number<std::uint64_t>();
        r.user_verification_policy = d.Text(); r.origin_source = d.Text(); r.backup_status = d.Text();
        result.records.push_back(std::move(r));
    }
    Require(d.at == bytes.size(), VaultError::corrupt);
    Validate(result.records);
    return result;
}
void DiscardTemp(const Context& c)
{
    const auto path = c.path + L".tmp";
    Handle file(CreateFileW(path.c_str(), DELETE | READ_CONTROL | FILE_READ_ATTRIBUTES, 0,
        nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    if (file.value == INVALID_HANDLE_VALUE)
    {
        Require(GetLastError() == ERROR_FILE_NOT_FOUND);
        return;
    }
    CheckFile(file.value, false);
    FILE_DISPOSITION_INFO info{TRUE};
    Require(SetFileInformationByHandle(file.value, FileDispositionInfo, &info, sizeof(info)) != FALSE);
}
void Commit(const Context& context, const std::vector<CredentialRecord>& records)
{
    Validate(records);
    Encoder e;
    for (char c : std::string("AEDVAULT")) e.Number(static_cast<BYTE>(c));
    e.Number(kSchema); e.Number(static_cast<unsigned>(records.size()));
    for (const auto& r : records)
    {
        e.Number(kSchema);
        e.Text(r.id); e.Text(r.rp_id); e.Text(r.account_label);
        e.Blob(r.user_handle); e.Blob(r.credential_id); e.Blob(r.public_key_cose);
        e.Blob(r.protected_private_key);
        e.Number(r.created_at); e.Number(r.last_used_at);
        e.Text(r.user_verification_policy); e.Text(r.origin_source); e.Text(r.backup_status);
    }
    DiscardTemp(context);
    Security security;
    const auto temp = context.path + L".tmp";
    Checkpoint("before-temp-create");
    Handle file(CreateFileW(temp.c_str(), GENERIC_WRITE | READ_CONTROL | DELETE,
        0, &security.attributes, CREATE_NEW, FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
    Require(file.value != INVALID_HANDLE_VALUE);
    struct HandleCleanup
    {
        HANDLE file;
        bool committed{};
        ~HandleCleanup()
        {
            if (!committed)
            {
                FILE_DISPOSITION_INFO disposition{TRUE};
                (void)SetFileInformationByHandle(file, FileDispositionInfo,
                    &disposition, sizeof(disposition));
            }
        }
    } cleanup{file.value};
    CheckFile(file.value, false);
    Checkpoint("temp-created");
    DWORD written{};
    const DWORD prefix = static_cast<DWORD>(e.bytes.size() / 2);
    Require(WriteFile(file.value, e.bytes.data(), prefix,
        &written, nullptr) != FALSE && written == prefix);
    Checkpoint("partial-write");
    const DWORD remainder = static_cast<DWORD>(e.bytes.size()) - prefix;
    Require(WriteFile(file.value, e.bytes.data() + prefix, remainder,
        &written, nullptr) != FALSE && written == remainder);
    Checkpoint("written");
    Require(FlushFileBuffers(file.value) != FALSE);
    Checkpoint("flushed");
    Checkpoint("before-rename");
    const auto& destination = context.path;
    const size_t nameBytes = destination.size() * sizeof(wchar_t);
    const size_t renameBytes = offsetof(FILE_RENAME_INFO, FileName) + nameBytes + sizeof(wchar_t);
    Require(renameBytes <= MAXDWORD, VaultError::resource_limit);
    std::vector<BYTE> renameStorage(renameBytes);
    auto rename = reinterpret_cast<FILE_RENAME_INFO*>(renameStorage.data());
    rename->ReplaceIfExists = TRUE;
    rename->RootDirectory = nullptr;
    rename->FileNameLength = static_cast<DWORD>(nameBytes);
    std::memcpy(rename->FileName, destination.data(), nameBytes);
    Require(SetFileInformationByHandle(file.value, FileRenameInfo,
        rename, static_cast<DWORD>(renameBytes)) != FALSE);
    cleanup.committed = true;
    Checkpoint("renamed");
}
template<class F> VaultStatus Status(F&& f)
{
    try { return f(); }
    catch (const Failure& e) { return {e.error}; }
    catch (const std::bad_alloc&) { return {VaultError::resource_limit}; }
    catch (...) { return {VaultError::io}; }
}
template<class F> VaultResult<std::vector<CredentialRecord>> ReadResult(F&& f)
{
    try { return f(); }
    catch (const Failure& e) { return {e.error, std::nullopt}; }
    catch (const std::bad_alloc&) { return {VaultError::resource_limit, std::nullopt}; }
    catch (...) { return {VaultError::io, std::nullopt}; }
}
void Metadata(CredentialRecord& r)
{
    r.protected_private_key.clear(); r.public_key_cose.clear(); r.user_handle.clear();
}
}
VaultStore::VaultStore(std::wstring path, VaultAccess access) : path_(std::move(path)), access_(access) {}
#ifdef AEDAE_VAULT_TEST_HOOKS
namespace vault_test
{
VaultStatus ValidateDescriptor(PSID owner, PACL acl)
{
    return Status([&]() -> VaultStatus { CheckDescriptor(owner, acl); return {}; });
}
}
#endif
VaultStatus VaultStore::Provision()
{
    return Status([&]() -> VaultStatus
    {
        Require(access_ == VaultAccess::writer, VaultError::read_only);
        const auto parent = std::filesystem::path(path_).parent_path();
        Security security;
        if (!CreateDirectoryW(parent.c_str(), &security.attributes))
            Require(GetLastError() == ERROR_ALREADY_EXISTS);
        Context c(path_);
        const auto attrs = GetFileAttributesW(c.path.c_str());
        if (attrs == INVALID_FILE_ATTRIBUTES)
        {
            Require(GetLastError() == ERROR_FILE_NOT_FOUND);
            Commit(c, {});
        }
        else (void)Load(c);
        return {VaultError::none, c.abandoned};
    });
}
VaultStatus VaultStore::Recover()
{
    return Status([&]() -> VaultStatus
    {
        Require(access_ == VaultAccess::writer, VaultError::read_only);
        Context c(path_);
        (void)Load(c);
        DiscardTemp(c);
        return {VaultError::none, c.abandoned};
    });
}
VaultStatus VaultStore::Migrate()
{
    return Status([&]() -> VaultStatus
    {
        Require(access_ == VaultAccess::writer, VaultError::read_only);
        Context c(path_);
        auto snapshot = Load(c);
        if (snapshot.version < kSchema)
        {
            try { Commit(c, snapshot.records); }
            catch (const Failure&) { return {VaultError::migration_failed, c.abandoned}; }
        }
        return {VaultError::none, c.abandoned};
    });
}
VaultStatus VaultStore::Mutate(int op, const CredentialRecord* record, const std::vector<BYTE>& id, std::uint64_t time)
{
    return Status([&]() -> VaultStatus
    {
        Require(access_ == VaultAccess::writer, VaultError::read_only);
        Context c(path_);
        auto snapshot = Load(c);
        // Explicit migration prevents accidental upgrades by ordinary CRUD.
        Require(snapshot.version == kSchema, VaultError::unsupported_schema);
        auto& rows = snapshot.records;
        auto it = std::find_if(rows.begin(), rows.end(), [&](const auto& r) { return r.credential_id == id; });
        if (op == 0)
        {
            Require(record != nullptr);
            Require(it == rows.end() && std::none_of(rows.begin(), rows.end(),
                [&](const auto& r) { return r.id == record->id; }), VaultError::duplicate);
            rows.push_back(*record);
        }
        else
        {
            Require(it != rows.end(), VaultError::not_found);
            if (op == 1) it->last_used_at = time;
            else rows.erase(it);
        }
        Commit(c, rows);
        return {VaultError::none, c.abandoned};
    });
}
VaultStatus VaultStore::AddCredential(const CredentialRecord& r) { return Mutate(0, &r, r.credential_id, 0); }
VaultStatus VaultStore::UpdateLastUsed(const std::vector<BYTE>& id, std::uint64_t t) { return Mutate(1, nullptr, id, t); }
VaultStatus VaultStore::DeleteCredential(const std::vector<BYTE>& id) { return Mutate(2, nullptr, id, 0); }
VaultResult<std::vector<CredentialRecord>> VaultStore::ListCredentials()
{
    return ReadResult([&]() -> VaultResult<std::vector<CredentialRecord>>
    {
        Context c(path_);
        auto snapshot = Load(c);
        for (auto& r : snapshot.records) Metadata(r);
        return {VaultError::none, std::move(snapshot.records), c.abandoned};
    });
}
VaultResult<std::vector<CredentialRecord>> VaultStore::FindCredentialsForRp(const std::wstring& rp)
{
    auto result = ListCredentials();
    if (result)
        std::erase_if(*result.value, [&](const auto& r) { return r.rp_id != rp; });
    return result;
}
VaultResult<CredentialRecord> VaultStore::GetCredentialById(const std::wstring& rp, const std::vector<BYTE>& id)
{
    auto result = ListCredentials();
    if (!result) return {result.error, std::nullopt, result.recovered_abandoned_mutex};
    for (const auto& r : *result.value)
        if (r.rp_id == rp && r.credential_id == id)
            return {VaultError::none, r, result.recovered_abandoned_mutex};
    return {VaultError::not_found, std::nullopt, result.recovered_abandoned_mutex};
}
}
