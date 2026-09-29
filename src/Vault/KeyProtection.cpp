#include "KeyProtection.h"

#include <windows.h>
#include <bcrypt.h>
#include <wincrypt.h>
#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <new>

#pragma comment(lib, "Bcrypt.lib")
#pragma comment(lib, "Crypt32.lib")

namespace aedae
{
namespace
{
constexpr std::array<std::uint8_t, 8> kMagic{'A', 'E', 'D', 'K', 'E', 'Y', '0', '1'};
constexpr std::uint32_t kVersion = 1;
constexpr std::uint32_t kAlgorithmAes256GcmDpapi = 1;
constexpr std::size_t kHeaderBytes = 32;
constexpr std::size_t kKeyBytes = 32;
constexpr std::size_t kNonceBytes = 12;
constexpr std::size_t kTagBytes = 16;
constexpr std::size_t kMaxPlaintextBytes = 1024 * 1024;
constexpr std::size_t kMaxProtectedBytes = 2 * 1024 * 1024;
constexpr std::array<std::uint8_t, 46> kPurposeEntropy{
    'a','e','D','a','e','/','k','e','y','-','p','r','o','t','e','c','t','i','o','n','/','v','1','/',
    'c','r','e','d','e','n','t','i','a','l','-','p','r','i','v','a','t','e','-','k','e','y'};

void Zero(std::span<std::uint8_t> bytes) noexcept
{
    if (!bytes.empty()) SecureZeroMemory(bytes.data(), bytes.size());
}

struct SecureBytes
{
    std::vector<std::uint8_t> value;
    explicit SecureBytes(std::size_t size) : value(size) {}
    ~SecureBytes() { Zero(value); }
    SecureBytes(const SecureBytes&) = delete;
    SecureBytes& operator=(const SecureBytes&) = delete;
};

struct Algorithm
{
    BCRYPT_ALG_HANDLE value{};
    ~Algorithm() { if (value) BCryptCloseAlgorithmProvider(value, 0); }
};

struct Key
{
    BCRYPT_KEY_HANDLE value{};
    ~Key() { if (value) BCryptDestroyKey(value); }
};

struct DpapiMemory
{
    BYTE* value{};
    DWORD size{};
    ~DpapiMemory()
    {
        if (value)
        {
            SecureZeroMemory(value, size);
            LocalFree(value);
        }
    }
};

bool NtOk(NTSTATUS status) noexcept { return status >= 0; }

void AppendU32(std::vector<std::uint8_t>& output, std::uint32_t value)
{
    output.push_back(static_cast<std::uint8_t>(value));
    output.push_back(static_cast<std::uint8_t>(value >> 8));
    output.push_back(static_cast<std::uint8_t>(value >> 16));
    output.push_back(static_cast<std::uint8_t>(value >> 24));
}

bool ReadU32(std::span<const std::uint8_t> bytes, std::size_t offset, std::uint32_t& value) noexcept
{
    if (offset > bytes.size() || bytes.size() - offset < sizeof(std::uint32_t)) return false;
    value = static_cast<std::uint32_t>(bytes[offset]) |
        (static_cast<std::uint32_t>(bytes[offset + 1]) << 8) |
        (static_cast<std::uint32_t>(bytes[offset + 2]) << 16) |
        (static_cast<std::uint32_t>(bytes[offset + 3]) << 24);
    return true;
}

std::vector<std::uint8_t> Header(std::uint32_t wrappedBytes, std::uint32_t cipherBytes)
{
    std::vector<std::uint8_t> header;
    header.reserve(kHeaderBytes);
    header.insert(header.end(), kMagic.begin(), kMagic.end());
    AppendU32(header, kVersion);
    AppendU32(header, kAlgorithmAes256GcmDpapi);
    AppendU32(header, wrappedBytes);
    AppendU32(header, static_cast<std::uint32_t>(kNonceBytes));
    AppendU32(header, static_cast<std::uint32_t>(kTagBytes));
    AppendU32(header, cipherBytes);
    return header;
}

DATA_BLOB PurposeEntropy() noexcept
{
    return {static_cast<DWORD>(kPurposeEntropy.size()),
        const_cast<BYTE*>(reinterpret_cast<const BYTE*>(kPurposeEntropy.data()))};
}

bool OpenAes(Algorithm& algorithm) noexcept
{
    if (!NtOk(BCryptOpenAlgorithmProvider(&algorithm.value, BCRYPT_AES_ALGORITHM, nullptr, 0)))
        return false;
    return NtOk(BCryptSetProperty(algorithm.value, BCRYPT_CHAINING_MODE,
        reinterpret_cast<PUCHAR>(const_cast<wchar_t*>(BCRYPT_CHAIN_MODE_GCM)),
        sizeof(BCRYPT_CHAIN_MODE_GCM), 0));
}

bool SupportsTag(BCRYPT_ALG_HANDLE algorithm) noexcept
{
    BCRYPT_AUTH_TAG_LENGTHS_STRUCT lengths{};
    ULONG actual{};
    if (!NtOk(BCryptGetProperty(algorithm, BCRYPT_AUTH_TAG_LENGTH,
        reinterpret_cast<PUCHAR>(&lengths), sizeof(lengths), &actual, 0)) || actual != sizeof(lengths))
        return false;
    if (kTagBytes < lengths.dwMinLength || kTagBytes > lengths.dwMaxLength) return false;
    return lengths.dwIncrement == 0 ||
        ((static_cast<ULONG>(kTagBytes) - lengths.dwMinLength) % lengths.dwIncrement) == 0;
}

bool MakeKey(BCRYPT_ALG_HANDLE algorithm, std::span<const std::uint8_t> raw,
    SecureBytes& keyObject, Key& key) noexcept
{
    ULONG objectBytes{}, actual{};
    if (!NtOk(BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH,
        reinterpret_cast<PUCHAR>(&objectBytes), sizeof(objectBytes), &actual, 0)) ||
        actual != sizeof(objectBytes) || objectBytes == 0)
        return false;
    try { keyObject.value.resize(objectBytes); }
    catch (...) { return false; }
    return NtOk(BCryptGenerateSymmetricKey(algorithm, &key.value, keyObject.value.data(),
        objectBytes, const_cast<PUCHAR>(raw.data()), static_cast<ULONG>(raw.size()), 0));
}

std::vector<std::uint8_t> AuthenticatedData(std::span<const std::uint8_t> header,
    std::span<const std::uint8_t> wrappedKey)
{
    std::vector<std::uint8_t> aad;
    aad.reserve(header.size() + wrappedKey.size() + kPurposeEntropy.size());
    aad.insert(aad.end(), header.begin(), header.end());
    aad.insert(aad.end(), wrappedKey.begin(), wrappedKey.end());
    aad.insert(aad.end(), kPurposeEntropy.begin(), kPurposeEntropy.end());
    return aad;
}

template<class T> KeyProtectionResult<T> Error(KeyProtectionError error) noexcept
{
    return {error, std::nullopt};
}
}

SensitiveBuffer::SensitiveBuffer(SensitiveBuffer&& other) noexcept : bytes_(std::move(other.bytes_)) {}

SensitiveBuffer& SensitiveBuffer::operator=(SensitiveBuffer&& other) noexcept
{
    if (this != &other)
    {
        Zero(bytes_);
        bytes_ = std::move(other.bytes_);
    }
    return *this;
}

SensitiveBuffer::~SensitiveBuffer() { Zero(bytes_); }

ProtectedKeyResult DpapiKeyProtection::ProtectPrivateKey(
    std::span<const std::uint8_t> plaintext) const noexcept
{
    if (plaintext.empty()) return Error<std::vector<std::uint8_t>>(KeyProtectionError::invalid_argument);
    if (plaintext.size() > kMaxPlaintextBytes ||
        plaintext.size() > (std::numeric_limits<ULONG>::max)())
        return Error<std::vector<std::uint8_t>>(KeyProtectionError::resource_limit);

    try
    {
        SecureBytes dataKey(kKeyBytes);
        std::array<std::uint8_t, kNonceBytes> nonce{};
        if (!NtOk(BCryptGenRandom(nullptr, dataKey.value.data(),
                static_cast<ULONG>(dataKey.value.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG)) ||
            !NtOk(BCryptGenRandom(nullptr, nonce.data(), static_cast<ULONG>(nonce.size()),
                BCRYPT_USE_SYSTEM_PREFERRED_RNG)))
            return Error<std::vector<std::uint8_t>>(KeyProtectionError::platform_failure);

        DATA_BLOB input{static_cast<DWORD>(dataKey.value.size()), dataKey.value.data()};
        DATA_BLOB entropy = PurposeEntropy();
        DATA_BLOB wrapped{};
        if (!CryptProtectData(&input, L"aeDae credential private key v1", &entropy, nullptr,
            nullptr, CRYPTPROTECT_UI_FORBIDDEN, &wrapped))
            return Error<std::vector<std::uint8_t>>(KeyProtectionError::platform_failure);
        DpapiMemory wrappedOwner{wrapped.pbData, wrapped.cbData};
        if (wrapped.cbData == 0 || wrapped.cbData > kMaxProtectedBytes)
            return Error<std::vector<std::uint8_t>>(KeyProtectionError::platform_failure);

        const auto header = Header(wrapped.cbData, static_cast<std::uint32_t>(plaintext.size()));
        auto aad = AuthenticatedData(header,
            std::span<const std::uint8_t>(wrapped.pbData, wrapped.cbData));
        std::array<std::uint8_t, kTagBytes> tag{};
        SecureBytes ciphertext(plaintext.size());

        Algorithm algorithm;
        if (!OpenAes(algorithm) || !SupportsTag(algorithm.value))
            return Error<std::vector<std::uint8_t>>(KeyProtectionError::platform_failure);
        SecureBytes keyObject(0);
        Key key;
        if (!MakeKey(algorithm.value, dataKey.value, keyObject, key))
            return Error<std::vector<std::uint8_t>>(KeyProtectionError::platform_failure);

        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO auth;
        BCRYPT_INIT_AUTH_MODE_INFO(auth);
        auth.pbNonce = nonce.data(); auth.cbNonce = static_cast<ULONG>(nonce.size());
        auth.pbAuthData = aad.data(); auth.cbAuthData = static_cast<ULONG>(aad.size());
        auth.pbTag = tag.data(); auth.cbTag = static_cast<ULONG>(tag.size());
        ULONG written{};
        if (!NtOk(BCryptEncrypt(key.value, const_cast<PUCHAR>(plaintext.data()),
            static_cast<ULONG>(plaintext.size()), &auth, nullptr, 0, ciphertext.value.data(),
            static_cast<ULONG>(ciphertext.value.size()), &written, 0)) || written != ciphertext.value.size())
            return Error<std::vector<std::uint8_t>>(KeyProtectionError::platform_failure);

        std::vector<std::uint8_t> output;
        const std::size_t total = header.size() + wrapped.cbData + nonce.size() + tag.size() + ciphertext.value.size();
        if (total > kMaxProtectedBytes) return Error<std::vector<std::uint8_t>>(KeyProtectionError::resource_limit);
        output.reserve(total);
        output.insert(output.end(), header.begin(), header.end());
        output.insert(output.end(), wrapped.pbData, wrapped.pbData + wrapped.cbData);
        output.insert(output.end(), nonce.begin(), nonce.end());
        output.insert(output.end(), tag.begin(), tag.end());
        output.insert(output.end(), ciphertext.value.begin(), ciphertext.value.end());
        return {KeyProtectionError::none, std::move(output)};
    }
    catch (const std::bad_alloc&) { return Error<std::vector<std::uint8_t>>(KeyProtectionError::resource_limit); }
    catch (...) { return Error<std::vector<std::uint8_t>>(KeyProtectionError::platform_failure); }
}

UnprotectedKeyResult DpapiKeyProtection::UnprotectPrivateKey(
    std::span<const std::uint8_t> protectedBlob, KeyProtectionPurpose purpose) const noexcept
{
    if (purpose != KeyProtectionPurpose::credential_private_key)
        return Error<SensitiveBuffer>(KeyProtectionError::authentication_failed);
    if (protectedBlob.size() > kMaxProtectedBytes)
        return Error<SensitiveBuffer>(KeyProtectionError::resource_limit);
    if (protectedBlob.size() < kHeaderBytes)
        return Error<SensitiveBuffer>(KeyProtectionError::malformed_blob);

    try
    {
        if (!std::equal(kMagic.begin(), kMagic.end(), protectedBlob.begin()))
            return Error<SensitiveBuffer>(KeyProtectionError::malformed_blob);
        std::uint32_t version{}, algorithmId{}, wrappedBytes{}, nonceBytes{}, tagBytes{}, cipherBytes{};
        if (!ReadU32(protectedBlob, 8, version) || !ReadU32(protectedBlob, 12, algorithmId) ||
            !ReadU32(protectedBlob, 16, wrappedBytes) || !ReadU32(protectedBlob, 20, nonceBytes) ||
            !ReadU32(protectedBlob, 24, tagBytes) || !ReadU32(protectedBlob, 28, cipherBytes) ||
            version != kVersion || algorithmId != kAlgorithmAes256GcmDpapi || wrappedBytes == 0 ||
            nonceBytes != kNonceBytes || tagBytes != kTagBytes || cipherBytes == 0 ||
            cipherBytes > kMaxPlaintextBytes)
            return Error<SensitiveBuffer>(KeyProtectionError::malformed_blob);
        const std::uint64_t total = static_cast<std::uint64_t>(kHeaderBytes) + wrappedBytes +
            nonceBytes + tagBytes + cipherBytes;
        if (total != protectedBlob.size()) return Error<SensitiveBuffer>(KeyProtectionError::malformed_blob);

        std::size_t at = kHeaderBytes;
        DATA_BLOB wrapped{wrappedBytes, const_cast<BYTE*>(protectedBlob.data() + at)};
        at += wrappedBytes;
        auto nonce = protectedBlob.subspan(at, nonceBytes); at += nonceBytes;
        auto tag = protectedBlob.subspan(at, tagBytes); at += tagBytes;
        auto ciphertext = protectedBlob.subspan(at, cipherBytes);

        DATA_BLOB entropy = PurposeEntropy();
        DATA_BLOB clearKey{};
        if (!CryptUnprotectData(&wrapped, nullptr, &entropy, nullptr, nullptr,
            CRYPTPROTECT_UI_FORBIDDEN, &clearKey))
            return Error<SensitiveBuffer>(KeyProtectionError::authentication_failed);
        DpapiMemory clearOwner{clearKey.pbData, clearKey.cbData};
        if (clearKey.cbData != kKeyBytes)
            return Error<SensitiveBuffer>(KeyProtectionError::authentication_failed);

        Algorithm algorithm;
        if (!OpenAes(algorithm) || !SupportsTag(algorithm.value))
            return Error<SensitiveBuffer>(KeyProtectionError::platform_failure);
        SecureBytes keyObject(0);
        Key key;
        if (!MakeKey(algorithm.value,
            std::span<const std::uint8_t>(clearKey.pbData, clearKey.cbData), keyObject, key))
            return Error<SensitiveBuffer>(KeyProtectionError::platform_failure);

        auto header = protectedBlob.first(kHeaderBytes);
        auto aad = AuthenticatedData(header, protectedBlob.subspan(kHeaderBytes, wrappedBytes));
        SecureBytes plaintext(cipherBytes);
        BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO auth;
        BCRYPT_INIT_AUTH_MODE_INFO(auth);
        auth.pbNonce = const_cast<PUCHAR>(nonce.data()); auth.cbNonce = nonceBytes;
        auth.pbAuthData = aad.data(); auth.cbAuthData = static_cast<ULONG>(aad.size());
        auth.pbTag = const_cast<PUCHAR>(tag.data()); auth.cbTag = tagBytes;
        ULONG written{};
        if (!NtOk(BCryptDecrypt(key.value, const_cast<PUCHAR>(ciphertext.data()), cipherBytes,
            &auth, nullptr, 0, plaintext.value.data(), static_cast<ULONG>(plaintext.value.size()),
            &written, 0)) || written != plaintext.value.size())
            return Error<SensitiveBuffer>(KeyProtectionError::authentication_failed);

        return {KeyProtectionError::none, SensitiveBuffer(std::move(plaintext.value))};
    }
    catch (const std::bad_alloc&) { return Error<SensitiveBuffer>(KeyProtectionError::resource_limit); }
    catch (...) { return Error<SensitiveBuffer>(KeyProtectionError::platform_failure); }
}
}
