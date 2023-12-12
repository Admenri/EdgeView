#pragma once

#include <atomic>
#include <thread>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "ev_msgpump.h"
#include "event_notify.h"
#include "util.h"
#include "webview_host.h"

namespace edgeview {

class EdgeWidgetHost;
class BrowserEventDispatcher;
struct FrameData;

struct StrMapElement {
  LPCSTR key;
  LPCSTR value;
};

class Semaphore : public base::RefCounted<Semaphore> {
 public:
  Semaphore() : atomic(false) {}
  ~Semaphore() = default;

  Semaphore(const Semaphore&) = delete;
  Semaphore& operator=(const Semaphore&) = delete;

  void Notify() { atomic.store(true); }
  bool IsTriggered() { return atomic.load(); }
  void Reset() { atomic.store(false); }

 private:
  std::atomic_bool atomic;
};

struct EnvironmentData : public base::RefCounted<EnvironmentData> {
  WRL::ComPtr<ICoreWebView2Environment11> core_env;
  scoped_refptr<MessagePump> msg_pump;
  scoped_refptr<Semaphore> semaphore_flag;

  std::thread::id ui_thread;

  base::WeakPtrFactory<EnvironmentData> weak_ptr_{this};

  EnvironmentData() = default;

  void PostEvent(base::OnceClosure event_notify) {
    // Always async running on ui thread
    msg_pump->PostTask(std::move(event_notify));
  }

  void PostUITask(base::OnceClosure task) {
    if (RunningOnUIThread()) {
      return std::move(task).Run();
    }

    // Only non UI thread should be post
    msg_pump->PostTask(std::move(task));
  }

  bool RunningOnUIThread() { return std::this_thread::get_id() == ui_thread; }

  scoped_refptr<Semaphore> semaphore() const {
    semaphore_flag->Reset();
    return semaphore_flag;
  }

  bool SyncWaitIfNeed() {
    if (semaphore_flag->IsTriggered()) return false;

    if (!RunningOnUIThread()) {
      // Only non UI thread should be sync
      while (!semaphore_flag->IsTriggered()) {
        // Yield current thread
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    } else {
      MSG pump_messsage{0};
      while (!semaphore_flag->IsTriggered()) {
        if (PeekMessage(&pump_messsage, nullptr, 0, 0, PM_REMOVE)) {
          TranslateMessage(&pump_messsage);
          DispatchMessage(&pump_messsage);

          if (pump_messsage.message == WM_QUIT) break;
        }
      }
    }

    // Reset
    semaphore_flag->Reset();

    return true;
  }
};

struct BrowserData : public base::RefCounted<BrowserData> {
  base::WeakPtr<EnvironmentData> parent;

  WRL::ComPtr<ICoreWebView2Controller4> core_controller;
  WRL::ComPtr<ICoreWebView2_16> core_webview;
  // Maybe nullptr
  WRL::ComPtr<ICoreWebView2CompositionController3> core_composition = nullptr;
  // Maybe nullptr
  scoped_refptr<EdgeWidgetHost> browser_window = nullptr;
  scoped_refptr<BrowserEventDispatcher> dispatcher;
  LPVOID pCallback = nullptr;

  std::vector<scoped_refptr<FrameData>> frames;

  base::WeakPtrFactory<BrowserData> weak_ptr_{this};

  BrowserData() = default;
};

struct FrameData : public base::RefCounted<FrameData> {
  base::WeakPtr<BrowserData> browser;

  WRL::ComPtr<ICoreWebView2Frame3> core_frame;
  std::string url;

  base::WeakPtrFactory<FrameData> weak_ptr_{this};

  FrameData(WRL::ComPtr<ICoreWebView2Frame3> frame) : core_frame(frame) {}
};

struct NewWindowDelegate : public base::RefCounted<NewWindowDelegate> {
  base::WeakPtr<BrowserData> browser;

  WRL::ComPtr<ICoreWebView2NewWindowRequestedEventArgs> core_newwindow;
  WRL::ComPtr<ICoreWebView2Deferral> internal_deferral;

  NewWindowDelegate() = default;
};

struct ScriptDialogDelegate : public base::RefCounted<ScriptDialogDelegate> {
  base::WeakPtr<BrowserData> browser;

  WRL::ComPtr<ICoreWebView2ScriptDialogOpeningEventArgs> core_dialog;
  WRL::ComPtr<ICoreWebView2Deferral> internal_deferral;

  ScriptDialogDelegate() = default;
};

struct ContextMenuParams : public base::RefCounted<ContextMenuParams> {
  base::WeakPtr<BrowserData> browser;

  WRL::ComPtr<ICoreWebView2ContextMenuRequestedEventArgs> core_menu;
  WRL::ComPtr<ICoreWebView2Deferral> internal_deferral;

  ContextMenuParams() = default;
};

struct ContextMenuCollection : public base::RefCounted<ContextMenuCollection> {
  base::WeakPtr<BrowserData> browser;

  WRL::ComPtr<ICoreWebView2ContextMenuItemCollection> core_list;

  ContextMenuCollection() = default;
};

struct ContextMenuItem : public base::RefCounted<ContextMenuItem> {
  base::WeakPtr<BrowserData> browser;

  WRL::ComPtr<ICoreWebView2ContextMenuItem> core_item;

  ContextMenuItem() = default;
};

struct PermissionDelegate : public base::RefCounted<PermissionDelegate> {
  base::WeakPtr<BrowserData> browser;

  WRL::ComPtr<ICoreWebView2PermissionRequestedEventArgs2> core_delegate;
  WRL::ComPtr<ICoreWebView2Deferral> internal_deferral;

  PermissionDelegate() = default;
};

struct CookieManagerData : public base::RefCounted<CookieManagerData> {
  base::WeakPtr<BrowserData> browser;

  WRL::ComPtr<ICoreWebView2CookieManager> core_manager;

  CookieManagerData() = default;
};

struct ResourceRequestCallback
    : public base::RefCounted<ResourceRequestCallback> {
  base::WeakPtr<BrowserData> browser;

  json event_parameter;

  ResourceRequestCallback() = default;
};

struct ResourceResponseCallback
    : public base::RefCounted<ResourceResponseCallback> {
  base::WeakPtr<BrowserData> browser;

  json event_parameter;

  ResourceResponseCallback() = default;
};

struct BasicAuthenticationCallback
    : public base::RefCounted<BasicAuthenticationCallback> {
  base::WeakPtr<BrowserData> browser;

  WRL::ComPtr<ICoreWebView2BasicAuthenticationRequestedEventArgs> core_callback;
  WRL::ComPtr<ICoreWebView2Deferral> internal_deferral;

  BasicAuthenticationCallback() = default;
};

struct DownloadOperation : public base::RefCounted<DownloadOperation> {
  base::WeakPtr<BrowserData> browser;

  WRL::ComPtr<ICoreWebView2DownloadOperation> core_operation;

  DownloadOperation() = default;
};

struct DownloadConfirm : public base::RefCounted<DownloadConfirm> {
  base::WeakPtr<BrowserData> browser;

  WRL::ComPtr<ICoreWebView2DownloadStartingEventArgs> core_args;
  WRL::ComPtr<ICoreWebView2Deferral> internal_deferral;

  DownloadConfirm() = default;
};

struct DOMOperation : public base::RefCounted<DOMOperation> {
  base::WeakPtr<BrowserData> browser;

  DOMOperation() = default;
};

}  // namespace edgeview
