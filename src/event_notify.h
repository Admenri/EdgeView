#pragma once

#include "base/memory/ref_counted.h"
#include "edgeview_data.h"
#include "ev_network.h"
#include "util.h"

namespace edgeview {

struct BrowserData;
struct NewWindowDelegate;
struct StrMapElement;
struct ScriptDialogDelegate;
struct ContextMenuParams;
struct ContextMenuItem;
struct PermissionDelegate;
struct BasicAuthenticationCallback;
struct DownloadConfirm;
struct FrameData;

class BrowserEventDispatcher : public base::RefCounted<BrowserEventDispatcher> {
 public:
  BrowserEventDispatcher(base::WeakPtr<BrowserData> browser, LPVOID callback);
  ~BrowserEventDispatcher();

  BrowserEventDispatcher(const BrowserEventDispatcher&) = delete;
  BrowserEventDispatcher& operator=(const BrowserEventDispatcher&) = delete;

  void OnCreated();
  void OnCloseRequested();
  void OnNewWindowRequested(scoped_refptr<NewWindowDelegate> delegate);
  void OnDocumentTitleChanged(LPCSTR title);
  void OnFullscreenModeChanged(BOOL fullscreen);
  BOOL OnBeforeNavigation(LPCSTR url, BOOL user_gesture, BOOL is_redirect,
                          LPCSTR headers, uint64_t nav_id);
  void OnContentLoading(BOOL error_page, uint64_t nav_id);
  void OnSourceChanged(BOOL new_document);
  void OnHistoryChanged();
  void OnNavigationComplete(BOOL success, int error_status, uint64_t nav_id);
  void OnScriptDialogRequested(LPCSTR url, int kind, LPCSTR message,
                               LPCSTR deftext,
                               scoped_refptr<ScriptDialogDelegate> delegate);
  void OnContextMenuRequested(scoped_refptr<ContextMenuParams> params);
  void OnContextMenuExecute(scoped_refptr<ContextMenuItem> item);
  void OnPermissionRequested(LPCSTR url, int kind, BOOL user_gesture,
                             scoped_refptr<PermissionDelegate> delegate);

  void OnResourceRequested(json request_parameter);
  void OnResourceReceiveResponse(json request_parameter);

  BOOL OnKeyEvent(COREWEBVIEW2_KEY_EVENT_KIND kind, uint32_t virtual_key,
                  int lparam, COREWEBVIEW2_PHYSICAL_KEY_STATUS* status);
  void BasicAuthRequested(LPCSTR url, LPCSTR challenge,
                          scoped_refptr<BasicAuthenticationCallback> callback);
  void OnReceivedWebMessage(scoped_refptr<FrameData> frame, LPCSTR source_url,
                            LPCSTR json_args);

  void OnFileChooserRequested(LPCSTR frame_id, BOOL multiselect, int node_id);
  void OnConsoleMessage(json console_event);

  void OnBeforeDownload(scoped_refptr<DownloadConfirm> confirm);
  void OnFaviconChanged(LPCSTR favicon);
  void OnAudioStateChanged(BOOL audible);
  void OnStatusTextChanged(LPCSTR status);

 private:
  base::WeakPtr<BrowserData> self;
  LPVOID ecallback = nullptr;
};

}  // namespace edgeview
