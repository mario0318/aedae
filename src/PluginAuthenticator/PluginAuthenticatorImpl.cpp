#include "PluginAuthenticatorImpl.h"
#include "ModuleLifetime.h"

namespace aedae
{
PluginAuthenticator::PluginAuthenticator()
{
    AddModuleObjectRef();
}

PluginAuthenticator::~PluginAuthenticator()
{
    ReleaseModuleObjectRef();
}

HRESULT STDMETHODCALLTYPE PluginAuthenticator::QueryInterface(REFIID riid, void** object)
{
    if (object == nullptr) return E_POINTER;
    *object = nullptr;
    if (riid == IID_IUnknown)
    {
        *object = static_cast<IUnknown*>(this);
        AddRef();
        return S_OK;
    }
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE PluginAuthenticator::AddRef()
{
    return static_cast<ULONG>(InterlockedIncrement(&reference_count_));
}

ULONG STDMETHODCALLTYPE PluginAuthenticator::Release()
{
    const auto remaining = static_cast<ULONG>(InterlockedDecrement(&reference_count_));
    if (remaining == 0) delete this;
    return remaining;
}
}
