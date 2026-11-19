#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <vector>
namespace aedae
{
struct CredentialRecord
{
    std::wstring id, rp_id, account_label;
    std::vector<std::uint8_t> user_handle, credential_id, public_key_cose;
    // Synthetic, non-secret bytes only in T-005.
    std::vector<std::uint8_t> protected_private_key;
    std::uint64_t created_at{}, last_used_at{};
    std::wstring user_verification_policy, origin_source, backup_status;
    std::uint32_t schema_version{3};
};
enum class VaultError
{
    none, io, access_denied, acl_policy, corrupt, unsupported_schema,
    duplicate, not_found, migration_failed, timeout, read_only, resource_limit
};
template<class T> struct VaultResult
{
    VaultError error{VaultError::none};
    std::optional<T> value;
    bool recovered_abandoned_mutex{};
    explicit operator bool() const { return error == VaultError::none && value.has_value(); }
};
struct VaultStatus
{
    VaultError error{VaultError::none};
    bool recovered_abandoned_mutex{};
    explicit operator bool() const { return error == VaultError::none; }
};
// Immutable instances support concurrent calls; every call returns its own error.
// credential_id and id are globally unique. Enumeration clears opaque blob fields.
class IVaultStore
{
public:
    virtual ~IVaultStore() = default;
    virtual VaultStatus AddCredential(const CredentialRecord&) = 0;
    virtual VaultResult<CredentialRecord> GetCredentialById(const std::wstring&, const std::vector<std::uint8_t>&) = 0;
    virtual VaultResult<std::vector<CredentialRecord>> FindCredentialsForRp(const std::wstring&) = 0;
    virtual VaultResult<std::vector<CredentialRecord>> ListCredentials() = 0;
    virtual VaultStatus UpdateLastUsed(const std::vector<std::uint8_t>&, std::uint64_t) = 0;
    virtual VaultStatus DeleteCredential(const std::vector<std::uint8_t>&) = 0;
};
}
