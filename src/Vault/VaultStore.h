#pragma once
#include "IVaultStore.h"
namespace aedae
{
enum class VaultAccess { snapshot, writer };
// Writer mode is a service convention, not a boundary against same-user malware.
// Provisioning/recovery are explicit writer actions. Snapshot calls never write.
class VaultStore final : public IVaultStore
{
public:
    explicit VaultStore(std::wstring path, VaultAccess access = VaultAccess::snapshot);
    VaultStatus Provision();
    VaultStatus Recover();
    VaultStatus Migrate();
    VaultStatus AddCredential(const CredentialRecord&) override;
    VaultResult<CredentialRecord> GetCredentialById(const std::wstring&, const std::vector<std::uint8_t>&) override;
    VaultResult<std::vector<CredentialRecord>> FindCredentialsForRp(const std::wstring&) override;
    VaultResult<std::vector<CredentialRecord>> ListCredentials() override;
    VaultStatus UpdateLastUsed(const std::vector<std::uint8_t>&, std::uint64_t) override;
    VaultStatus DeleteCredential(const std::vector<std::uint8_t>&) override;
private:
    const std::wstring path_;
    const VaultAccess access_;
    VaultStatus Mutate(int, const CredentialRecord*, const std::vector<std::uint8_t>&, std::uint64_t);
};
}
