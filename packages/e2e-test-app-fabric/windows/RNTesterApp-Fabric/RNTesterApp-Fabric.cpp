// RNTesterApp-Fabric.cpp : Defines the entry point for the application.
//

#include "pch.h"
#include "RNTesterApp-Fabric.h"

#include <UIAutomation.h>
#include <winrt/Microsoft.ReactNative.Composition.h>
#include <winrt/Microsoft.ReactNative.h>
#include <winrt/Microsoft.UI.Composition.h>
#include <winrt/Microsoft.UI.Content.h>
#include <winrt/Microsoft.UI.Dispatching.h>
#include <winrt/Microsoft.UI.Windowing.h>
#include <winrt/Microsoft.UI.interop.h>
#include <winrt/Windows.Data.Json.h>
#include <winrt/Windows.Foundation.h>
#include "winrt/AutomationChannel.h"

#include <NativeModules.h>

struct RNTesterAppReactPackageProvider
    : winrt::implements<RNTesterAppReactPackageProvider, winrt::Microsoft::ReactNative::IReactPackageProvider> {
 public: // IReactPackageProvider
  void CreatePackage(winrt::Microsoft::ReactNative::IReactPackageBuilder const &packageBuilder) noexcept {
    AddAttributedModules(packageBuilder, true);
  }
};

constexpr PCWSTR appName = L"RNTesterApp";

// Keep track of errors and warnings to be able to report them to automation
std::vector<std::string> g_Errors;
std::vector<std::string> g_Warnings;
HWND global_hwnd;
winrt::Microsoft::ReactNative::CompositionRootView *global_rootView{nullptr};

// Forward declarations of functions included in this code module:
winrt::Windows::Data::Json::JsonObject ListErrors(winrt::Windows::Data::Json::JsonValue payload);
winrt::Windows::Data::Json::JsonObject DumpVisualTree(winrt::Windows::Data::Json::JsonValue payload);
winrt::Windows::Foundation::IAsyncAction LoopServer(winrt::AutomationChannel::Server &server);

float ScaleFactor(HWND hwnd) noexcept {
  return GetDpiForWindow(hwnd) / static_cast<float>(USER_DEFAULT_SCREEN_DPI);
}

void UpdateRootViewSizeToAppWindow(
    winrt::Microsoft::ReactNative::CompositionRootView const &rootView,
    winrt::Microsoft::UI::Windowing::AppWindow const &window) {
  auto hwnd = winrt::Microsoft::UI::GetWindowFromWindowId(window.Id());
  auto scaleFactor = ScaleFactor(hwnd);
  winrt::Windows::Foundation::Size size{
      window.ClientSize().Width / scaleFactor, window.ClientSize().Height / scaleFactor};
  // Do not relayout when minimized
  if (window.Presenter().as<winrt::Microsoft::UI::Windowing::OverlappedPresenter>().State() !=
      winrt::Microsoft::UI::Windowing::OverlappedPresenterState::Minimized) {
    rootView.Arrange(size);
    rootView.Size(size);
  }
}

// Create and configure the ReactNativeHost
winrt::Microsoft::ReactNative::ReactNativeHost CreateReactNativeHost(
    HWND hwnd,
    const winrt::Microsoft::UI::Composition::Compositor &compositor) {
  WCHAR workingDir[MAX_PATH];
  GetCurrentDirectory(MAX_PATH, workingDir);

  auto host = winrt::Microsoft::ReactNative::ReactNativeHost();
  // Disable until we have a 3rd party story for custom components
  // RegisterAutolinkedNativeModulePackages(host.PackageProviders()); // Includes any
  // autolinked modules
  host.PackageProviders().Append(winrt::make<RNTesterAppReactPackageProvider>());
  host.PackageProviders().Append(winrt::AutomationChannel::ReactPackageProvider());

  host.InstanceSettings().JavaScriptBundleFile(L"index.windows");
  host.InstanceSettings().DebugBundlePath(L"index");

  host.InstanceSettings().BundleRootPath(std::wstring(L"file:").append(workingDir).append(L"\\Bundle\\").c_str());
  host.InstanceSettings().DebuggerBreakOnNextLine(false);
#if _DEBUG
  host.InstanceSettings().UseDirectDebugger(true);
  host.InstanceSettings().UseFastRefresh(true);
#endif
  host.InstanceSettings().UseDeveloperSupport(true);

  // Test App hooks into JS console.log implementation to record errors/warnings
  host.InstanceSettings().NativeLogger([](winrt::Microsoft::ReactNative::LogLevel level, winrt::hstring message) {
    if (level == winrt::Microsoft::ReactNative::LogLevel::Error ||
        level == winrt::Microsoft::ReactNative::LogLevel::Fatal) {
      g_Errors.push_back(winrt::to_string(message));
    } else if (level == winrt::Microsoft::ReactNative::LogLevel::Warning) {
      g_Warnings.push_back(winrt::to_string(message));
    }
  });

  winrt::Microsoft::ReactNative::ReactCoreInjection::SetTopLevelWindowId(
      host.InstanceSettings().Properties(), reinterpret_cast<uint64_t>(hwnd));

  // By using the MicrosoftCompositionContextHelper here, React Native Windows will use Lifted Visuals for its
  // tree.
  winrt::Microsoft::ReactNative::Composition::CompositionUIService::SetCompositionContext(
      host.InstanceSettings().Properties(),
      winrt::Microsoft::ReactNative::Composition::MicrosoftCompositionContextHelper::CreateContext(compositor));

  return host;
}

_Use_decl_annotations_ int CALLBACK WinMain(HINSTANCE instance, HINSTANCE, PSTR /* commandLine */, int showCmd) {
  // Initialize WinRT.
  winrt::init_apartment(winrt::apartment_type::single_threaded);

  // Enable per monitor DPI scaling
  SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

  // Create a DispatcherQueue for this thread.  This is needed for Composition, Content, and
  // Input APIs.
  auto dispatcherQueueController{winrt::Microsoft::UI::Dispatching::DispatcherQueueController::CreateOnCurrentThread()};

  // Create a Compositor for all Content on this thread.
  auto compositor{winrt::Microsoft::UI::Composition::Compositor()};

  // Create a top-level window.
  auto window = winrt::Microsoft::UI::Windowing::AppWindow::Create();
  window.Title(appName);
  window.Resize({1000, 1000});
  window.Show();
  auto hwnd = winrt::Microsoft::UI::GetWindowFromWindowId(window.Id());
  global_hwnd = hwnd;
  auto scaleFactor = ScaleFactor(hwnd);

  auto host = CreateReactNativeHost(hwnd, compositor);

  // Start the react-native instance, which will create a JavaScript runtime and load the applications bundle
  host.ReloadInstance();

  // Create a RootView which will present a react-native component
  winrt::Microsoft::ReactNative::ReactViewOptions viewOptions;
  viewOptions.ComponentName(appName);
  auto rootView = winrt::Microsoft::ReactNative::CompositionRootView(compositor);
  rootView.ReactViewHost(winrt::Microsoft::ReactNative::ReactCoreInjection::MakeViewHost(host, viewOptions));

  // Update the size of the RootView when the AppWindow changes size
  window.Changed([wkRootView = winrt::make_weak(rootView)](
                     winrt::Microsoft::UI::Windowing::AppWindow const &window,
                     winrt::Microsoft::UI::Windowing::AppWindowChangedEventArgs const &args) {
    if (args.DidSizeChange() || args.DidVisibilityChange()) {
      if (auto rootView = wkRootView.get()) {
        UpdateRootViewSizeToAppWindow(rootView, window);
      }
    }
  });

  // Quit application when main window is closed
  window.Destroying(
      [host](winrt::Microsoft::UI::Windowing::AppWindow const &window, winrt::IInspectable const & /*args*/) {
        // Before we shutdown the application - unload the ReactNativeHost to give the javascript a chance to save any
        // state
        auto async = host.UnloadInstance();
        async.Completed([host](auto asyncInfo, winrt::Windows::Foundation::AsyncStatus asyncStatus) {
          assert(asyncStatus == winrt::Windows::Foundation::AsyncStatus::Completed);
          host.InstanceSettings().UIDispatcher().Post([]() { PostQuitMessage(0); });
        });
      });

  // DesktopChildSiteBridge create a ContentSite that can host the RootView ContentIsland
  auto bridge = winrt::Microsoft::UI::Content::DesktopChildSiteBridge::Create(compositor, window.Id());
  bridge.Connect(rootView.Island());
  bridge.ResizePolicy(winrt::Microsoft::UI::Content::ContentSizePolicy::ResizeContentToParentWindow);

  auto invScale = 1.0f / scaleFactor;
  rootView.RootVisual().Scale({invScale, invScale, invScale});
  rootView.ScaleFactor(scaleFactor);

  // Set the intialSize of the root view
  UpdateRootViewSizeToAppWindow(rootView, window);

  bridge.Show();

  // Set Up Servers for E2E Testing
  winrt::AutomationChannel::CommandHandler handler;
  handler.BindOperation(L"DumpVisualTree", DumpVisualTree);
  handler.BindOperation(L"ListErrors", ListErrors);
  global_rootView = &rootView;

  auto server = winrt::AutomationChannel::Server(handler);
  auto asyncAction = LoopServer(server);

  // Run the main application event loop
  dispatcherQueueController.DispatcherQueue().RunEventLoop();

  // Rundown the DispatcherQueue. This drains the queue and raises events to let components
  // know the message loop has finished.
  dispatcherQueueController.ShutdownQueue();

  bridge.Close();
  bridge = nullptr;

  // Destroy all Composition objects
  compositor.Close();
  compositor = nullptr;
}

winrt::Windows::Data::Json::JsonObject ListErrors(winrt::Windows::Data::Json::JsonValue payload) {
  winrt::Windows::Data::Json::JsonObject result;
  winrt::Windows::Data::Json::JsonArray jsonErrors;
  winrt::Windows::Data::Json::JsonArray jsonWarnings;

  for (auto &err : g_Errors) {
    jsonErrors.InsertAt(0, winrt::Windows::Data::Json::JsonValue::CreateStringValue(winrt::to_hstring(err)));
  }
  g_Errors.clear();
  for (auto &warn : g_Warnings) {
    jsonWarnings.InsertAt(0, winrt::Windows::Data::Json::JsonValue::CreateStringValue(winrt::to_hstring(warn)));
  }
  g_Warnings.clear();

  result.Insert(L"errors", jsonErrors);
  result.Insert(L"warnings", jsonWarnings);

  return result;
}

winrt::Windows::Data::Json::JsonObject DumpUIATreeRecurse(
    IUIAutomationElement *pTarget,
    IUIAutomationTreeWalker *pWalker) {
  winrt::Windows::Data::Json::JsonObject result;
  BSTR automationId;
  CONTROLTYPEID controlType;
  BSTR helpText;
  BOOL isEnabled;
  BOOL isKeyboardFocusable;
  BSTR localizedControlType;
  BSTR name;

  pTarget->get_CurrentAutomationId(&automationId);
  pTarget->get_CurrentControlType(&controlType);
  pTarget->get_CurrentHelpText(&helpText);
  pTarget->get_CurrentIsEnabled(&isEnabled);
  pTarget->get_CurrentIsKeyboardFocusable(&isKeyboardFocusable);
  pTarget->get_CurrentLocalizedControlType(&localizedControlType);
  pTarget->get_CurrentName(&name);
  result.Insert(L"AutomationId", winrt::Windows::Data::Json::JsonValue::CreateStringValue(automationId));
  result.Insert(L"ControlType", winrt::Windows::Data::Json::JsonValue::CreateNumberValue(controlType));
  result.Insert(L"HelpText", winrt::Windows::Data::Json::JsonValue::CreateStringValue(helpText));
  result.Insert(L"IsEnabled", winrt::Windows::Data::Json::JsonValue::CreateBooleanValue(isEnabled));
  result.Insert(L"IsKeyboardFocusable", winrt::Windows::Data::Json::JsonValue::CreateBooleanValue(isKeyboardFocusable));
  result.Insert(
      L"LocalizedControlType", winrt::Windows::Data::Json::JsonValue::CreateStringValue(localizedControlType));
  result.Insert(L"Name", winrt::Windows::Data::Json::JsonValue::CreateStringValue(name));

  IUIAutomationElement *pChild;
  IUIAutomationElement *pSibling;
  pWalker->GetFirstChildElement(pTarget, &pChild);
  winrt::Windows::Data::Json::JsonArray children;
  while (pChild != nullptr) {
    children.Append(DumpUIATreeRecurse(pChild, pWalker));
    pWalker->GetNextSiblingElement(pChild, &pSibling);
    pChild = pSibling;
    pSibling = nullptr;
  }
  if (children.Size() > 0) {
    result.Insert(L"Children", children);
  }
  return result;
}

winrt::Windows::Data::Json::JsonObject DumpUIATreeHelper(winrt::Windows::Data::Json::JsonObject payloadObj) {
  auto accessibilityId = payloadObj.GetNamedString(L"accessibilityId");

  winrt::Windows::Data::Json::JsonObject result;

  IUIAutomation *pAutomation;
  IUIAutomationElement *pRootElement;
  IUIAutomationTreeWalker *pWalker;

  CoCreateInstance(__uuidof(CUIAutomation8), nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pAutomation));
  pAutomation->get_ContentViewWalker(&pWalker);
  pAutomation->ElementFromHandle(global_hwnd, &pRootElement);

  IUIAutomationElement *pTarget;
  IUIAutomationCondition *pCondition;
  VARIANT varAutomationId;
  VariantInit(&varAutomationId);

  varAutomationId.vt = VT_BSTR;
  varAutomationId.bstrVal = SysAllocString(accessibilityId.c_str());
  pAutomation->CreatePropertyCondition(UIA_AutomationIdPropertyId, varAutomationId, &pCondition);
  pRootElement->FindFirst(TreeScope_Descendants, pCondition, &pTarget);

  winrt::Windows::Data::Json::JsonObject uiaTree;
  if (pTarget != nullptr) {
    uiaTree = DumpUIATreeRecurse(pTarget, pWalker);
  }

  pWalker->Release();
  pRootElement->Release();
  pAutomation->Release();
  pCondition->Release();

  return uiaTree;
}

winrt::Windows::Data::Json::JsonObject PrintVisualTree(winrt::Microsoft::UI::Composition::Visual node) {
  winrt::Windows::Data::Json::JsonObject result;
  if (!node.Comment().empty()) {
    result.Insert(L"Comment", winrt::Windows::Data::Json::JsonValue::CreateStringValue(node.Comment()));
  }
  winrt::Windows::Data::Json::JsonArray visualSize;
  visualSize.Append(winrt::Windows::Data::Json::JsonValue::CreateNumberValue(node.Size().x));
  visualSize.Append(winrt::Windows::Data::Json::JsonValue::CreateNumberValue(node.Size().y));
  result.Insert(L"Size", visualSize);
  winrt::Windows::Data::Json::JsonArray visualOffset;
  visualOffset.Append(winrt::Windows::Data::Json::JsonValue::CreateNumberValue(node.Offset().x));
  visualOffset.Append(winrt::Windows::Data::Json::JsonValue::CreateNumberValue(node.Offset().y));
  visualOffset.Append(winrt::Windows::Data::Json::JsonValue::CreateNumberValue(node.Offset().z));
  result.Insert(L"Offset", visualOffset);
  result.Insert(L"Opacity", winrt::Windows::Data::Json::JsonValue::CreateNumberValue(node.Opacity()));
  auto spriteVisual = node.try_as<winrt::Microsoft::UI::Composition::SpriteVisual>();
  if (spriteVisual) {
    result.Insert(L"Visual Type", winrt::Windows::Data::Json::JsonValue::CreateStringValue(L"SpriteVisual"));
    auto spriteBrush = spriteVisual.Brush();
    if (spriteBrush) {
      winrt::Windows::Data::Json::JsonObject brush;
      auto colorBrush = spriteBrush.try_as<winrt::Microsoft::UI::Composition::CompositionColorBrush>();
      if (colorBrush) {
        brush.Insert(L"Brush Type", winrt::Windows::Data::Json::JsonValue::CreateStringValue(L"ColorBrush"));
        auto colorString = L"rgba(" + winrt::to_hstring(colorBrush.Color().R) + L", " +
            winrt::to_hstring(colorBrush.Color().G) + L", " + winrt::to_hstring(colorBrush.Color().B) + L", " +
            winrt::to_hstring(colorBrush.Color().A) + L")";
        brush.Insert(L"Color", winrt::Windows::Data::Json::JsonValue::CreateStringValue(colorString));
        result.Insert(L"Brush", brush);
      }
    }
  } else {
    result.Insert(L"Visual Type", winrt::Windows::Data::Json::JsonValue::CreateStringValue(L"Visual"));
  }
  return result;
}

winrt::Windows::Data::Json::JsonObject DumpVisualTreeRecurse(
    winrt::Microsoft::UI::Composition::Visual node,
    winrt::hstring accessibilityId,
    boolean targetNodeHit) {
  winrt::Windows::Data::Json::JsonObject result;
  boolean targetNodeFound = false;
  if (targetNodeHit) {
    result = PrintVisualTree(node);
  }

  auto containerNode = node.try_as<winrt::Microsoft::UI::Composition::ContainerVisual>();
  if (containerNode == nullptr) {
    return result;
  }
  auto nodeChildren = containerNode.Children();
  winrt::Windows::Data::Json::JsonArray children;
  for (auto childVisual : nodeChildren) {
    if (!targetNodeHit && childVisual.Comment() == accessibilityId) {
      targetNodeFound = true;
      result = DumpVisualTreeRecurse(childVisual, accessibilityId, true);
      break;
    } else if (targetNodeHit) {
      children.Append(DumpVisualTreeRecurse(childVisual, accessibilityId, targetNodeHit));
    } else if (!targetNodeHit) {
      auto subtree = DumpVisualTreeRecurse(childVisual, accessibilityId, targetNodeHit);
      if (subtree.Size() > 0) {
        result = subtree;
        break;
      }
    }
  }
  if (targetNodeHit && children.Size() > 0) {
    result.Insert(L"Children", children);
  }
  return result;
}

winrt::Windows::Data::Json::JsonObject DumpVisualTreeHelper(winrt::Windows::Data::Json::JsonObject payloadObj) {
  auto accessibilityId = payloadObj.GetNamedString(L"accessibilityId");
  winrt::Windows::Data::Json::JsonObject visualTree;
  auto root = winrt::Microsoft::ReactNative::Composition::MicrosoftCompositionContextHelper::InnerVisual(
      global_rootView->RootVisual());
  visualTree = DumpVisualTreeRecurse(root, accessibilityId, false);
  return visualTree;
}

winrt::Windows::Data::Json::JsonObject DumpVisualTree(winrt::Windows::Data::Json::JsonValue payload) {
  winrt::Windows::Data::Json::JsonObject payloadObj = payload.GetObject();
  winrt::Windows::Data::Json::JsonObject result;
  result.Insert(L"Automation Tree", DumpUIATreeHelper(payloadObj));
  result.Insert(L"Visual Tree", DumpVisualTreeHelper(payloadObj));
  return result;
}

winrt::Windows::Foundation::IAsyncAction LoopServer(winrt::AutomationChannel::Server &server) {
  while (true) {
    try {
      co_await server.ProcessAllClientRequests(8603, std::chrono::milliseconds(50));
    } catch (const std::exception ex) {
    }
  }
}
