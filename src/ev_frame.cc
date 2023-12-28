#include "ev_frame.h"

#include "edgeview_data.h"

namespace edgeview {

namespace {

LPCSTR WINAPI GetName(FrameData* obj) {
  LPSTR cpp_url = nullptr;

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<FrameData> self, scoped_refptr<Semaphore> sync,
         LPSTR* cpp_url) {
        wil::unique_cotaskmem_string url = nullptr;
        self->core_frame->get_Name(&url);
        *cpp_url = WrapComString(url);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), &cpp_url));
  obj->browser->parent->SyncWaitIfNeed();

  return cpp_url;
}

LPCSTR WINAPI GetURL(FrameData* obj) { return WrapComString(obj->url.c_str()); }

LPCSTR WINAPI ExecuteJavascript(FrameData* obj, LPCSTR script) {
  LPCSTR value = FALSE;

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<FrameData> self, scoped_refptr<Semaphore> sync,
         LPCSTR script, LPCSTR* value) {
        self->core_frame->ExecuteScript(
            Utf8Conv::Utf8ToUtf16(script).c_str(),
            WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                [sync, value](HRESULT errorCode, LPCWSTR resultObjectAsJson) {
                  *value = WrapComString(resultObjectAsJson);
                  sync->Notify();
                  return S_OK;
                })
                .Get());
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), script, &value));
  obj->browser->parent->SyncWaitIfNeed();

  return value;
}

using ExecuteJavascriptCB = void(CALLBACK*)(LPCSTR json, LPVOID param);
void WINAPI ExecuteJavascriptAsync(FrameData* obj, LPCSTR script,
                                   ExecuteJavascriptCB callback, LPVOID param) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<FrameData> self, std::string script,
         ExecuteJavascriptCB callback, LPVOID param) {
        self->core_frame->ExecuteScript(
            Utf8Conv::Utf8ToUtf16(script).c_str(),
            WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                [weak_ptr = self->browser, callback, param](
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

void WINAPI PostWebMessage(FrameData* obj, LPCSTR arg, BOOL as_json) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<FrameData> self, std::string arg, BOOL as_json) {
        if (as_json) {
          self->core_frame->PostWebMessageAsJson(
              Utf8Conv::Utf8ToUtf16(arg).c_str());
        } else {
          self->core_frame->PostWebMessageAsString(
              Utf8Conv::Utf8ToUtf16(arg).c_str());
        }
      },
      scoped_refptr(obj), std::string(arg), as_json));
}

}  // namespace

DWORD fnFrameTable[] = {
    (DWORD)GetName,           (DWORD)GetURL,
    (DWORD)ExecuteJavascript, (DWORD)ExecuteJavascriptAsync,
    (DWORD)PostWebMessage,
};

}  // namespace edgeview
