#include "ev_dom.h"

#include "edgeview_data.h"
#include "modp_b64.h"

namespace edgeview {

static void ExecuteScriptAsync(scoped_refptr<BrowserData> browser,
                               const std::string& script) {
  // Force async task post
  browser->parent->PostEvent(base::BindOnce(
      [](scoped_refptr<BrowserData> obj, const std::string& script) {
        obj->core_webview->ExecuteScript(Utf8Conv::Utf8ToUtf16(script).c_str(),
                                         nullptr);
      },
      browser, script));
}

static json ExecuteScriptSync(scoped_refptr<BrowserData> browser,
                              const std::string& script) {
  json ret_obj = json::object();

  // Force async task post
  browser->parent->PostEvent(base::BindOnce(
      [](scoped_refptr<BrowserData> obj, scoped_refptr<Semaphore> sync,
         const std::string& script, json* ret_obj) {
        obj->core_webview->ExecuteScript(
            Utf8Conv::Utf8ToUtf16(script).c_str(),
            WRL::Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
                [ret_obj, sync](HRESULT errorCode, LPCWSTR resultObjectAsJson) {
                  if (SUCCEEDED(errorCode)) {
                    *ret_obj =
                        json::parse(Utf8Conv::Utf16ToUtf8(resultObjectAsJson));
                  }

                  sync->Notify();
                  return S_OK;
                })
                .Get());
      },
      browser, browser->parent->semaphore(), script, &ret_obj));
  browser->parent->SyncWaitIfNeed();

  return ret_obj;
}

static void CallCDPMethodAsync(scoped_refptr<BrowserData> browser,
                               const std::string& method,
                               const json& args) {
  // Force async task post
  browser->parent->PostEvent(base::BindOnce(
      [](scoped_refptr<BrowserData> obj, const std::string& method,
         const json& args) {
        obj->core_webview->CallDevToolsProtocolMethod(
            Utf8Conv::Utf8ToUtf16(method).c_str(),
            Utf8Conv::Utf8ToUtf16(args.dump()).c_str(), nullptr);
      },
      browser, method, std::move(args)));
}

static json CallCDPMethodSync(scoped_refptr<BrowserData> browser,
                              const std::string& method,
                              const json& args) {
  json ret_obj;

  // Force async task post
  browser->parent->PostEvent(base::BindOnce(
      [](scoped_refptr<BrowserData> obj, scoped_refptr<Semaphore> sync,
         const std::string& method, const json& args, json* ret_obj) {
        obj->core_webview->CallDevToolsProtocolMethod(
            Utf8Conv::Utf8ToUtf16(method).c_str(),
            Utf8Conv::Utf8ToUtf16(args.dump()).c_str(),
            WRL::Callback<
                ICoreWebView2CallDevToolsProtocolMethodCompletedHandler>(
                [ret_obj, sync](HRESULT errorCode, LPCWSTR resultObjectAsJson) {
                  if (SUCCEEDED(errorCode)) {
                    *ret_obj =
                        json::parse(Utf8Conv::Utf16ToUtf8(resultObjectAsJson));
                  }

                  sync->Notify();
                  return S_OK;
                })
                .Get());
      },
      browser, browser->parent->semaphore(), method, std::move(args),
      &ret_obj));
  browser->parent->SyncWaitIfNeed();

  return ret_obj;
}

namespace {

void WINAPI Element_Click(DOMOperation* obj, LPCSTR selector, int index) {
  ExecuteScriptAsync(
      obj->browser.get(),
      std::format("document.querySelectorAll(\"{}\")[{}].click();",
                  std::string(selector), index));
}

void WINAPI Element_CustomEvent(DOMOperation* obj,
                                LPCSTR selector,
                                int index,
                                LPCSTR event_name) {
  ExecuteScriptAsync(
      obj->browser.get(),
      std::format(
          "document.querySelectorAll(\"{}\")[{}].dispatchEvent(new "
          "Event(\"{}\", {\"bubbles\": true, \"cancelable\": false }));",
          std::string(selector), index, std::string(event_name)));
}

LPCSTR WINAPI Element_Get_InnerText(DOMOperation* obj,
                                    LPCSTR selector,
                                    int index) {
  auto ret = ExecuteScriptSync(
      obj->browser.get(),
      std::format("document.querySelectorAll(\"{}\")[{}].innerText",
                  std::string(selector), index));

  std::string ret_val;
  if (ret.type() == json::value_t::string)
    ret.get_to(ret_val);

  return WrapComString(ret_val.c_str());
}

void WINAPI Element_Put_InnerText(DOMOperation* obj,
                                  LPCSTR selector,
                                  int index,
                                  LPCSTR text) {
  ExecuteScriptAsync(
      obj->browser.get(),
      std::format("document.querySelectorAll(\"{}\")[{}].innerText = \"{}\"; ",
                  std::string(selector), index, std::string(text)));
}

LPCSTR WINAPI Element_Get_InnerHTML(DOMOperation* obj,
                                    LPCSTR selector,
                                    int index) {
  auto ret = ExecuteScriptSync(
      obj->browser.get(),
      std::format("document.querySelectorAll(\"{}\")[{}].innerHTML",
                  std::string(selector), index));

  std::string ret_val;
  if (ret.type() == json::value_t::string)
    ret.get_to(ret_val);

  return WrapComString(ret_val.c_str());
}

void WINAPI Element_Put_InnerHTML(DOMOperation* obj,
                                  LPCSTR selector,
                                  int index,
                                  LPCSTR text) {
  ExecuteScriptAsync(
      obj->browser.get(),
      std::format("document.querySelectorAll(\"{}\")[{}].innerHTML = \"{}\"; ",
                  std::string(selector), index, std::string(text)));
}

int WINAPI Element_QuerySelector(DOMOperation* obj, int node, LPCSTR selector) {
  json args;
  args["nodeId"] = node;
  args["selector"] = selector;

  auto ret = CallCDPMethodSync(obj->browser.get(), "DOM.querySelector",
                               std::move(args));

  return ret["nodeId"].template get<int>();
}

void WINAPI Element_QuerySelectorAll(DOMOperation* obj,
                                     int node,
                                     LPCSTR selector,
                                     LPVOID* mem,
                                     uint32_t* size) {
  json args;
  args["nodeId"] = node;
  args["selector"] = selector;

  auto ret = CallCDPMethodSync(obj->browser.get(), "DOM.querySelectorAll",
                               std::move(args));
  auto ary = ret.array();

  if (ary.size()) {
    *mem = edgeview_MemAlloc(ary.size() * sizeof(DWORD));
    *size = ary.size();
    for (size_t i = 0; i < ary.size(); ++i) {
      *(((LPINT)*mem) + i) = ary[i].template get<int>();
    }
  }
}

int WINAPI Element_GetDocument(DOMOperation* obj) {
  auto ret = CallCDPMethodSync(obj->browser.get(), "DOM.getDocument", json());

  return ret["root"]["nodeId"].template get<int>();
}

LPCSTR WINAPI Element_Get_OuterText(DOMOperation* obj,
                                    LPCSTR selector,
                                    int index) {
  auto ret = ExecuteScriptSync(
      obj->browser.get(),
      std::format("document.querySelectorAll(\"{}\")[{}].outerText",
                  std::string(selector), index));

  std::string ret_val;
  if (ret.type() == json::value_t::string)
    ret.get_to(ret_val);

  return WrapComString(ret_val.c_str());
}

LPCSTR WINAPI Element_Get_OuterHTML(DOMOperation* obj,
                                    LPCSTR selector,
                                    int index) {
  auto ret = ExecuteScriptSync(
      obj->browser.get(),
      std::format("document.querySelectorAll(\"{}\")[{}].outerHTML",
                  std::string(selector), index));

  std::string ret_val;
  if (ret.type() == json::value_t::string)
    ret.get_to(ret_val);

  return WrapComString(ret_val.c_str());
}

LPCSTR WINAPI Element_Get_Value(DOMOperation* obj, LPCSTR selector, int index) {
  auto ret = ExecuteScriptSync(
      obj->browser.get(),
      std::format("document.querySelectorAll(\"{}\")[{}].value",
                  std::string(selector), index));

  std::string ret_val;
  if (ret.type() == json::value_t::string)
    ret.get_to(ret_val);

  return WrapComString(ret_val.c_str());
}

void WINAPI Element_Put_Value(DOMOperation* obj,
                              LPCSTR selector,
                              int index,
                              LPCSTR text) {
  ExecuteScriptAsync(
      obj->browser.get(),
      std::format("document.querySelectorAll(\"{}\")[{}].value = \"{}\"; ",
                  std::string(selector), index, std::string(text)));
}

LPCSTR WINAPI Element_Get_Attribute(DOMOperation* obj,
                                    LPCSTR selector,
                                    int index,
                                    LPCSTR attr) {
  auto ret = ExecuteScriptSync(
      obj->browser.get(),
      std::format("document.querySelectorAll(\"{}\")[{}].getAttribute(\"{}\")",
                  std::string(selector), index, std::string(attr)));

  std::string ret_val;
  if (ret.type() == json::value_t::string)
    ret.get_to(ret_val);

  return WrapComString(ret_val.c_str());
}

void WINAPI Element_Put_Attribute(DOMOperation* obj,
                                  LPCSTR selector,
                                  int index,
                                  LPCSTR attr,
                                  LPCSTR text) {
  ExecuteScriptAsync(obj->browser.get(),
                     std::format("document.querySelectorAll(\"{}\")[{}]."
                                 "setAttribute(\"{}\", \"{}\"); ",
                                 std::string(selector), index,
                                 std::string(attr), std::string(text)));
}

BOOL WINAPI Element_Get_Checked(DOMOperation* obj, LPCSTR selector, int index) {
  auto ret = ExecuteScriptSync(
      obj->browser.get(),
      std::format("document.querySelectorAll(\"{}\")[{}].checked == true",
                  std::string(selector), index));

  bool ret_val = false;
  if (ret.type() == json::value_t::boolean)
    ret.get_to(ret_val);

  return ret_val;
}

void WINAPI Element_Put_Checked(DOMOperation* obj,
                                LPCSTR selector,
                                int index,
                                BOOL checked) {
  ExecuteScriptAsync(
      obj->browser.get(),
      std::format("document.querySelectorAll(\"{}\")[{}].checked = {};",
                  std::string(selector), index,
                  checked ? std::string("true") : std::string("false")));
}

void WINAPI Element_Remove_Attribute(DOMOperation* obj,
                                     LPCSTR selector,
                                     int index,
                                     LPCSTR attr) {
  ExecuteScriptAsync(
      obj->browser.get(),
      std::format(
          "document.querySelectorAll(\"{}\")[{}].removeAttribute(\"{}\");",
          std::string(selector), index, std::string(attr)));
}

void WINAPI Element_SetScrollPos(DOMOperation* obj,
                                 LPCSTR selector,
                                 int index,
                                 int delx,
                                 int dely) {
  ExecuteScriptAsync(
      obj->browser.get(),
      std::format("document.querySelectorAll(\"{}\")[{}].scrollTo({}, {});",
                  std::string(selector), index, delx, dely));
}

void WINAPI Element_GetCanvasData(DOMOperation* obj,
                                  LPCSTR selector,
                                  int index,
                                  LPVOID* ptr,
                                  uint32_t* size) {
  auto ret = ExecuteScriptSync(
      obj->browser.get(),
      std::format(
          "document.querySelectorAll(\"{}\")[{}].toDataURL('image/png')",
          std::string(selector), index));

  if (ret.type() == json::value_t::string) {
    auto b64_str = ret.template get<std::string>();
    auto b64_data = b64_str.substr(b64_str.find(',') + 1, b64_str.size());

    auto raw_data = modp_b64_decode(b64_data);

    *ptr = edgeview_MemAlloc(raw_data.size());
    memcpy(*ptr, raw_data.data(), raw_data.size());
    *size = raw_data.size();
  }
}

void WINAPI Element_SetFocusState(DOMOperation* obj,
                                  LPCSTR selector,
                                  int index,
                                  BOOL focus) {
  ExecuteScriptAsync(
      obj->browser.get(),
      std::format("document.querySelectorAll(\"{}\")[{}].{}();",
                  std::string(selector), index,
                  focus ? std::string("focus") : std::string("blur")));
}

}  // namespace

DWORD fnDOMTable[] = {
    (DWORD)Element_Click,         (DWORD)Element_CustomEvent,
    (DWORD)Element_Get_InnerText, (DWORD)Element_Put_InnerText,
    (DWORD)Element_QuerySelector, (DWORD)Element_QuerySelectorAll,
    (DWORD)Element_GetDocument,   (DWORD)Element_Get_InnerHTML,
    (DWORD)Element_Put_InnerHTML, (DWORD)Element_Get_OuterText,
    (DWORD)Element_Get_OuterHTML, (DWORD)Element_Get_Value,
    (DWORD)Element_Put_Value,     (DWORD)Element_Get_Attribute,
    (DWORD)Element_Put_Attribute, (DWORD)Element_Get_Checked,
    (DWORD)Element_Put_Checked,   (DWORD)Element_Remove_Attribute,
    (DWORD)Element_SetScrollPos,  (DWORD)Element_GetCanvasData,
    (DWORD)Element_SetFocusState,
};

}  // namespace edgeview
