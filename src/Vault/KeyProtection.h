#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <utility>
#include <vector>

namespace aedae
{
enum class KeyProtectionPurpose : std::uint32_t
{
    credential_private_key = 1
};

enum class KeyProtectionError
{
    none,
    invalid_argument,
    resource_limit,
    malformed_blob,
    authentication_failed,
    platform_failure
};

class SensitiveBuffer
{
public:
    SensitiveBuffer(const SensitiveBuffer&) = delete;
    SensitiveBuffer& operator=(const SensitiveBuffer&) = delete;
    SensitiveBuffer(SensitiveBuffer&& other) noexcept;
    SensitiveBuffer& operator=(SensitiveBuffer&& other) noexcept;
    ~SensitiveBuffer();
    std::span<const std::uint8_t> bytes() const noexcept { return bytes_; }

private:
    friend class DpapiKeyProtection;
    explicit SensitiveBuffer(std::vector<std::uint8_t>&& bytes) noexcept : bytes_(std::move(bytes)) {}
    std::vector<std::uint8_t> bytes_;
};

template<class T> struct KeyProtectionResult
{
    KeyProtectionError error{KeyProtectionError::none};
    std::optional<T> value;
    explicit operator bool() const noexcept
    {
        return error == KeyProtectionError::none && value.has_value();
    }
};

using ProtectedKeyResult = KeyProtectionResult<std::vector<std::uint8_t>>;
using UnprotectedKeyResult = KeyProtectionResult<SensitiveBuffer>;

// Stateless current-user protection for credential-private-key material. The purpose is domain
// separation only; it is not proof of Windows Hello verification or authorization to sign.
// A successful unprotect returns move-only transient plaintext that is zeroized on destruction.
class IKeyProtection
{
public:
    virtual ~IKeyProtection() = default;
    virtual ProtectedKeyResult ProtectPrivateKey(
        std::span<const std::uint8_t> plaintext) const noexcept = 0;
    virtual UnprotectedKeyResult UnprotectPrivateKey(
        std::span<const std::uint8_t> protectedBlob,
        KeyProtectionPurpose purpose) const noexcept = 0;
};

class DpapiKeyProtection final : public IKeyProtection
{
public:
    ProtectedKeyResult ProtectPrivateKey(
        std::span<const std::uint8_t> plaintext) const noexcept override;
    UnprotectedKeyResult UnprotectPrivateKey(
        std::span<const std::uint8_t> protectedBlob,
        KeyProtectionPurpose purpose) const noexcept override;
};
}
