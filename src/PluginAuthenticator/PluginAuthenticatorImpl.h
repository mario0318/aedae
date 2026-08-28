#pragma once

#include <unknwn.h>

namespace aedae
{
// Bootstrap-only COM class. It is not the Windows IPluginAuthenticator contract.
class PluginAuthenticator final : public IUnknown
{
public:
    PluginAuthenticator();
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override;
    ULONG STDMETHODCALLTYPE AddRef() override;
    ULONG STDMETHODCALLTYPE Release() override;

private:
    volatile LONG reference_count_{1};
    ~PluginAuthenticator();
};
}
