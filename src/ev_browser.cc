#include "ev_browser.h"

#include "edgeview_data.h"
#include "ev_browser.h"
#include "ev_dom.h"
#include "ev_env.h"
#include "ev_extension.h"
#include "ev_frame.h"
#include "ev_network.h"
#include "webview_host.h"

namespace edgeview {

json RemoteObjectToJSON(RemoteObject* ro) {
  json obj = json::object();

  obj["type"] = ro->type;
  obj["subtype"] = ro->subtype;
  obj["className"] = ro->class_name;
  obj["value"] = json::parse(ro->raw_value);
  obj["unserializableValue"] = ro->unserializable_value;
  obj["description"] = ro->description;
  obj["deepSerializedValue"] = json::parse(ro->raw_deepSerializedValue);
  obj["objectId"] = ro->objectID;

  return obj;
}

void JSONToRemoteObject(RemoteObject* ro, const json& raw) {
  ro->type = WrapComString(raw["type"].template get<std::string>().c_str());

  if (raw.find("subtype") != raw.end()) {
    ro->subtype =
        WrapComString(raw["subtype"].template get<std::string>().c_str());
  }

  if (raw.find("className") != raw.end()) {
    ro->class_name =
        WrapComString(raw["className"].template get<std::string>().c_str());
  }

  if (raw.find("value") != raw.end()) {
    ro->raw_value = WrapComString(raw["value"].dump().c_str());
  }

  if (raw.find("unserializableValue") != raw.end()) {
    ro->unserializable_value = WrapComString(
        raw["unserializableValue"].template get<std::string>().c_str());
  }

  if (raw.find("description") != raw.end()) {
    ro->description =
        WrapComString(raw["description"].template get<std::string>().c_str());
  }

  if (raw.find("deepSerializedValue") != raw.end()) {
    ro->raw_deepSerializedValue =
        WrapComString(raw["deepSerializedValue"].dump().c_str());
  }

  if (raw.find("objectId") != raw.end()) {
    ro->objectID =
        WrapComString(raw["objectId"].template get<std::string>().c_str());
  }
}

void BindEventForWebView(scoped_refptr<BrowserData> browser_wrapper) {
  base::WeakPtr<BrowserData> weak_ptr = browser_wrapper->weak_ptr_.GetWeakPtr();

  browser_wrapper->core_webview->add_NewWindowRequested(
      WRL::Callback<ICoreWebView2NewWindowRequestedEventHandler>(
          [weak_ptr](ICoreWebView2* sender,
                     ICoreWebView2NewWindowRequestedEventArgs* args) {
            scoped_refptr<NewWindowDelegate> wrapper = new NewWindowDelegate();
            wrapper->browser = weak_ptr;
            wrapper->core_newwindow = args;
            args->GetDeferral(&wrapper->internal_deferral);

            weak_ptr->parent->PostEvent(base::BindOnce(
                [](base::WeakPtr<BrowserData> weak_ptr,
                   scoped_refptr<NewWindowDelegate> wrapper) {
                  weak_ptr->dispatcher->OnNewWindowRequested(wrapper);
                },
                weak_ptr, wrapper));

            return S_OK;
          })
          .Get(),
      nullptr);

  browser_wrapper->core_webview->add_WindowCloseRequested(
      WRL::Callback<ICoreWebView2WindowCloseRequestedEventHandler>(
          [weak_ptr](ICoreWebView2* sender, IUnknown* args) {
            weak_ptr->parent->PostEvent(base::BindOnce(
                [](base::WeakPtr<BrowserData> weak_ptr) {
                  weak_ptr->dispatcher->OnCloseRequested();
                },
                weak_ptr));

            return S_OK;
          })
          .Get(),
      nullptr);

  browser_wrapper->core_webview->add_DocumentTitleChanged(
      WRL::Callback<ICoreWebView2DocumentTitleChangedEventHandler>(
          [weak_ptr](ICoreWebView2* sender, IUnknown* args) {
            weak_ptr->parent->PostEvent(base::BindOnce(
                [](base::WeakPtr<BrowserData> weak_ptr) {
                  wil::unique_cotaskmem_string raw_title;
                  weak_ptr->core_webview->get_DocumentTitle(&raw_title);

                  weak_ptr->dispatcher->OnDocumentTitleChanged(
                      WrapComString(raw_title));
                },
                weak_ptr));

            return S_OK;
          })
          .Get(),
      nullptr);

  browser_wrapper->core_webview->add_ContainsFullScreenElementChanged(
      WRL::Callback<ICoreWebView2ContainsFullScreenElementChangedEventHandler>(
          [weak_ptr](ICoreWebView2* sender, IUnknown* args) {
            weak_ptr->parent->PostEvent(base::BindOnce(
                [](base::WeakPtr<BrowserData> weak_ptr) {
                  BOOL fullscreen = FALSE;
                  weak_ptr->core_webview->get_ContainsFullScreenElement(
                      &fullscreen);

                  weak_ptr->dispatcher->OnFullscreenModeChanged(fullscreen);
                },
                weak_ptr));

            return S_OK;
          })
          .Get(),
      nullptr);

  browser_wrapper->core_webview->add_NavigationStarting(
      WRL::Callback<ICoreWebView2NavigationStartingEventHandler>(
          [weak_ptr](ICoreWebView2* sender,
                     ICoreWebView2NavigationStartingEventArgs* args) {
            WRL::ComPtr<ICoreWebView2NavigationStartingEventArgs2> nav_args;
            args->QueryInterface<ICoreWebView2NavigationStartingEventArgs2>(
                &nav_args);

            // No-sync-support
            weak_ptr->parent->PostUITask(base::BindOnce(
                [](base::WeakPtr<BrowserData> weak_ptr,
                   WRL::ComPtr<ICoreWebView2NavigationStartingEventArgs2>
                       args) {
                  wil::unique_cotaskmem_string raw_url = nullptr;
                  args->get_Uri(&raw_url);
                  LPCSTR url = WrapComString(raw_url);

                  BOOL user_gesture = FALSE, is_redirect = FALSE;
                  args->get_IsUserInitiated(&user_gesture);
                  args->get_IsRedirected(&is_redirect);

                  std::wstring headers;
                  WRL::ComPtr<ICoreWebView2HttpRequestHeaders> header_obj =
                      nullptr;
                  args->get_RequestHeaders(&header_obj);
                  WRL::ComPtr<ICoreWebView2HttpHeadersCollectionIterator> iter =
                      nullptr;
                  header_obj->GetIterator(&iter);
                  BOOL has_current = FALSE;
                  while (SUCCEEDED(iter->get_HasCurrentHeader(&has_current)) &&
                         has_current) {
                    wil::unique_cotaskmem_string raw_key = nullptr,
                                                 raw_value = nullptr;
                    iter->GetCurrentHeader(&raw_key, &raw_value);

                    headers.append(raw_key.get());
                    headers.append(L": ");
                    headers.append(raw_value.get());
                    headers.append(L"\r\n");

                    iter->MoveNext(nullptr);
                  }

                  uint64_t nav_id = 0;
                  args->get_NavigationId(&nav_id);

                  BOOL cancel_nav = weak_ptr->dispatcher->OnBeforeNavigation(
                      url, user_gesture, is_redirect,
                      WrapComString(headers.c_str()), nav_id);

                  args->put_Cancel(cancel_nav);
                },
                weak_ptr, std::move(nav_args)));

            return S_OK;
          })
          .Get(),
      nullptr);

  browser_wrapper->core_webview->add_ContentLoading(
      WRL::Callback<ICoreWebView2ContentLoadingEventHandler>(
          [weak_ptr](ICoreWebView2* sender,
                     ICoreWebView2ContentLoadingEventArgs* args) {
            BOOL is_error_page = FALSE;
            args->get_IsErrorPage(&is_error_page);

            uint64_t nav_id = 0;
            args->get_NavigationId(&nav_id);

            weak_ptr->parent->PostEvent(base::BindOnce(
                [](base::WeakPtr<BrowserData> weak_ptr, BOOL is_error_page,
                   uint64_t nav_id) {
                  weak_ptr->dispatcher->OnContentLoading(is_error_page, nav_id);
                },
                weak_ptr, is_error_page, nav_id));

            return S_OK;
          })
          .Get(),
      nullptr);

  browser_wrapper->core_webview->add_SourceChanged(
      WRL::Callback<ICoreWebView2SourceChangedEventHandler>(
          [weak_ptr](ICoreWebView2* sender,
                     ICoreWebView2SourceChangedEventArgs* args) {
            BOOL is_newdoc = FALSE;
            args->get_IsNewDocument(&is_newdoc);

            weak_ptr->parent->PostEvent(base::BindOnce(
                [](base::WeakPtr<BrowserData> weak_ptr, BOOL is_newdoc) {
                  weak_ptr->dispatcher->OnSourceChanged(is_newdoc);
                },
                weak_ptr, is_newdoc));

            return S_OK;
          })
          .Get(),
      nullptr);

  browser_wrapper->core_webview->add_HistoryChanged(
      WRL::Callback<ICoreWebView2HistoryChangedEventHandler>(
          [weak_ptr](ICoreWebView2* sender, IUnknown* args) {
            weak_ptr->parent->PostEvent(base::BindOnce(
                [](base::WeakPtr<BrowserData> weak_ptr) {
                  weak_ptr->dispatcher->OnHistoryChanged();
                },
                weak_ptr));

            return S_OK;
          })
          .Get(),
      nullptr);

  browser_wrapper->core_webview->add_NavigationCompleted(
      WRL::Callback<ICoreWebView2NavigationCompletedEventHandler>(
          [weak_ptr](ICoreWebView2* sender,
                     ICoreWebView2NavigationCompletedEventArgs* args) {
            BOOL success = FALSE;
            args->get_IsSuccess(&success);

            COREWEBVIEW2_WEB_ERROR_STATUS status =
                COREWEBVIEW2_WEB_ERROR_STATUS();
            args->get_WebErrorStatus(&status);

            uint64_t nav_id = 0;
            args->get_NavigationId(&nav_id);

            weak_ptr->parent->PostEvent(base::BindOnce(
                [](base::WeakPtr<BrowserData> weak_ptr, BOOL success,
                   COREWEBVIEW2_WEB_ERROR_STATUS status, uint64_t nav_id) {
                  weak_ptr->dispatcher->OnNavigationComplete(success, status,
                                                             nav_id);
                },
                weak_ptr, success, status, nav_id));

            return S_OK;
          })
          .Get(),
      nullptr);

  browser_wrapper->core_webview->add_ScriptDialogOpening(
      WRL::Callback<ICoreWebView2ScriptDialogOpeningEventHandler>(
          [weak_ptr](ICoreWebView2* sender,
                     ICoreWebView2ScriptDialogOpeningEventArgs* args) {
            WRL::ComPtr<ICoreWebView2ScriptDialogOpeningEventArgs> args_obj =
                args;

            scoped_refptr<ScriptDialogDelegate> delegate =
                new ScriptDialogDelegate();
            delegate->browser = weak_ptr;
            delegate->core_dialog = args;
            args->GetDeferral(&delegate->internal_deferral);

            weak_ptr->parent->PostEvent(base::BindOnce(
                [](base::WeakPtr<BrowserData> weak_ptr,
                   WRL::ComPtr<ICoreWebView2ScriptDialogOpeningEventArgs> args,
                   scoped_refptr<ScriptDialogDelegate> delegate) {
                  wil::unique_cotaskmem_string raw_url = nullptr;
                  args->get_Uri(&raw_url);

                  COREWEBVIEW2_SCRIPT_DIALOG_KIND kind =
                      COREWEBVIEW2_SCRIPT_DIALOG_KIND();
                  args->get_Kind(&kind);

                  wil::unique_cotaskmem_string raw_message = nullptr;
                  args->get_Message(&raw_message);

                  wil::unique_cotaskmem_string raw_deftext = nullptr;
                  args->get_DefaultText(&raw_deftext);

                  weak_ptr->dispatcher->OnScriptDialogRequested(
                      WrapComString(raw_url), kind, WrapComString(raw_message),
                      WrapComString(raw_deftext), delegate);
                },
                weak_ptr, std::move(args_obj), std::move(delegate)));

            return S_OK;
          })
          .Get(),
      nullptr);

  browser_wrapper->core_webview->add_ContextMenuRequested(
      WRL::Callback<ICoreWebView2ContextMenuRequestedEventHandler>(
          [weak_ptr](ICoreWebView2* sender,
                     ICoreWebView2ContextMenuRequestedEventArgs* args) {
            scoped_refptr<ContextMenuParams> params = new ContextMenuParams();
            params->browser = weak_ptr;
            params->core_menu = args;
            args->GetDeferral(&params->internal_deferral);

            weak_ptr->parent->PostEvent(base::BindOnce(
                [](base::WeakPtr<BrowserData> weak_ptr,
                   scoped_refptr<ContextMenuParams> params) {
                  weak_ptr->dispatcher->OnContextMenuRequested(params);
                },
                weak_ptr, params));

            return S_OK;
          })
          .Get(),
      nullptr);

  browser_wrapper->core_webview->add_PermissionRequested(
      WRL::Callback<ICoreWebView2PermissionRequestedEventHandler>(
          [weak_ptr](ICoreWebView2* sender,
                     ICoreWebView2PermissionRequestedEventArgs* args) {
            scoped_refptr<PermissionDelegate> delegate =
                new PermissionDelegate();
            args->QueryInterface<ICoreWebView2PermissionRequestedEventArgs2>(
                &delegate->core_delegate);
            args->GetDeferral(&delegate->internal_deferral);
            delegate->browser = weak_ptr;

            weak_ptr->parent->PostEvent(base::BindOnce(
                [](base::WeakPtr<BrowserData> weak_ptr,
                   scoped_refptr<PermissionDelegate> delegate) {
                  wil::unique_cotaskmem_string url = nullptr;
                  delegate->core_delegate->get_Uri(&url);

                  COREWEBVIEW2_PERMISSION_KIND kind;
                  delegate->core_delegate->get_PermissionKind(&kind);

                  BOOL user_gesture = FALSE;
                  delegate->core_delegate->get_IsUserInitiated(&user_gesture);

                  weak_ptr->dispatcher->OnPermissionRequested(
                      WrapComString(url), kind, user_gesture, delegate);
                },
                weak_ptr, delegate));

            return S_OK;
          })
          .Get(),
      nullptr);

  browser_wrapper->core_controller->add_AcceleratorKeyPressed(
      WRL::Callback<ICoreWebView2AcceleratorKeyPressedEventHandler>(
          [weak_ptr](ICoreWebView2Controller* sender,
                     ICoreWebView2AcceleratorKeyPressedEventArgs* args) {
            COREWEBVIEW2_KEY_EVENT_KIND kind;
            args->get_KeyEventKind(&kind);
            uint32_t virtual_key = 0;
            args->get_VirtualKey(&virtual_key);
            int lparam = 0;
            args->get_KeyEventLParam(&lparam);
            COREWEBVIEW2_PHYSICAL_KEY_STATUS status;
            args->get_PhysicalKeyStatus(&status);

            BOOL handled = weak_ptr->dispatcher->OnKeyEvent(kind, virtual_key,
                                                            lparam, &status);

            args->put_Handled(handled);

            return S_OK;
          })
          .Get(),
      nullptr);

  browser_wrapper->core_webview->add_BasicAuthenticationRequested(
      WRL::Callback<ICoreWebView2BasicAuthenticationRequestedEventHandler>(
          [weak_ptr](ICoreWebView2* sender,
                     ICoreWebView2BasicAuthenticationRequestedEventArgs* args) {
            scoped_refptr<BasicAuthenticationCallback> callback =
                new BasicAuthenticationCallback();
            callback->core_callback = args;
            args->GetDeferral(&callback->internal_deferral);
            callback->browser = weak_ptr;

            weak_ptr->parent->PostEvent(base::BindOnce(
                [](base::WeakPtr<BrowserData> weak_ptr,
                   scoped_refptr<BasicAuthenticationCallback> callback) {
                  wil::unique_cotaskmem_string url = nullptr,
                                               challenge = nullptr;
                  callback->core_callback->get_Uri(&url);
                  callback->core_callback->get_Challenge(&challenge);

                  weak_ptr->dispatcher->BasicAuthRequested(
                      WrapComString(url), WrapComString(challenge), callback);
                },
                weak_ptr, callback));

            return S_OK;
          })
          .Get(),
      nullptr);

  browser_wrapper->core_webview->add_WebMessageReceived(
      WRL::Callback<ICoreWebView2WebMessageReceivedEventHandler>(
          [weak_ptr](ICoreWebView2* sender,
                     ICoreWebView2WebMessageReceivedEventArgs* args) {
            wil::unique_cotaskmem_string src_url = nullptr;
            args->get_Source(&src_url);
            wil::unique_cotaskmem_string json_args = nullptr;
            args->get_WebMessageAsJson(&json_args);

            weak_ptr->parent->PostEvent(base::BindOnce(
                [](base::WeakPtr<BrowserData> weak_ptr, LPCSTR source_url,
                   LPCSTR json_args) {
                  weak_ptr->dispatcher->OnReceivedWebMessage(
                      nullptr, source_url, json_args);
                },
                weak_ptr, WrapComString(src_url), WrapComString(json_args)));

            return S_OK;
          })
          .Get(),
      nullptr);

  browser_wrapper->core_webview->add_DownloadStarting(
      WRL::Callback<ICoreWebView2DownloadStartingEventHandler>(
          [weak_ptr](ICoreWebView2* sender,
                     ICoreWebView2DownloadStartingEventArgs* args) {
            scoped_refptr<DownloadConfirm> confirm = new DownloadConfirm();
            confirm->browser = weak_ptr;
            confirm->core_args = args;
            args->GetDeferral(&confirm->internal_deferral);

            weak_ptr->parent->PostEvent(base::BindOnce(
                [](base::WeakPtr<BrowserData> weak_ptr,
                   scoped_refptr<DownloadConfirm> confirm) {
                  weak_ptr->dispatcher->OnBeforeDownload(confirm);
                },
                weak_ptr, confirm));

            return S_OK;
          })
          .Get(),
      nullptr);

  browser_wrapper->core_webview->add_FrameCreated(
      WRL::Callback<ICoreWebView2FrameCreatedEventHandler>(
          [weak_ptr](ICoreWebView2* sender,
                     ICoreWebView2FrameCreatedEventArgs* args) {
            WRL::ComPtr<ICoreWebView2Frame> raw_frame = nullptr;
            WRL::ComPtr<ICoreWebView2Frame3> frame_obj = nullptr;
            args->get_Frame(&raw_frame);
            raw_frame->QueryInterface<ICoreWebView2Frame3>(&frame_obj);

            scoped_refptr<FrameData> frame = new FrameData(frame_obj);
            frame->browser = weak_ptr;
            weak_ptr->frames.push_back(frame);

            frame_obj->add_NavigationStarting(
                WRL::Callback<ICoreWebView2FrameNavigationStartingEventHandler>(
                    [weak_ptr = frame->weak_ptr_.GetWeakPtr()](
                        ICoreWebView2Frame* sender,
                        ICoreWebView2NavigationStartingEventArgs* args) {
                      wil::unique_cotaskmem_string raw_url = nullptr;
                      args->get_Uri(&raw_url);
                      weak_ptr->url = Utf8Conv::Utf16ToUtf8(raw_url.get());
                      return S_OK;
                    })
                    .Get(),
                nullptr);

            frame_obj->add_Destroyed(
                WRL::Callback<ICoreWebView2FrameDestroyedEventHandler>(
                    [weak_ptr](ICoreWebView2Frame* sender, IUnknown* args) {
                      auto it = std::remove_if(
                          weak_ptr->frames.begin(), weak_ptr->frames.end(),
                          [sender](scoped_refptr<FrameData> element) {
                            BOOL is_destroyed = FALSE;
                            element->core_frame->IsDestroyed(&is_destroyed);
                            return is_destroyed;
                          });

                      weak_ptr->frames.erase(it, weak_ptr->frames.end());
                      return S_OK;
                    })
                    .Get(),
                nullptr);

            frame_obj->add_WebMessageReceived(
                WRL::Callback<ICoreWebView2FrameWebMessageReceivedEventHandler>(
                    [frame_weak_ptr = frame->weak_ptr_.GetWeakPtr(), weak_ptr](
                        ICoreWebView2Frame* sender,
                        ICoreWebView2WebMessageReceivedEventArgs* args) {
                      wil::unique_cotaskmem_string src_url = nullptr;
                      args->get_Source(&src_url);
                      wil::unique_cotaskmem_string json_args = nullptr;
                      args->get_WebMessageAsJson(&json_args);

                      weak_ptr->parent->PostEvent(base::BindOnce(
                          [](base::WeakPtr<BrowserData> weak_ptr,
                             base::WeakPtr<FrameData> frame_weak_ptr,
                             LPCSTR source_url, LPCSTR json_args) {
                            weak_ptr->dispatcher->OnReceivedWebMessage(
                                frame_weak_ptr.get(), source_url, json_args);
                          },
                          weak_ptr, frame_weak_ptr, WrapComString(src_url),
                          WrapComString(json_args)));

                      return S_OK;
                    })
                    .Get(),
                nullptr);

            return S_OK;
          })
          .Get(),
      nullptr);

  browser_wrapper->core_webview->add_FaviconChanged(
      WRL::Callback<ICoreWebView2FaviconChangedEventHandler>(
          [weak_ptr](ICoreWebView2* sender, IUnknown* args) {
            weak_ptr->parent->PostEvent(base::BindOnce(
                [](base::WeakPtr<BrowserData> weak_ptr) {
                  wil::unique_cotaskmem_string raw_favicon = nullptr;
                  weak_ptr->core_webview->get_FaviconUri(&raw_favicon);
                  weak_ptr->dispatcher->OnFaviconChanged(
                      WrapComString(raw_favicon));
                },
                weak_ptr));

            return S_OK;
          })
          .Get(),
      nullptr);

  browser_wrapper->core_webview->add_IsDocumentPlayingAudioChanged(
      WRL::Callback<ICoreWebView2IsDocumentPlayingAudioChangedEventHandler>(
          [weak_ptr](ICoreWebView2* sender, IUnknown* args) {
            weak_ptr->parent->PostEvent(base::BindOnce(
                [](base::WeakPtr<BrowserData> weak_ptr) {
                  BOOL is_playing = FALSE;
                  weak_ptr->core_webview->get_IsDocumentPlayingAudio(
                      &is_playing);

                  weak_ptr->dispatcher->OnAudioStateChanged(is_playing);
                },
                weak_ptr));

            return S_OK;
          })
          .Get(),
      nullptr);

  browser_wrapper->core_webview->add_StatusBarTextChanged(
      WRL::Callback<ICoreWebView2StatusBarTextChangedEventHandler>(
          [weak_ptr](ICoreWebView2* sender, IUnknown* args) {
            weak_ptr->parent->PostEvent(base::BindOnce(
                [](base::WeakPtr<BrowserData> weak_ptr) {
                  wil::unique_cotaskmem_string status_text = nullptr;
                  weak_ptr->core_webview->get_StatusBarText(&status_text);

                  weak_ptr->dispatcher->OnStatusTextChanged(
                      WrapComString(status_text));
                },
                weak_ptr));

            return S_OK;
          })
          .Get(),
      nullptr);

  browser_wrapper->core_webview->add_ProcessFailed(
      WRL::Callback<ICoreWebView2ProcessFailedEventHandler>(
          [weak_ptr](ICoreWebView2* sender,
                     ICoreWebView2ProcessFailedEventArgs* args) {
            COREWEBVIEW2_PROCESS_FAILED_KIND kind;
            args->get_ProcessFailedKind(&kind);

            weak_ptr->parent->PostEvent(base::BindOnce(
                [](base::WeakPtr<BrowserData> weak_ptr,
                   COREWEBVIEW2_PROCESS_FAILED_KIND kind) {
                  wil::unique_cotaskmem_string status_text = nullptr;
                  weak_ptr->core_webview->get_StatusBarText(&status_text);

                  weak_ptr->dispatcher->OnProcessFailed(kind);
                },
                weak_ptr, kind));

            return S_OK;
          })
          .Get(),
      nullptr);
}

void BindEventForUpdate(scoped_refptr<BrowserData> browser_wrapper) {
  base::WeakPtr<BrowserData> weak_ptr = browser_wrapper->weak_ptr_.GetWeakPtr();
  browser_wrapper->core_webview->CallDevToolsProtocolMethod(L"DOM.enable",
                                                            L"{}", nullptr);

  // ------------------------ CDP event extensions ------------------------
  // Resource intercept event
  WRL::ComPtr<ICoreWebView2DevToolsProtocolEventReceiver> fetch_receiver;
  browser_wrapper->core_webview->GetDevToolsProtocolEventReceiver(
      L"Fetch.requestPaused", &fetch_receiver);
  fetch_receiver->add_DevToolsProtocolEventReceived(
      WRL::Callback<ICoreWebView2DevToolsProtocolEventReceivedEventHandler>(
          [weak_ptr](
              ICoreWebView2* sender,
              ICoreWebView2DevToolsProtocolEventReceivedEventArgs* args) {
            WRL::ComPtr<ICoreWebView2DevToolsProtocolEventReceivedEventArgs2>
                event_args = nullptr;
            args->QueryInterface<
                ICoreWebView2DevToolsProtocolEventReceivedEventArgs2>(
                &event_args);

            // Serialize json
            wil::unique_cotaskmem_string raw_json = nullptr;
            event_args->get_ParameterObjectAsJson(&raw_json);
            json json_obj = json::parse(Utf8Conv::Utf16ToUtf8(raw_json.get()));

            // Common arguments
            if (json_obj.find("responseStatusCode") != json_obj.end() ||
                json_obj.find("responseHeaders") != json_obj.end()) {
              weak_ptr->parent->PostEvent(base::BindOnce(
                  [](scoped_refptr<BrowserEventDispatcher> dispatcher,
                     json json_obj) {
                    dispatcher->OnResourceReceiveResponse(json_obj);
                  },
                  weak_ptr->dispatcher, std::move(json_obj)));
            } else {
              weak_ptr->parent->PostEvent(base::BindOnce(
                  [](scoped_refptr<BrowserEventDispatcher> dispatcher,
                     json json_obj) {
                    dispatcher->OnResourceRequested(json_obj);
                  },
                  weak_ptr->dispatcher, std::move(json_obj)));
            }

            return S_OK;
          })
          .Get(),
      nullptr);

  // ------------------------ CDP event extensions ------------------------
  // File chooser event
  browser_wrapper->core_webview->CallDevToolsProtocolMethod(L"Page.enable",
                                                            L"{}", nullptr);
  WRL::ComPtr<ICoreWebView2DevToolsProtocolEventReceiver> filechooser_receiver;
  browser_wrapper->core_webview->GetDevToolsProtocolEventReceiver(
      L"Page.fileChooserOpened", &filechooser_receiver);
  filechooser_receiver->add_DevToolsProtocolEventReceived(
      WRL::Callback<ICoreWebView2DevToolsProtocolEventReceivedEventHandler>(
          [weak_ptr](
              ICoreWebView2* sender,
              ICoreWebView2DevToolsProtocolEventReceivedEventArgs* args) {
            WRL::ComPtr<ICoreWebView2DevToolsProtocolEventReceivedEventArgs2>
                event_args = nullptr;
            args->QueryInterface<
                ICoreWebView2DevToolsProtocolEventReceivedEventArgs2>(
                &event_args);

            // Serialize json
            wil::unique_cotaskmem_string raw_json = nullptr;
            event_args->get_ParameterObjectAsJson(&raw_json);
            json json_obj = json::parse(Utf8Conv::Utf16ToUtf8(raw_json.get()));

            weak_ptr->parent->PostEvent(base::BindOnce(
                [](scoped_refptr<BrowserEventDispatcher> dispatcher,
                   json json_obj) {
                  auto frame_id =
                      json_obj["frameId"].template get<std::string>();
                  bool multiselect =
                      json_obj["mode"].template get<std::string>() ==
                      "selectMultiple";
                  int node_id = json_obj["backendNodeId"].template get<int>();

                  dispatcher->OnFileChooserRequested(
                      WrapComString(frame_id.c_str()), multiselect, node_id);
                },
                weak_ptr->dispatcher, std::move(json_obj)));

            return S_OK;
          })
          .Get(),
      nullptr);

  // ------------------------ CDP event extensions ------------------------
  // Console event
  browser_wrapper->core_webview->CallDevToolsProtocolMethod(L"Console.enable",
                                                            L"{}", nullptr);
  WRL::ComPtr<ICoreWebView2DevToolsProtocolEventReceiver> console_receiver;
  browser_wrapper->core_webview->GetDevToolsProtocolEventReceiver(
      L"Console.messageAdded", &console_receiver);
  console_receiver->add_DevToolsProtocolEventReceived(
      WRL::Callback<ICoreWebView2DevToolsProtocolEventReceivedEventHandler>(
          [weak_ptr](
              ICoreWebView2* sender,
              ICoreWebView2DevToolsProtocolEventReceivedEventArgs* args) {
            WRL::ComPtr<ICoreWebView2DevToolsProtocolEventReceivedEventArgs2>
                event_args = nullptr;
            args->QueryInterface<
                ICoreWebView2DevToolsProtocolEventReceivedEventArgs2>(
                &event_args);

            // Serialize json
            wil::unique_cotaskmem_string raw_json = nullptr;
            event_args->get_ParameterObjectAsJson(&raw_json);
            json json_obj = json::parse(Utf8Conv::Utf16ToUtf8(raw_json.get()));

            weak_ptr->parent->PostEvent(base::BindOnce(
                [](scoped_refptr<BrowserEventDispatcher> dispatcher,
                   json json_obj) {
                  dispatcher->OnConsoleMessage(std::move(json_obj));
                },
                weak_ptr->dispatcher, std::move(json_obj)));

            return S_OK;
          })
          .Get(),
      nullptr);
}

namespace {

HWND WINAPI GetWindowHandle(BrowserData* obj) {
  if (!obj->browser_window)
    return nullptr;

  HWND wrapper_widget = obj->browser_window->GetHandle();
  HWND webview2_widget = ::GetWindow(wrapper_widget, GW_CHILD);
  if (!webview2_widget)
    return wrapper_widget;

  HWND chrome_widget = ::GetWindow(webview2_widget, GW_CHILD);
  if (!chrome_widget)
    return webview2_widget;

  return chrome_widget;
}

void WINAPI SetWindowBound(BrowserData* obj, RECT* rt) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, RECT rt) {
        ::MoveWindow(self->browser_window->GetHandle(), rt.left, rt.top,
                     rt.right - rt.left, rt.bottom - rt.top, true);
      },
      scoped_refptr(obj), *rt));
}

void WINAPI SetVisible(BrowserData* obj, BOOL visible) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, BOOL visible) {
        self->core_controller->put_IsVisible(visible);
        ::ShowWindow(self->browser_window->GetHandle(),
                     visible ? SW_SHOW : SW_HIDE);
      },
      scoped_refptr(obj), visible));
}

void WINAPI LoadURL(BrowserData* obj, LPCSTR url) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, std::string url) {
        // Format URL for Navigate
        std::wstring uri(Utf8Conv::Utf8ToUtf16(url.c_str()));
        HRESULT hr = self->core_webview->Navigate(uri.c_str());
        if (hr == E_INVALIDARG) {
          if (uri.find(L' ') == std::wstring::npos &&
              uri.find(L'.') != std::wstring::npos) {
            hr = self->core_webview->Navigate((L"http://" + uri).c_str());
          }
        }
      },
      scoped_refptr(obj), std::string(url)));
}

BOOL WINAPI CanGoBack(BrowserData* obj) {
  BOOL value = FALSE;

  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, scoped_refptr<Semaphore> sync,
         BOOL* value) {
        self->core_webview->get_CanGoBack(value);
        sync->Notify();
      },
      scoped_refptr(obj), obj->parent->semaphore(), &value));
  obj->parent->SyncWaitIfNeed();

  return value;
}

BOOL WINAPI CanGoForward(BrowserData* obj) {
  BOOL value = FALSE;

  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, scoped_refptr<Semaphore> sync,
         BOOL* value) {
        self->core_webview->get_CanGoForward(value);
        sync->Notify();
      },
      scoped_refptr(obj), obj->parent->semaphore(), &value));
  obj->parent->SyncWaitIfNeed();

  return value;
}

void WINAPI GoBack(BrowserData* obj) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self) { self->core_webview->GoBack(); },
      scoped_refptr(obj)));
}

void WINAPI GoForward(BrowserData* obj) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self) { self->core_webview->GoForward(); },
      scoped_refptr(obj)));
}

void WINAPI Reload(BrowserData* obj) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self) { self->core_webview->Reload(); },
      scoped_refptr(obj)));
}

void WINAPI Stop(BrowserData* obj) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self) { self->core_webview->Stop(); },
      scoped_refptr(obj)));
}

using BrowserSettings = struct {
  BOOL enableScripts;
  BOOL enableWebMessage;
  BOOL enableDefaultDialogs;
  BOOL enableStatusBar;
  BOOL enableDevTools;
  BOOL enableDefaultContextMenu;
  BOOL enableHostJSObject;
  BOOL enableZoomControl;
  BOOL enableBuiltInError;
  BOOL enableBrowserAcceleratorKeys;
  BOOL enablePasswdAutoSave;
  BOOL enableAutoFillin;
  BOOL enablePinchZoom;
  BOOL enableSwipeNavigation;
  COREWEBVIEW2_PDF_TOOLBAR_ITEMS hiddenPdfToolbar;
};

void WINAPI GetBrowserSettings(BrowserData* obj, BrowserSettings* data) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, scoped_refptr<Semaphore> sync,
         BrowserSettings* pset) {
        WRL::ComPtr<ICoreWebView2Settings> sets = nullptr;
        self->core_webview->get_Settings(&sets);

        WRL::ComPtr<ICoreWebView2Settings7> target = nullptr;
        sets->QueryInterface<ICoreWebView2Settings7>(&target);

        if (pset) {
          target->get_IsScriptEnabled(&pset->enableScripts);
          target->get_IsWebMessageEnabled(&pset->enableWebMessage);
          target->get_AreDefaultScriptDialogsEnabled(
              &pset->enableDefaultDialogs);
          target->get_IsStatusBarEnabled(&pset->enableStatusBar);
          target->get_AreDevToolsEnabled(&pset->enableDevTools);
          target->get_AreDefaultContextMenusEnabled(
              &pset->enableDefaultContextMenu);
          target->get_AreHostObjectsAllowed(&pset->enableHostJSObject);
          target->get_IsZoomControlEnabled(&pset->enableZoomControl);
          target->get_IsBuiltInErrorPageEnabled(&pset->enableBuiltInError);
          target->get_AreBrowserAcceleratorKeysEnabled(
              &pset->enableBrowserAcceleratorKeys);
          target->get_IsPasswordAutosaveEnabled(&pset->enablePasswdAutoSave);
          target->get_IsGeneralAutofillEnabled(&pset->enableAutoFillin);
          target->get_IsPinchZoomEnabled(&pset->enablePinchZoom);
          target->get_IsSwipeNavigationEnabled(&pset->enableSwipeNavigation);
          target->get_HiddenPdfToolbarItems(&pset->hiddenPdfToolbar);
        }

        sync->Notify();
      },
      scoped_refptr(obj), obj->parent->semaphore(), data));
  obj->parent->SyncWaitIfNeed();
}

void WINAPI SetBrowserSettings(BrowserData* obj, BrowserSettings* data) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, BrowserSettings pset) {
        WRL::ComPtr<ICoreWebView2Settings> settings = nullptr;
        self->core_webview->get_Settings(&settings);

        WRL::ComPtr<ICoreWebView2Settings7> target = nullptr;
        settings->QueryInterface<ICoreWebView2Settings7>(&target);

        target->put_IsScriptEnabled(pset.enableScripts);
        target->put_IsWebMessageEnabled(pset.enableWebMessage);
        target->put_AreDefaultScriptDialogsEnabled(pset.enableDefaultDialogs);
        target->put_IsStatusBarEnabled(pset.enableStatusBar);
        target->put_AreDevToolsEnabled(pset.enableDevTools);
        target->put_AreDefaultContextMenusEnabled(
            pset.enableDefaultContextMenu);
        target->put_AreHostObjectsAllowed(pset.enableHostJSObject);
        target->put_IsZoomControlEnabled(pset.enableZoomControl);
        target->put_IsBuiltInErrorPageEnabled(pset.enableBuiltInError);
        target->put_AreBrowserAcceleratorKeysEnabled(
            pset.enableBrowserAcceleratorKeys);
        target->put_IsPasswordAutosaveEnabled(pset.enablePasswdAutoSave);
        target->put_IsGeneralAutofillEnabled(pset.enableAutoFillin);
        target->put_IsPinchZoomEnabled(pset.enablePinchZoom);
        target->put_IsSwipeNavigationEnabled(pset.enableSwipeNavigation);
        target->put_HiddenPdfToolbarItems(pset.hiddenPdfToolbar);
      },
      scoped_refptr(obj), *data));
}

LPCSTR WINAPI ExecuteJavascript(BrowserData* obj, LPCSTR script) {
  LPCSTR value = FALSE;

  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, scoped_refptr<Semaphore> sync,
         LPCSTR script, LPCSTR* value) {
        self->core_webview->ExecuteScript(
            Utf8Conv::Utf8ToUtf16(script).c_str(),
            WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                [sync, value](HRESULT errorCode, LPCWSTR resultObjectAsJson) {
                  *value = WrapComString(resultObjectAsJson);
                  sync->Notify();
                  return S_OK;
                })
                .Get());
      },
      scoped_refptr(obj), obj->parent->semaphore(), script, &value));
  obj->parent->SyncWaitIfNeed();

  return value;
}

using ExecuteJavascriptCB = void(CALLBACK*)(LPCSTR json, LPVOID param);
void WINAPI ExecuteJavascriptAsync(BrowserData* obj,
                                   LPCSTR script,
                                   ExecuteJavascriptCB callback,
                                   LPVOID param) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, std::string script,
         ExecuteJavascriptCB callback, LPVOID param) {
        self->core_webview->ExecuteScript(
            Utf8Conv::Utf8ToUtf16(script).c_str(),
            WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                [weak_ptr = self->weak_ptr_.GetWeakPtr(), callback, param](
                    HRESULT errorCode, LPCWSTR resultObjectAsJson) {
                  auto json_ret = Utf8Conv::Utf16ToUtf8(resultObjectAsJson);

                  weak_ptr->parent->PostEvent(base::BindOnce(
                      [](const std::string& json_ret,
                         ExecuteJavascriptCB callback,
                         LPVOID param) { callback(json_ret.c_str(), param); },
                      std::move(json_ret), callback, param));

                  return S_OK;
                })
                .Get());
      },
      scoped_refptr(obj), std::string(script), callback, param));
}

void WINAPI CaptureShotsnap(BrowserData* obj,
                            LPBYTE* img_data,
                            int32_t* img_size) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, scoped_refptr<Semaphore> sync,
         LPBYTE* img_data, int32_t* img_size) {
        IStream* is = SHCreateMemStream(nullptr, 0);

        self->core_webview->CapturePreview(
            COREWEBVIEW2_CAPTURE_PREVIEW_IMAGE_FORMAT_PNG, is,
            WRL::Callback<ICoreWebView2CapturePreviewCompletedHandler>(
                [img_data, img_size, is, sync](HRESULT errorCode) {
                  STATSTG stat;
                  is->Stat(&stat, STATFLAG_NONAME);

                  LARGE_INTEGER linfo;
                  linfo.QuadPart = 0;
                  is->Seek(linfo, STREAM_SEEK_SET, NULL);

                  uint32_t size = stat.cbSize.LowPart;
                  uint8_t* buf = static_cast<uint8_t*>(edgeview_MemAlloc(size));
                  ULONG dummy = 0;
                  is->Read(buf, size, &dummy);

                  is->Release();

                  *img_data = buf;
                  *img_size = size;

                  sync->Notify();
                  return S_OK;
                })
                .Get());
      },
      scoped_refptr(obj), obj->parent->semaphore(), img_data, img_size));
  obj->parent->SyncWaitIfNeed();
}

LPCSTR WINAPI GetSourceURL(BrowserData* obj) {
  LPSTR cpp_url = nullptr;

  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, scoped_refptr<Semaphore> sync,
         LPSTR* cpp_url) {
        wil::unique_cotaskmem_string url = nullptr;
        self->core_webview->get_Source(&url);
        *cpp_url = WrapComString(url);

        sync->Notify();
      },
      scoped_refptr(obj), obj->parent->semaphore(), &cpp_url));
  obj->parent->SyncWaitIfNeed();

  return cpp_url;
}

LPCSTR WINAPI GetDocumentTitle(BrowserData* obj) {
  LPSTR cpp_url = nullptr;

  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, scoped_refptr<Semaphore> sync,
         LPSTR* cpp_url) {
        wil::unique_cotaskmem_string url = nullptr;
        self->core_webview->get_DocumentTitle(&url);
        *cpp_url = WrapComString(url);

        sync->Notify();
      },
      scoped_refptr(obj), obj->parent->semaphore(), &cpp_url));
  obj->parent->SyncWaitIfNeed();

  return cpp_url;
}

void WINAPI GetCookieManager(BrowserData* obj, DWORD* retObj) {
  scoped_refptr<CookieManagerData> ckm = new CookieManagerData();

  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> obj, scoped_refptr<Semaphore> sync,
         scoped_refptr<CookieManagerData> ckm) {
        ckm->browser = obj->weak_ptr_.GetWeakPtr();
        obj->core_webview->get_CookieManager(&ckm->core_manager);

        sync->Notify();
      },
      scoped_refptr(obj), obj->parent->semaphore(), ckm));
  obj->parent->SyncWaitIfNeed();

  if (retObj) {
    ckm->AddRef();
    retObj[1] = (DWORD)ckm.get();
    retObj[2] = (DWORD)fnCookieManagerTable;
  }
}

void WINAPI ClearBrowsingData(BrowserData* obj,
                              COREWEBVIEW2_BROWSING_DATA_KINDS kind) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self,
         COREWEBVIEW2_BROWSING_DATA_KINDS kind) {
        WRL::ComPtr<ICoreWebView2Profile> profile = nullptr;
        WRL::ComPtr<ICoreWebView2Profile2> profile2 = nullptr;
        self->core_webview->get_Profile(&profile);
        profile->QueryInterface<ICoreWebView2Profile2>(&profile2);

        profile2->ClearBrowsingData(kind, nullptr);
      },
      scoped_refptr(obj), kind));
}

using PDFPrintSettingsData = struct {
  COREWEBVIEW2_PRINT_ORIENTATION orientation;
  float scale_factor;
  float page_width;
  float page_height;
  float margin_top;
  float margin_bottom;
  float margin_left;
  float margin_right;
  BOOL print_background;
  BOOL only_selection;
  BOOL page_header_footer;
  LPCSTR header_title;
  LPCSTR footer_url;
};

void WINAPI PrintToPDFStream(BrowserData* obj,
                             PDFPrintSettingsData* settings,
                             LPBYTE* img_data,
                             int32_t* img_size) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, scoped_refptr<Semaphore> sync,
         LPBYTE* img_data, int32_t* img_size, PDFPrintSettingsData* settings) {
        WRL::ComPtr<ICoreWebView2PrintSettings> pdf_settings = nullptr;
        self->parent->core_env->CreatePrintSettings(&pdf_settings);

        if (settings) {
          pdf_settings->put_Orientation(settings->orientation);
          pdf_settings->put_ScaleFactor(settings->scale_factor);
          pdf_settings->put_PageWidth(settings->page_width);
          pdf_settings->put_PageHeight(settings->page_height);
          pdf_settings->put_MarginTop(settings->margin_top);
          pdf_settings->put_MarginBottom(settings->margin_bottom);
          pdf_settings->put_MarginLeft(settings->margin_left);
          pdf_settings->put_MarginRight(settings->margin_right);
          pdf_settings->put_ShouldPrintBackgrounds(settings->print_background);
          pdf_settings->put_ShouldPrintSelectionOnly(settings->only_selection);
          pdf_settings->put_ShouldPrintHeaderAndFooter(
              settings->page_header_footer);

          if (settings->header_title && *settings->header_title)
            pdf_settings->put_HeaderTitle(
                Utf8Conv::Utf8ToUtf16(settings->header_title).c_str());
          if (settings->footer_url && *settings->footer_url)
            pdf_settings->put_FooterUri(
                Utf8Conv::Utf8ToUtf16(settings->footer_url).c_str());
        }

        self->core_webview->PrintToPdfStream(
            settings ? pdf_settings.Get() : nullptr,
            WRL::Callback<ICoreWebView2PrintToPdfStreamCompletedHandler>(
                [img_data, img_size, sync](HRESULT errorCode,
                                           IStream* pdfStream) {
                  STATSTG stat;
                  pdfStream->Stat(&stat, STATFLAG_NONAME);

                  LARGE_INTEGER linfo;
                  linfo.QuadPart = 0;
                  pdfStream->Seek(linfo, STREAM_SEEK_SET, NULL);

                  uint32_t size = stat.cbSize.LowPart;
                  uint8_t* buf = static_cast<uint8_t*>(edgeview_MemAlloc(size));
                  ULONG dummy = 0;
                  pdfStream->Read(buf, size, &dummy);

                  *img_data = buf;
                  *img_size = size;

                  sync->Notify();
                  return S_OK;
                })
                .Get());
      },
      scoped_refptr(obj), obj->parent->semaphore(), img_data, img_size,
      settings));
  obj->parent->SyncWaitIfNeed();
}

void WINAPI SetZoomFactor(BrowserData* obj, double* factor) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, double factor) {
        self->core_controller->put_ZoomFactor(factor);
      },
      scoped_refptr(obj), *factor));
}

void WINAPI GetZoomFactor(BrowserData* obj, double* factor) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, scoped_refptr<Semaphore> sync,
         double* factor) {
        self->core_controller->get_ZoomFactor(factor);
        sync->Notify();
      },
      scoped_refptr(obj), obj->parent->semaphore(), factor));
  obj->parent->SyncWaitIfNeed();
}

void WINAPI CloseController(BrowserData* obj) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self) { self->core_controller->Close(); },
      scoped_refptr(obj)));
}

LPCSTR WINAPI AddHookScript(BrowserData* obj, LPCSTR script) {
  LPCSTR cpp_str = nullptr;

  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, scoped_refptr<Semaphore> sync,
         std::string script, LPCSTR* cpp_str) {
        self->core_webview->AddScriptToExecuteOnDocumentCreated(
            Utf8Conv::Utf8ToUtf16(script).c_str(),
            WRL::Callback<
                ICoreWebView2AddScriptToExecuteOnDocumentCreatedCompletedHandler>(
                [sync, cpp_str](HRESULT errorCode, LPCWSTR id) {
                  *cpp_str = WrapComString(id);
                  sync->Notify();
                  return S_OK;
                })
                .Get());
      },
      scoped_refptr(obj), obj->parent->semaphore(), std::string(script),
      &cpp_str));
  obj->parent->SyncWaitIfNeed();

  return cpp_str;
}

void WINAPI OpenDefaultDownloadDialog(BrowserData* obj) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self) {
        self->core_webview->OpenDefaultDownloadDialog();
      },
      scoped_refptr(obj)));
}

void WINAPI OpenTaskManager(BrowserData* obj) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self) {
        self->core_webview->OpenTaskManagerWindow();
      },
      scoped_refptr(obj)));
}

void WINAPI OpenDevtools(BrowserData* obj) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self) {
        self->core_webview->OpenDevToolsWindow();
      },
      scoped_refptr(obj)));
}

void WINAPI SetRequestInterception(BrowserData* obj, BOOL enable) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, BOOL enable) {
        self->core_webview->CallDevToolsProtocolMethod(
            enable ? L"Fetch.enable" : L"Fetch.disable", L"{}", nullptr);
      },
      scoped_refptr(obj), enable));
}

void WINAPI PostWebMessage(BrowserData* obj, LPCSTR arg, BOOL as_json) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, std::string arg, BOOL as_json) {
        if (as_json) {
          self->core_webview->PostWebMessageAsJson(
              Utf8Conv::Utf8ToUtf16(arg).c_str());
        } else {
          self->core_webview->PostWebMessageAsString(
              Utf8Conv::Utf8ToUtf16(arg).c_str());
        }
      },
      scoped_refptr(obj), std::string(arg), as_json));
}

void WINAPI SetFilechooserInfo(BrowserData* obj, LPCSTR files, int backend_id) {
  json args = json::object();
  args["backendNodeId"] = backend_id;

  json file_path = json::array();
  std::vector<std::string> path_list = SplitString(files, "|");
  for (const auto& it : path_list) {
    auto obj = TrimString(it);
    if (!obj.empty())
      file_path.push_back(obj);
  }

  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, json args) {
        self->core_webview->CallDevToolsProtocolMethod(
            L"DOM.setFileInputFiles",
            Utf8Conv::Utf8ToUtf16(args.dump()).c_str(), nullptr);
      },
      scoped_refptr(obj), std::move(args)));
}

void WINAPI SetVirtualHostMapping(BrowserData* obj,
                                  LPCSTR host,
                                  LPCSTR folder_path,
                                  COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND kind) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, std::string host,
         std::string folder_path, COREWEBVIEW2_HOST_RESOURCE_ACCESS_KIND kind) {
        self->core_webview->SetVirtualHostNameToFolderMapping(
            Utf8Conv::Utf8ToUtf16(host).c_str(),
            Utf8Conv::Utf8ToUtf16(folder_path).c_str(), kind);
      },
      scoped_refptr(obj), std::string(host), std::string(folder_path), kind));
}

void WINAPI ClearVirtualHostMapping(BrowserData* obj, LPCSTR host) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, std::string host) {
        self->core_webview->ClearVirtualHostNameToFolderMapping(
            Utf8Conv::Utf8ToUtf16(host).c_str());
      },
      scoped_refptr(obj), std::string(host)));
}

uint32_t WINAPI GetFrameCount(BrowserData* obj) {
  return obj->frames.size();
}

void WINAPI GetFrameAt(BrowserData* obj, int idx, DWORD* retObj) {
  if (idx < 0 || idx >= obj->frames.size())
    return;

  scoped_refptr<FrameData> frame = obj->frames[idx];
  if (retObj) {
    frame->AddRef();
    retObj[1] = (DWORD)frame.get();
    retObj[2] = (DWORD)fnFrameTable;
  }
}

void WINAPI SetDefaultDownloadDialogPos(BrowserData* obj, POINT* pt) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, POINT pt) {
        self->core_webview->put_DefaultDownloadDialogMargin(pt);
      },
      scoped_refptr(obj), *pt));
}

void WINAPI GetDOMOperation(BrowserData* obj, DWORD* retObj) {
  scoped_refptr<DOMOperation> ckm = new DOMOperation();
  ckm->browser = obj->weak_ptr_.GetWeakPtr();

  if (retObj) {
    ckm->AddRef();
    retObj[1] = (DWORD)ckm.get();
    retObj[2] = (DWORD)fnDOMTable;
  }
}

void WINAPI SetAudioMuted(BrowserData* obj, BOOL mute) {
  obj->parent->PostUITask(
      base::BindOnce([](scoped_refptr<BrowserData> self,
                        BOOL mute) { self->core_webview->put_IsMuted(mute); },
                     scoped_refptr(obj), mute));
}

BOOL WINAPI IsAudioMuted(BrowserData* obj) {
  BOOL value = FALSE;

  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, scoped_refptr<Semaphore> sync,
         BOOL* value) {
        self->core_webview->get_IsMuted(value);
        sync->Notify();
      },
      scoped_refptr(obj), obj->parent->semaphore(), &value));
  obj->parent->SyncWaitIfNeed();

  return value;
}

void WINAPI TrySuspendPage(BrowserData* obj) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self) {
        self->core_webview->TrySuspend(nullptr);
      },
      scoped_refptr(obj)));
}

void WINAPI ResumePage(BrowserData* obj) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self) { self->core_webview->Resume(); },
      scoped_refptr(obj)));
}

BOOL WINAPI IsPageSuspended(BrowserData* obj) {
  BOOL value = FALSE;

  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, scoped_refptr<Semaphore> sync,
         BOOL* value) {
        self->core_webview->get_IsSuspended(value);
        sync->Notify();
      },
      scoped_refptr(obj), obj->parent->semaphore(), &value));
  obj->parent->SyncWaitIfNeed();

  return value;
}

void WINAPI SetBackgroundColor(BrowserData* obj, uint32_t argb) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, uint32_t argb) {
        COREWEBVIEW2_COLOR cr;
        *((uint32_t*)&cr) = argb;
        self->core_controller->put_DefaultBackgroundColor(cr);
      },
      scoped_refptr(obj), argb));
}

void WINAPI SetDragAllow(BrowserData* obj, BOOL allow) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, BOOL allow) {
        self->core_controller->put_AllowExternalDrop(allow);
      },
      scoped_refptr(obj), allow));
}

LPCSTR WINAPI GetUserAgent(BrowserData* obj) {
  LPSTR cpp_url = nullptr;

  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, scoped_refptr<Semaphore> sync,
         LPSTR* cpp_url) {
        wil::unique_cotaskmem_string url = nullptr;
        WRL::ComPtr<ICoreWebView2Settings> settings = nullptr;
        WRL::ComPtr<ICoreWebView2Settings7> target_settings = nullptr;
        self->core_webview->get_Settings(&settings);
        settings->QueryInterface<ICoreWebView2Settings7>(&target_settings);

        wil::unique_cotaskmem_string raw_ua = nullptr;
        target_settings->get_UserAgent(&raw_ua);

        *cpp_url = WrapComString(raw_ua);

        sync->Notify();
      },
      scoped_refptr(obj), obj->parent->semaphore(), &cpp_url));
  obj->parent->SyncWaitIfNeed();

  return cpp_url;
}

void WINAPI SetUserAgent(BrowserData* obj, LPCSTR user_agent) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, LPCSTR user_agent) {
        WRL::ComPtr<ICoreWebView2Settings> settings = nullptr;
        WRL::ComPtr<ICoreWebView2Settings7> target_settings = nullptr;
        self->core_webview->get_Settings(&settings);
        settings->QueryInterface<ICoreWebView2Settings7>(&target_settings);

        target_settings->put_UserAgent(
            Utf8Conv::Utf8ToUtf16(user_agent).c_str());
      },
      scoped_refptr(obj), user_agent));
}

void WINAPI SetEmitTouchEvents(BrowserData* obj, BOOL enable, LPCSTR conf) {
  json args;
  args["enabled"] = json::boolean_t(enable);
  if (conf)
    args["configuration"] = std::string(conf);

  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, json args) {
        self->core_webview->CallDevToolsProtocolMethod(
            L"Emulation.setEmitTouchEventsForMouse",
            Utf8Conv::Utf8ToUtf16(args.dump()).c_str(), nullptr);
      },
      scoped_refptr(obj), std::move(args)));
}

using PageViewport = struct {
  float x;
  float y;
  float width;
  float height;
  float scale;
};
void WINAPI SetDeviceMetricsOverride(BrowserData* obj,
                                     int width,
                                     int height,
                                     double* device_factor,
                                     BOOL mobile,
                                     double* scale,
                                     int screen_width,
                                     int screen_height,
                                     int x,
                                     int y,
                                     PageViewport* vp) {
  json args;
  args["width"] = width;
  args["height"] = height;
  args["deviceScaleFactor"] = *device_factor;
  args["mobile"] = json::boolean_t(mobile);
  args["scale"] = *scale;
  args["screenWidth"] = screen_width;
  args["screenHeight"] = screen_height;
  args["positionX"] = x;
  args["positionY"] = y;

  if (vp) {
    json viewport = json::object();
    viewport["x"] = vp->x;
    viewport["y"] = vp->y;
    viewport["width"] = vp->width;
    viewport["height"] = vp->height;
    viewport["scale"] = vp->scale;
    args["viewport"] = viewport;
  }

  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, json args) {
        self->core_webview->CallDevToolsProtocolMethod(
            L"Emulation.setDeviceMetricsOverride",
            Utf8Conv::Utf8ToUtf16(args.dump()).c_str(), nullptr);
      },
      scoped_refptr(obj), std::move(args)));
}

void WINAPI ClearDeviceMetricsOverride(BrowserData* obj) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self) {
        self->core_webview->CallDevToolsProtocolMethod(
            L"Emulation.clearDeviceMetricsOverride", L"{}", nullptr);
      },
      scoped_refptr(obj)));
}

void WINAPI ExecuteScriptCDP(BrowserData* obj,
                             LPCSTR script,
                             RemoteObject* ro) {
  json args;
  args["expression"] = script;
  args["includeCommandLineAPI"] = true;
  args["silent"] = false;
  args["returnByValue"] = false;
  args["userGesture"] = true;
  args["awaitPromise"] = false;

  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, scoped_refptr<Semaphore> sync,
         json args, RemoteObject* ro) {
        self->core_webview->CallDevToolsProtocolMethod(
            L"Runtime.evaluate", Utf8Conv::Utf8ToUtf16(args.dump()).c_str(),
            WRL::Callback<
                ICoreWebView2CallDevToolsProtocolMethodCompletedHandler>(
                [ro, sync = std::move(sync),
                 weak_ptr = self->weak_ptr_.GetWeakPtr()](
                    HRESULT errorCode, LPCWSTR returnObjectAsJson) {
                  json retval =
                      json::parse(Utf8Conv::Utf16ToUtf8(returnObjectAsJson));

                  JSONToRemoteObject(ro, retval["result"]);

                  sync->Notify();
                  return S_OK;
                })
                .Get());
      },
      scoped_refptr(obj), obj->parent->semaphore(), std::move(args), ro));
  obj->parent->SyncWaitIfNeed();
}

using ExecuteScriptCDPCallback = void(CALLBACK*)(LPVOID ptr,
                                                 uint32_t size,
                                                 LPVOID param);
void WINAPI ExecuteScriptCDPAsync(BrowserData* obj,
                                  LPCSTR script,
                                  BOOL await_promise,
                                  ExecuteScriptCDPCallback callback,
                                  LPVOID param) {
  json args;
  args["expression"] = script;
  args["includeCommandLineAPI"] = true;
  args["silent"] = false;
  args["returnByValue"] = false;
  args["userGesture"] = true;
  args["awaitPromise"] = json::boolean_t(await_promise);

  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, json args,
         ExecuteScriptCDPCallback callback, LPVOID param) {
        auto handler = WRL::Callback<
            ICoreWebView2CallDevToolsProtocolMethodCompletedHandler>(
            [callback, param, weak_ptr = self->weak_ptr_.GetWeakPtr()](
                HRESULT errorCode, LPCWSTR returnObjectAsJson) {
              json retval =
                  json::parse(Utf8Conv::Utf16ToUtf8(returnObjectAsJson));

              weak_ptr->parent->PostEvent(base::BindOnce(
                  [](const json& retval, ExecuteScriptCDPCallback callback,
                     LPVOID param) {
                    RemoteObject* ro =
                        (RemoteObject*)edgeview_MemAlloc(sizeof(RemoteObject));
                    JSONToRemoteObject(ro, retval["result"]);

                    callback(ro, sizeof(RemoteObject), param);

                    FreeComString(ro->type);
                    FreeComString(ro->subtype);
                    FreeComString(ro->class_name);
                    FreeComString(ro->raw_value);
                    FreeComString(ro->unserializable_value);
                    FreeComString(ro->description);
                    FreeComString(ro->raw_deepSerializedValue);
                    FreeComString(ro->objectID);

                    edgeview_MemFree(ro);
                  },
                  std::move(retval), callback, param));

              return S_OK;
            });

        self->core_webview->CallDevToolsProtocolMethod(
            L"Runtime.evaluate", Utf8Conv::Utf8ToUtf16(args.dump()).c_str(),
            callback ? handler.Get() : nullptr);
      },
      scoped_refptr(obj), std::move(args), callback, param));
}

LPCSTR WINAPI CallCDPMethod(BrowserData* obj,
                            LPCSTR method,
                            LPCSTR parameter,
                            LPCSTR session) {
  LPCSTR ret_val = nullptr;

  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> obj, scoped_refptr<Semaphore> sync,
         std::string method, std::string parameter, std::string session,
         LPCSTR* ret_val) {
        auto callback = WRL::Callback<
            ICoreWebView2CallDevToolsProtocolMethodCompletedHandler>(
            [sync, ret_val](HRESULT errorCode, LPCWSTR returnObjectAsJson) {
              *ret_val = WrapComString(returnObjectAsJson);

              sync->Notify();
              return S_OK;
            });

        if (session.empty()) {
          obj->core_webview->CallDevToolsProtocolMethod(
              Utf8Conv::Utf8ToUtf16(method).c_str(),
              Utf8Conv::Utf8ToUtf16(parameter).c_str(),

              callback.Get());
        } else {
          obj->core_webview->CallDevToolsProtocolMethodForSession(
              Utf8Conv::Utf8ToUtf16(session).c_str(),
              Utf8Conv::Utf8ToUtf16(method).c_str(),
              Utf8Conv::Utf8ToUtf16(parameter).c_str(), callback.Get());
        }
      },
      scoped_refptr(obj), obj->parent->semaphore(), std::string(method),
      std::string(parameter), session ? std::string(session) : std::string(),
      &ret_val));

  return ret_val;
}

using CallCDPMethodCB = void(CALLBACK*)(LPCSTR json_ret, LPVOID param);
void WINAPI CallCDPMethodAsync(BrowserData* obj,
                               LPCSTR method,
                               LPCSTR parameter,
                               LPCSTR session,
                               CallCDPMethodCB callback,
                               LPVOID param) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> obj, std::string method,
         std::string parameter, std::string session, CallCDPMethodCB callback,
         LPVOID param) {
        auto handler = WRL::Callback<
            ICoreWebView2CallDevToolsProtocolMethodCompletedHandler>(
            [callback, param](HRESULT errorCode, LPCWSTR returnObjectAsJson) {
              auto json_str = Utf8Conv::Utf16ToUtf8(returnObjectAsJson);
              callback(json_str.c_str(), param);

              return S_OK;
            });

        if (session.empty()) {
          obj->core_webview->CallDevToolsProtocolMethod(
              Utf8Conv::Utf8ToUtf16(method).c_str(),
              Utf8Conv::Utf8ToUtf16(parameter).c_str(),
              callback ? handler.Get() : nullptr);
        } else {
          obj->core_webview->CallDevToolsProtocolMethodForSession(
              Utf8Conv::Utf8ToUtf16(session).c_str(),
              Utf8Conv::Utf8ToUtf16(method).c_str(),
              Utf8Conv::Utf8ToUtf16(parameter).c_str(),
              callback ? handler.Get() : nullptr);
        }
      },
      scoped_refptr(obj), std::string(method), std::string(parameter),
      session ? std::string(session) : std::string(), callback, param));
}

using CDPEventReceivedCB = void(CALLBACK*)(LPCSTR json_ret,
                                           LPCSTR session,
                                           LPVOID param);
void WINAPI SetCDPEventReceiver(BrowserData* obj,
                                LPCSTR event_name,
                                CDPEventReceivedCB callback,
                                LPVOID param) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> obj, std::string event_name,
         CDPEventReceivedCB callback, LPVOID param) {
        WRL::ComPtr<ICoreWebView2DevToolsProtocolEventReceiver> receiver =
            nullptr;
        obj->core_webview->GetDevToolsProtocolEventReceiver(
            Utf8Conv::Utf8ToUtf16(event_name).c_str(), &receiver);

        receiver->add_DevToolsProtocolEventReceived(
            WRL::Callback<
                ICoreWebView2DevToolsProtocolEventReceivedEventHandler>(
                [callback, param](
                    ICoreWebView2* sender,
                    ICoreWebView2DevToolsProtocolEventReceivedEventArgs* args) {
                  WRL::ComPtr<
                      ICoreWebView2DevToolsProtocolEventReceivedEventArgs2>
                      args2 = nullptr;
                  args->QueryInterface<
                      ICoreWebView2DevToolsProtocolEventReceivedEventArgs2>(
                      &args2);

                  wil::unique_cotaskmem_string json_raw = nullptr;
                  args2->get_ParameterObjectAsJson(&json_raw);

                  wil::unique_cotaskmem_string session = nullptr;
                  args2->get_SessionId(&session);

                  auto json_str = Utf8Conv::Utf16ToUtf8(json_raw.get());
                  auto session_str = Utf8Conv::Utf16ToUtf8(session.get());
                  callback(json_str.c_str(), session_str.c_str(), param);

                  return S_OK;
                })
                .Get(),
            nullptr);
      },
      scoped_refptr(obj), std::string(event_name), callback, param));
}

void WINAPI SetFilechooserInterception(BrowserData* obj, BOOL enable) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, BOOL enable) {
        json args = json::object();
        args["enabled"] = json::boolean_t(enable);

        self->core_webview->CallDevToolsProtocolMethod(
            L"Page.setInterceptFileChooserDialog",
            Utf8Conv::Utf8ToUtf16(args.dump()).c_str(), nullptr);
      },
      scoped_refptr(obj), enable));
}

void WINAPI LoadBrowserExtension(BrowserData* obj, LPCSTR path, DWORD* retObj) {
  scoped_refptr<ExtensionData> data = new ExtensionData();

  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, scoped_refptr<Semaphore> semaphore,
         std::string path, scoped_refptr<ExtensionData> data) {
        WRL::ComPtr<ICoreWebView2Profile> profile = nullptr;
        self->core_webview->get_Profile(&profile);

        data->parent = self->parent;

        WRL::ComPtr<ICoreWebView2Profile8> profile_ptr = nullptr;
        profile->QueryInterface<ICoreWebView2Profile8>(&profile_ptr);

        profile_ptr->AddBrowserExtension(
            Utf8Conv::Utf8ToUtf16(path).c_str(),
            WRL::Callback<
                ICoreWebView2ProfileAddBrowserExtensionCompletedHandler>(
                [data, semaphore](HRESULT errorCode,
                                  ICoreWebView2BrowserExtension* extension) {
                  if (SUCCEEDED(errorCode))
                    data->core_extension = extension;

                  semaphore->Notify();
                  return S_OK;
                })
                .Get());
      },
      scoped_refptr(obj), obj->parent->semaphore(), std::string(path), data));
  obj->parent->SyncWaitIfNeed();

  if (retObj && data->core_extension.Get()) {
    data->AddRef();
    retObj[1] = (DWORD)data.get();
    retObj[2] = (DWORD)fnExtensionDataTable;
  }
}

LPCSTR WINAPI GetProfileName(BrowserData* obj) {
  LPSTR cpp_url = nullptr;

  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, scoped_refptr<Semaphore> sync,
         LPSTR* cpp_url) {
        WRL::ComPtr<ICoreWebView2Profile> profile = nullptr;
        self->core_webview->get_Profile(&profile);

        wil::unique_cotaskmem_string url = nullptr;
        profile->get_ProfileName(&url);

        *cpp_url = WrapComString(url);

        sync->Notify();
      },
      scoped_refptr(obj), obj->parent->semaphore(), &cpp_url));
  obj->parent->SyncWaitIfNeed();

  return cpp_url;
}

void WINAPI RemoveHOOKScript(BrowserData* obj, LPCSTR id) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BrowserData> self, std::string str) {
        self->core_webview->RemoveScriptToExecuteOnDocumentCreated(
            Utf8Conv::Utf8ToUtf16(str).c_str());
      },
      scoped_refptr(obj), std::string(id)));
}

}  // namespace

DWORD fnBrowserTable[] = {
    (DWORD)GetWindowHandle,
    (DWORD)SetWindowBound,
    (DWORD)SetVisible,
    (DWORD)LoadURL,
    (DWORD)CanGoBack,
    (DWORD)CanGoForward,
    (DWORD)GoBack,
    (DWORD)GoForward,
    (DWORD)Reload,
    (DWORD)Stop,
    (DWORD)GetBrowserSettings,
    (DWORD)SetBrowserSettings,
    (DWORD)ExecuteJavascript,
    (DWORD)ExecuteJavascriptAsync,
    (DWORD)CaptureShotsnap,
    (DWORD)GetSourceURL,
    (DWORD)GetDocumentTitle,
    (DWORD)GetCookieManager,
    (DWORD)ClearBrowsingData,
    (DWORD)PrintToPDFStream,
    (DWORD)SetZoomFactor,
    (DWORD)GetZoomFactor,
    (DWORD)CloseController,
    (DWORD)AddHookScript,
    (DWORD)OpenDefaultDownloadDialog,
    (DWORD)OpenTaskManager,
    (DWORD)OpenDevtools,
    (DWORD)SetRequestInterception,
    (DWORD)PostWebMessage,
    (DWORD)SetFilechooserInfo,
    (DWORD)SetVirtualHostMapping,
    (DWORD)ClearVirtualHostMapping,
    (DWORD)GetFrameCount,
    (DWORD)GetFrameAt,
    (DWORD)SetDefaultDownloadDialogPos,
    (DWORD)GetDOMOperation,
    (DWORD)SetAudioMuted,
    (DWORD)IsAudioMuted,
    (DWORD)TrySuspendPage,
    (DWORD)ResumePage,
    (DWORD)IsPageSuspended,
    (DWORD)SetBackgroundColor,
    (DWORD)SetDragAllow,
    (DWORD)GetUserAgent,
    (DWORD)SetUserAgent,
    (DWORD)SetEmitTouchEvents,
    (DWORD)SetDeviceMetricsOverride,
    (DWORD)ClearDeviceMetricsOverride,
    (DWORD)ExecuteScriptCDP,
    (DWORD)ExecuteScriptCDPAsync,
    (DWORD)CallCDPMethod,
    (DWORD)CallCDPMethodAsync,
    (DWORD)SetCDPEventReceiver,
    (DWORD)SetFilechooserInterception,
    (DWORD)LoadBrowserExtension,
    (DWORD)GetProfileName,
    (DWORD)RemoveHOOKScript,
};  // namespace edgeview

namespace {

LPCSTR WINAPI GetNewWindowURL(NewWindowDelegate* obj) {
  LPSTR cpp_url = nullptr;

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<NewWindowDelegate> self, scoped_refptr<Semaphore> sync,
         LPSTR* cpp_url) {
        wil::unique_cotaskmem_string url = nullptr;
        self->core_newwindow->get_Uri(&url);
        *cpp_url = WrapComString(url);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), &cpp_url));
  obj->browser->parent->SyncWaitIfNeed();

  return cpp_url;
}

BOOL WINAPI GetIsUserGesture(NewWindowDelegate* obj) {
  BOOL value = FALSE;

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<NewWindowDelegate> self, scoped_refptr<Semaphore> sync,
         BOOL* value) {
        self->core_newwindow->get_IsUserInitiated(value);
        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), &value));
  obj->browser->parent->SyncWaitIfNeed();

  return value;
}

using WindowFeaturesData = struct {
  BOOL hasPos;
  BOOL hasSize;
  UINT32 left;
  UINT32 top;
  UINT32 height;
  UINT32 width;
  BOOL showMenu;
  BOOL showStatus;
  BOOL showToolbar;
  BOOL showScrollbar;
};

void WINAPI GetWindowFeatures(NewWindowDelegate* obj,
                              WindowFeaturesData* data) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<NewWindowDelegate> self, scoped_refptr<Semaphore> sync,
         WindowFeaturesData* data) {
        WRL::ComPtr<ICoreWebView2WindowFeatures> winfeatures;
        self->core_newwindow->get_WindowFeatures(&winfeatures);

        if (data) {
          winfeatures->get_HasPosition(&data->hasPos);
          winfeatures->get_HasSize(&data->hasSize);
          winfeatures->get_Left(&data->left);
          winfeatures->get_Top(&data->top);
          winfeatures->get_Height(&data->height);
          winfeatures->get_Width(&data->width);
          winfeatures->get_ShouldDisplayMenuBar(&data->showMenu);
          winfeatures->get_ShouldDisplayStatus(&data->showStatus);
          winfeatures->get_ShouldDisplayToolbar(&data->showToolbar);
          winfeatures->get_ShouldDisplayScrollBars(&data->showScrollbar);
        }

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), data));
  obj->browser->parent->SyncWaitIfNeed();
}

void WINAPI SetNewWindow(NewWindowDelegate* obj, BrowserData* browser) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<NewWindowDelegate> self,
         scoped_refptr<BrowserData> browser) {
        self->core_newwindow->put_NewWindow(browser->core_webview.Get());
      },
      scoped_refptr(obj), scoped_refptr(browser)));
}

void WINAPI SetHandled(NewWindowDelegate* obj, BOOL handled) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<NewWindowDelegate> self, BOOL handled) {
        self->core_newwindow->put_Handled(handled);
        self->internal_deferral->Complete();

        // Update new window event binding
        BindEventForUpdate(self->browser.get());
      },
      scoped_refptr(obj), handled));
}

}  // namespace

DWORD fnNewWindowDelegateTable[] = {
    (DWORD)GetNewWindowURL, (DWORD)GetIsUserGesture, (DWORD)GetWindowFeatures,
    (DWORD)SetNewWindow,    (DWORD)SetHandled,
};  // namespace edgeview

namespace {

void WINAPI ScriptDialogAccept(ScriptDialogDelegate* obj) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ScriptDialogDelegate> obj) {
        obj->core_dialog->Accept();
      },
      scoped_refptr(obj)));
}

void WINAPI ScriptDialogSetInput(ScriptDialogDelegate* obj, LPCSTR result) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ScriptDialogDelegate> obj, std::wstring input) {
        obj->core_dialog->put_ResultText(input.c_str());
      },
      scoped_refptr(obj), Utf8Conv::Utf8ToUtf16(result)));
}

void WINAPI ScriptDialogProcess(ScriptDialogDelegate* obj) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ScriptDialogDelegate> obj) {
        obj->internal_deferral->Complete();
      },
      scoped_refptr(obj)));
}

}  // namespace

DWORD fnScriptDialogDelegateTable[] = {
    (DWORD)ScriptDialogAccept,
    (DWORD)ScriptDialogSetInput,
    (DWORD)ScriptDialogProcess,
};

void WINAPI SetPermissionState(PermissionDelegate* obj,
                               COREWEBVIEW2_PERMISSION_STATE state) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<PermissionDelegate> obj,
         COREWEBVIEW2_PERMISSION_STATE state) {
        obj->core_delegate->put_State(state);
      },
      scoped_refptr(obj), state));
}

void WINAPI PermissionProcess(PermissionDelegate* obj, BOOL handled) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<PermissionDelegate> obj, BOOL handled) {
        obj->core_delegate->put_Handled(handled);
        obj->internal_deferral->Complete();
      },
      scoped_refptr(obj), handled));
}

DWORD fnPermissionDelegateTable[] = {
    (DWORD)SetPermissionState,
    (DWORD)PermissionProcess,
};

}  // namespace edgeview
