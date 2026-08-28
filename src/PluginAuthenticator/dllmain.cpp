#include <windows.h>
#include <unknwn.h>
#include <new>
#include "../Common/Identifiers.h"
#include "PluginAuthenticatorImpl.h"
#include "ModuleLifetime.h"

namespace
{
class PluginClassFactory final : public IClassFactory
{
public:
    PluginClassFactory()
    {
        aedae::AddModuleObjectRef();
    }
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** object) override
    {
        if (object == nullptr) return E_POINTER;
        *object = nullptr;
        if (riid == IID_IUnknown || riid == IID_IClassFactory)
        {
            *object = static_cast<IClassFactory*>(this);
            AddRef();
            return S_OK;
        }
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return static_cast<ULONG>(InterlockedIncrement(&references_)); }
    ULONG STDMETHODCALLTYPE Release() override
    {
        const auto remaining = static_cast<ULONG>(InterlockedDecrement(&references_));
        if (remaining == 0) delete this;
        return remaining;
    }
    HRESULT STDMETHODCALLTYPE CreateInstance(IUnknown* outer, REFIID riid, void** object) override
    {
        if (outer != nullptr) return CLASS_E_NOAGGREGATION;
        auto authenticator = new (std::nothrow) aedae::PluginAuthenticator();
        if (authenticator == nullptr) return E_OUTOFMEMORY;
        const HRESULT result = authenticator->QueryInterface(riid, object);
        authenticator->Release();
        return result;
    }
    HRESULT STDMETHODCALLTYPE LockServer(BOOL lock) override
    {
        if (lock)
        {
            aedae::AddServerLock();
        }
        else
        {
            if (!aedae::ReleaseServerLock()) return E_UNEXPECTED;
        }
        return S_OK;
    }
private:
    volatile LONG references_{1};
    ~PluginClassFactory()
    {
        aedae::ReleaseModuleObjectRef();
    }
};
}

BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }

STDAPI DllGetClassObject(REFCLSID clsid, REFIID riid, void** object)
{
    if (clsid != aedae::kPluginClsid) return CLASS_E_CLASSNOTAVAILABLE;
    auto factory = new (std::nothrow) PluginClassFactory();
    if (factory == nullptr) return E_OUTOFMEMORY;
    const HRESULT result = factory->QueryInterface(riid, object);
    factory->Release();
    return result;
}

STDAPI DllCanUnloadNow()
{
    return aedae::CanUnloadModule() ? S_OK : S_FALSE;
}
