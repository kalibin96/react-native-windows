// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

// WARNING: Please don't edit this file. It was generated by C++/WinRT v2.0.200615.7

#include "pch.h"
#include "winrt/base.h"
void* winrt_make_Microsoft_Internal_TestController();
void* winrt_make_Microsoft_ReactNative_CompositionRootView();
#ifdef USE_WINUI3
void *winrt_make_Microsoft_ReactNative_Composition_MicrosoftCompositionContextHelper();
#endif
void *winrt_make_Microsoft_ReactNative_Composition_WindowsCompositionContextHelper();
void *winrt_make_Microsoft_ReactNative_Composition_CompositionUIService();
void* winrt_make_Microsoft_ReactNative_JsiRuntime();
void* winrt_make_Microsoft_ReactNative_ReactCoreInjection();
void* winrt_make_Microsoft_ReactNative_ReactDispatcherHelper();
void* winrt_make_Microsoft_ReactNative_ReactInstanceSettings();
void* winrt_make_Microsoft_ReactNative_ReactNativeHost();
void* winrt_make_Microsoft_ReactNative_ReactNotificationServiceHelper();
void* winrt_make_Microsoft_ReactNative_ReactPropertyBagHelper();
void* winrt_make_Microsoft_ReactNative_ReactViewOptions();
void* winrt_make_Microsoft_ReactNative_RedBoxHelper();
void* winrt_make_facebook_react_NativeLogEventSource();
void* winrt_make_facebook_react_NativeTraceEventSource();

bool __stdcall winrt_can_unload_now() noexcept
{
    if (winrt::get_module_lock())
    {
        return false;
    }

    winrt::clear_factory_cache();
    return true;
}

void* __stdcall winrt_get_activation_factory([[maybe_unused]] std::wstring_view const& name)
{
    auto requal = [](std::wstring_view const& left, std::wstring_view const& right) noexcept
    {
        return std::equal(left.rbegin(), left.rend(), right.rbegin(), right.rend());
    };

    if (requal(name, L"Microsoft.Internal.TestController"))
    {
        return winrt_make_Microsoft_Internal_TestController();
    }
    if (requal(name, L"Microsoft.ReactNative.CompositionRootView")) {
      return winrt_make_Microsoft_ReactNative_CompositionRootView();
    }
#ifdef USE_WINUI3
    if (requal(name, L"Microsoft.ReactNative.Composition.MicrosoftCompositionContextHelper")) {
      return winrt_make_Microsoft_ReactNative_Composition_MicrosoftCompositionContextHelper();
    }
#endif
    if (requal(name, L"Microsoft.ReactNative.Composition.WindowsCompositionContextHelper")) {
      return winrt_make_Microsoft_ReactNative_Composition_WindowsCompositionContextHelper();
    }
    if (requal(name, L"Microsoft.ReactNative.Composition.CompositionUIService")) {
      return winrt_make_Microsoft_ReactNative_Composition_CompositionUIService();
    }
    if (requal(name, L"Microsoft.ReactNative.JsiRuntime"))
    {
        return winrt_make_Microsoft_ReactNative_JsiRuntime();
    }

    if (requal(name, L"Microsoft.ReactNative.ReactCoreInjection"))
    {
        return winrt_make_Microsoft_ReactNative_ReactCoreInjection();
    }

    if (requal(name, L"Microsoft.ReactNative.ReactDispatcherHelper"))
    {
        return winrt_make_Microsoft_ReactNative_ReactDispatcherHelper();
    }

    if (requal(name, L"Microsoft.ReactNative.ReactInstanceSettings"))
    {
        return winrt_make_Microsoft_ReactNative_ReactInstanceSettings();
    }

    if (requal(name, L"Microsoft.ReactNative.ReactNativeHost"))
    {
        return winrt_make_Microsoft_ReactNative_ReactNativeHost();
    }

    if (requal(name, L"Microsoft.ReactNative.ReactNotificationServiceHelper"))
    {
        return winrt_make_Microsoft_ReactNative_ReactNotificationServiceHelper();
    }

    if (requal(name, L"Microsoft.ReactNative.ReactPropertyBagHelper"))
    {
        return winrt_make_Microsoft_ReactNative_ReactPropertyBagHelper();
    }

    if (requal(name, L"Microsoft.ReactNative.ReactViewOptions"))
    {
        return winrt_make_Microsoft_ReactNative_ReactViewOptions();
    }

    if (requal(name, L"Microsoft.ReactNative.RedBoxHelper"))
    {
        return winrt_make_Microsoft_ReactNative_RedBoxHelper();
    }

    if (requal(name, L"facebook.react.NativeLogEventSource"))
    {
        return winrt_make_facebook_react_NativeLogEventSource();
    }

    if (requal(name, L"facebook.react.NativeTraceEventSource"))
    {
        return winrt_make_facebook_react_NativeTraceEventSource();
    }

    return nullptr;
}

int32_t __stdcall WINRT_CanUnloadNow() noexcept
{
#ifdef _WRL_MODULE_H_
    if (!::Microsoft::WRL::Module<::Microsoft::WRL::InProc>::GetModule().Terminate())
    {
        return 1;
    }
#endif

    return winrt_can_unload_now() ? 0 : 1;
}

int32_t __stdcall WINRT_GetActivationFactory(void* classId, void** factory) noexcept try
{
    std::wstring_view const name{ *reinterpret_cast<winrt::hstring*>(&classId) };
    *factory = winrt_get_activation_factory(name);

    if (*factory)
    {
        return 0;
    }

#ifdef _WRL_MODULE_H_
    return ::Microsoft::WRL::Module<::Microsoft::WRL::InProc>::GetModule().GetActivationFactory(static_cast<HSTRING>(classId), reinterpret_cast<::IActivationFactory**>(factory));
#else
    return winrt::hresult_class_not_available(name).to_abi();
#endif
}
catch (...) { return winrt::to_hresult(); }
