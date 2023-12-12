#include "ev_env.h"

#include "ev_browser.h"
#include "webview_host.h"

namespace edgeview {

namespace {

using BrowserCreateParams = struct {
  HWND hParent;
  int left;
  int top;
  int width;
  int height;
  BOOL bPrivateMode;
  LPCSTR pszProfileName;
};

void WINAPI CreateBrowser(EnvironmentData* obj, BrowserCreateParams* params,
                          LPVOID lpCallback, DWORD* retObj) {
  scoped_refptr<BrowserData> browser_wrapper = new BrowserData();

  auto task = base::BindOnce(
      [](scoped_refptr<EnvironmentData> self, scoped_refptr<Semaphore> sync,
         scoped_refptr<BrowserData> browser_wrapper, HWND parent_window,
         RECT rect, BOOL private_mode, std::string profile_name,
         LPVOID lpCallback) {
        browser_wrapper->browser_window = new EdgeWidgetHost(
            browser_wrapper->weak_ptr_.GetWeakPtr(), parent_window, rect);
        browser_wrapper->dispatcher = new BrowserEventDispatcher(
            browser_wrapper->weak_ptr_.GetWeakPtr(), lpCallback);
        browser_wrapper->parent = self->weak_ptr_.GetWeakPtr();

        WRL::ComPtr<ICoreWebView2Environment10> env = nullptr;
        self->core_env->QueryInterface<ICoreWebView2Environment10>(&env);

        WRL::ComPtr<ICoreWebView2ControllerOptions> options = nullptr;
        env->CreateCoreWebView2ControllerOptions(&options);

        options->put_IsInPrivateModeEnabled(private_mode);
        if (!profile_name.empty())
          options->put_ProfileName(
              Utf8Conv::Utf8ToUtf16(profile_name.c_str()).c_str());

        env->CreateCoreWebView2ControllerWithOptions(
            browser_wrapper->browser_window->GetHandle(), options.Get(),
            WRL::Callback<
                ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                [browser_wrapper, sync](
                    HRESULT errorCode,
                    ICoreWebView2Controller* createdController) {
                  createdController->QueryInterface<ICoreWebView2Controller4>(
                      &browser_wrapper->core_controller);
                  WRL::ComPtr<ICoreWebView2> webview2 = nullptr;
                  createdController->get_CoreWebView2(&webview2);
                  webview2->QueryInterface<ICoreWebView2_16>(
                      &browser_wrapper->core_webview);

                  browser_wrapper->browser_window->OnSize();
                  BindEventForWebView(browser_wrapper);
                  BindEventForUpdate(browser_wrapper);

                  sync->Notify();

                  browser_wrapper->parent->PostUITask(base::BindOnce(
                      [](scoped_refptr<BrowserData> browser_wrapper) {
                        browser_wrapper->dispatcher->OnCreated();
                      },
                      browser_wrapper));

                  return S_OK;
                })
                .Get());
      },
      scoped_refptr(obj), obj->semaphore(), browser_wrapper, params->hParent,
      RECT{params->left, params->top, params->width, params->height},
      params->bPrivateMode,
      params->pszProfileName ? std::string(params->pszProfileName)
                             : std::string(),
      lpCallback);

  obj->PostUITask(std::move(task));
  obj->SyncWaitIfNeed();

  if (retObj) {
    browser_wrapper->AddRef();
    retObj[1] = (DWORD)browser_wrapper.get();
    retObj[2] = (DWORD)fnBrowserTable;
  }
}

void WINAPI CreateCompositionBrowser(EnvironmentData* obj,
                                     BrowserCreateParams* params,
                                     LPVOID lpCallback, DWORD* retObj) {
  scoped_refptr<BrowserData> browser_wrapper = new BrowserData();

  auto task = base::BindOnce(
      [](scoped_refptr<EnvironmentData> self, scoped_refptr<Semaphore> sync,
         scoped_refptr<BrowserData> browser_wrapper, HWND parent_window,
         RECT rect, BOOL private_mode, std::string profile_name,
         LPVOID lpCallback) {
        browser_wrapper->browser_window = new EdgeWidgetHost(
            browser_wrapper->weak_ptr_.GetWeakPtr(), parent_window, rect);
        browser_wrapper->dispatcher = new BrowserEventDispatcher(
            browser_wrapper->weak_ptr_.GetWeakPtr(), lpCallback);
        browser_wrapper->parent = self->weak_ptr_.GetWeakPtr();

        WRL::ComPtr<ICoreWebView2Environment10> env = nullptr;
        self->core_env->QueryInterface<ICoreWebView2Environment10>(&env);

        WRL::ComPtr<ICoreWebView2ControllerOptions> options = nullptr;
        env->CreateCoreWebView2ControllerOptions(&options);

        options->put_IsInPrivateModeEnabled(private_mode);
        if (!profile_name.empty())
          options->put_ProfileName(
              Utf8Conv::Utf8ToUtf16(profile_name.c_str()).c_str());

        env->CreateCoreWebView2CompositionControllerWithOptions(
            browser_wrapper->browser_window->GetHandle(), options.Get(),
            WRL::Callback<
                ICoreWebView2CreateCoreWebView2CompositionControllerCompletedHandler>(
                [browser_wrapper, sync](
                    HRESULT errorCode,
                    ICoreWebView2CompositionController* webView) {
                  webView->QueryInterface<ICoreWebView2CompositionController3>(
                      &browser_wrapper->core_composition);

                  WRL::ComPtr<ICoreWebView2Controller> createdController =
                      nullptr;
                  webView->QueryInterface<ICoreWebView2Controller>(
                      &createdController);

                  createdController->QueryInterface<ICoreWebView2Controller4>(
                      &browser_wrapper->core_controller);
                  WRL::ComPtr<ICoreWebView2> webview2 = nullptr;
                  createdController->get_CoreWebView2(&webview2);
                  webview2->QueryInterface<ICoreWebView2_16>(
                      &browser_wrapper->core_webview);

                  browser_wrapper->browser_window->OnSize();
                  BindEventForWebView(browser_wrapper);
                  BindEventForUpdate(browser_wrapper);

                  sync->Notify();

                  browser_wrapper->parent->PostUITask(base::BindOnce(
                      [](scoped_refptr<BrowserData> browser_wrapper) {
                        browser_wrapper->dispatcher->OnCreated();
                      },
                      browser_wrapper));

                  return S_OK;
                })
                .Get());
      },
      scoped_refptr(obj), obj->semaphore(), browser_wrapper, params->hParent,
      RECT{params->left, params->top, params->width, params->height},
      params->bPrivateMode,
      params->pszProfileName ? std::string(params->pszProfileName)
                             : std::string(),
      lpCallback);

  obj->PostUITask(std::move(task));
  obj->SyncWaitIfNeed();

  if (retObj) {
    browser_wrapper->AddRef();
    retObj[1] = (DWORD)browser_wrapper.get();
    retObj[2] = (DWORD)fnBrowserTable;
  }
}

LPCSTR WINAPI GetChildProcessInfos(EnvironmentData* obj) {
  std::string processes_info;

  obj->PostUITask(base::BindOnce(
      [](scoped_refptr<EnvironmentData> obj, scoped_refptr<Semaphore> sync,
         std::string* info) {
        WRL::ComPtr<ICoreWebView2ProcessInfoCollection> infos = nullptr;
        obj->core_env->GetProcessInfos(&infos);

        uint32_t size = 0;
        infos->get_Count(&size);

        for (size_t i = 0; i < size; ++i) {
          WRL::ComPtr<ICoreWebView2ProcessInfo> process_info = nullptr;
          infos->GetValueAtIndex(i, &process_info);
          COREWEBVIEW2_PROCESS_KIND kind;
          int pid = 0;

          process_info->get_Kind(&kind);
          process_info->get_ProcessId(&pid);

          *info += std::to_string(pid) + "=" + std::to_string(kind) + "|";
        }

        sync->Notify();
      },
      scoped_refptr(obj), obj->semaphore(), &processes_info));
  obj->SyncWaitIfNeed();

  return WrapComString(processes_info.c_str());
}

}  // namespace

DWORD fnEnvironmentTable[] = {
    (DWORD)CreateBrowser,
    (DWORD)CreateCompositionBrowser,
    (DWORD)GetChildProcessInfos,
};

}  // namespace edgeview

EV_EXPORTS(CreateEnvironment, BOOL)(EnvCreateParams* params, DWORD* retObj) {
  auto options = WRL::Make<CoreWebView2EnvironmentOptions>();

  if (params->pszCommandLine && *params->pszCommandLine)
    options->put_AdditionalBrowserArguments(
        Utf8Conv::Utf8ToUtf16(params->pszCommandLine).c_str());
  if (params->pszLanguage && *params->pszLanguage)
    options->put_Language(Utf8Conv::Utf8ToUtf16(params->pszLanguage).c_str());

  // Avoid send report to microsoft server
  options->put_IsCustomCrashReportingEnabled(true);

  scoped_refptr<edgeview::EnvironmentData> shared_data =
      new edgeview::EnvironmentData();
  shared_data->msg_pump = new edgeview::MessagePump();
  shared_data->ui_thread = std::this_thread::get_id();
  shared_data->semaphore_flag = new edgeview::Semaphore();

  CreateCoreWebView2EnvironmentWithOptions(
      Utf8Conv::Utf8ToUtf16(params->pszBrowserPath).c_str(),
      Utf8Conv::Utf8ToUtf16(params->pszUserDataPath).c_str(), options.Get(),
      WRL::Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
          [shared_data](HRESULT errorCode,
                        ICoreWebView2Environment* createdEnvironment) {
            createdEnvironment->QueryInterface<ICoreWebView2Environment11>(
                &shared_data->core_env);
            return S_OK;
          })
          .Get());

  if (retObj) {
    shared_data->AddRef();
    retObj[1] = (DWORD)shared_data.get();
    retObj[2] = (DWORD)edgeview::fnEnvironmentTable;
  }

  return !!shared_data->core_env;
}

EV_EXPORTS(CheckRuntime, LPCSTR)(LPCSTR browser_path) {
  LPWSTR available_version = nullptr;
  HRESULT hr = GetAvailableCoreWebView2BrowserVersionString(
      Utf8Conv::Utf8ToUtf16(browser_path).c_str(), &available_version);

  if (!available_version || !SUCCEEDED(hr)) return nullptr;

  return edgeview::WrapComString(available_version);
}
