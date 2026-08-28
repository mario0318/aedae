#include <windows.h>
#include <unknwn.h>
#include <iostream>
#include "../../src/Common/Identifiers.h"

using DllGetClassObjectFn = HRESULT(STDAPICALLTYPE*)(REFCLSID, REFIID, void**);
using DllCanUnloadNowFn = HRESULT(STDAPICALLTYPE*)();

int wmain(int argc, wchar_t** argv)
{
    const wchar_t* dll_path = argc == 2 ? argv[1] : L"aeDaePlugin.dll";
    HMODULE module = LoadLibraryW(dll_path);
    if (module == nullptr) return 1;
    auto get_class_object = reinterpret_cast<DllGetClassObjectFn>(GetProcAddress(module, "DllGetClassObject"));
    auto can_unload_now = reinterpret_cast<DllCanUnloadNowFn>(GetProcAddress(module, "DllCanUnloadNow"));
    if (get_class_object == nullptr || can_unload_now == nullptr) return 2;
    if (can_unload_now() != S_OK) return 3;
    IClassFactory* factory = nullptr;
    if (FAILED(get_class_object(aedae::kPluginClsid, IID_IClassFactory, reinterpret_cast<void**>(&factory)))) return 4;
    if (can_unload_now() != S_FALSE) return 5;
    factory->Release();
    if (can_unload_now() != S_OK) return 6;

    if (FAILED(get_class_object(aedae::kPluginClsid, IID_IClassFactory, reinterpret_cast<void**>(&factory)))) return 7;
    if (FAILED(factory->LockServer(TRUE))) return 8;
    factory->Release();
    if (can_unload_now() != S_FALSE) return 8;
    if (FAILED(get_class_object(aedae::kPluginClsid, IID_IClassFactory, reinterpret_cast<void**>(&factory)))) return 9;
    if (FAILED(factory->LockServer(FALSE))) return 10;
    factory->Release();
    if (can_unload_now() != S_OK) return 11;

    if (FAILED(get_class_object(aedae::kPluginClsid, IID_IClassFactory, reinterpret_cast<void**>(&factory)))) return 12;
    IUnknown* instance = nullptr;
    const HRESULT result = factory->CreateInstance(nullptr, IID_IUnknown, reinterpret_cast<void**>(&instance));
    if (FAILED(result) || instance == nullptr) return 13;
    factory->Release();
    if (can_unload_now() != S_FALSE) return 14;
    instance->Release();
    if (can_unload_now() != S_OK) return 15;
    FreeLibrary(module);
    std::wcout << L"COM activation harness passed\n";
    return 0;
}
