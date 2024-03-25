#include "ev_network.h"

#include "edgeview_data.h"
#include "modp_b64.h"

namespace edgeview {

namespace {

void ExtractKeyValue(const std::string& str,
                     std::string& key,
                     std::string& value) {
  size_t delimiterPos = str.find(':');

  if (delimiterPos != std::string::npos) {
    key = str.substr(0, delimiterPos);
    value = str.substr(delimiterPos + 1);
  }
}

}  // namespace

using CookieData = struct {
  LPCSTR name;
  LPCSTR value;
  LPCSTR domain;
  LPCSTR path;
  double expires;
  BOOL http_only;
  COREWEBVIEW2_COOKIE_SAME_SITE_KIND same_site;
  BOOL is_secure;
  BOOL is_session;
};

static void TransferCookie(WRL::ComPtr<ICoreWebView2Cookie> from,
                           CookieData* to) {
  wil::unique_cotaskmem_string value = nullptr;

  from->get_Name(&value);
  to->name = WrapComString(value);
  from->get_Value(&value);
  to->value = WrapComString(value);
  from->get_Domain(&value);
  to->domain = WrapComString(value);
  from->get_Path(&value);
  to->path = WrapComString(value);

  from->get_Expires(&to->expires);
  from->get_IsHttpOnly(&to->http_only);
  from->get_SameSite(&to->same_site);
  from->get_IsSecure(&to->is_secure);
  from->get_IsSession(&to->is_session);
}

static void TransferCookie(CookieData* from,
                           WRL::ComPtr<ICoreWebView2Cookie> to) {
  to->put_Value(Utf8Conv::Utf8ToUtf16(from->value).c_str());

  to->put_Expires(from->expires);
  to->put_IsHttpOnly(from->http_only);
  to->put_SameSite(from->same_site);
  to->put_IsSecure(from->is_secure);
}

void WINAPI GetCookies(CookieManagerData* obj, LPCSTR url, LPVOID* ary) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<CookieManagerData> obj, scoped_refptr<Semaphore> sync,
         LPCSTR url, LPVOID* ary) {
        obj->core_manager->GetCookies(
            Utf8Conv::Utf8ToUtf16(url).c_str(),
            WRL::Callback<ICoreWebView2GetCookiesCompletedHandler>(
                [sync, ary](HRESULT result,
                            ICoreWebView2CookieList* cookieList) {
                  FreeAryElement(*ary);

                  uint32_t cookie_size = 0;
                  cookieList->get_Count(&cookie_size);
                  std::vector<WRL::ComPtr<ICoreWebView2Cookie>> cookies;
                  for (uint32_t i = 0; i < cookie_size; ++i) {
                    WRL::ComPtr<ICoreWebView2Cookie> cookie;
                    cookieList->GetValueAtIndex(i, &cookie);
                    cookies.push_back(std::move(cookie));
                  }

                  DWORD* pStrs = new DWORD[cookies.size()];
                  for (size_t i = 0; i < cookies.size(); i++) {
                    CookieData* pNewClass = static_cast<CookieData*>(
                        edgeview_MemAlloc(sizeof(CookieData) * cookies.size()));
                    TransferCookie(cookies[i], pNewClass);
                    pStrs[i] = (DWORD)pNewClass;
                  }

                  int nSize = cookies.size() * sizeof(DWORD);
                  LPSTR pAry =
                      (LPSTR)edgeview_MemAlloc(sizeof(INT) * 2 + nSize);
                  *(LPINT)pAry = 1;
                  *(LPINT)(pAry + sizeof(INT)) = cookies.size();
                  memcpy(pAry + sizeof(INT) * 2, pStrs, nSize);
                  delete[] pStrs;

                  *ary = pAry;

                  sync->Notify();
                  return S_OK;
                })
                .Get());
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), url, ary));
  obj->browser->parent->SyncWaitIfNeed();
}

void WINAPI AddOrUpdateCookie(CookieManagerData* obj, CookieData* data) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<CookieManagerData> self, scoped_refptr<Semaphore> sync,
         CookieData* data) {
        WRL::ComPtr<ICoreWebView2Cookie> cookie = nullptr;
        self->core_manager->CreateCookie(
            Utf8Conv::Utf8ToUtf16(data->name).c_str(),
            Utf8Conv::Utf8ToUtf16(data->value).c_str(),
            Utf8Conv::Utf8ToUtf16(data->domain).c_str(),
            Utf8Conv::Utf8ToUtf16(data->path).c_str(), &cookie);
        TransferCookie(data, cookie);
        self->core_manager->AddOrUpdateCookie(cookie.Get());

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), data));
  obj->browser->parent->SyncWaitIfNeed();
}

void WINAPI DeleteCookie(CookieManagerData* obj, LPCSTR name, LPCSTR url) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<CookieManagerData> self, scoped_refptr<Semaphore> sync,
         std::string name, std::string url) {
        if (name.empty() && url.empty())
          self->core_manager->DeleteAllCookies();
        else
          self->core_manager->DeleteCookies(Utf8Conv::Utf8ToUtf16(name).c_str(),
                                            Utf8Conv::Utf8ToUtf16(url).c_str());

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), std::string(name),
      std::string(url)));
  obj->browser->parent->SyncWaitIfNeed();
}

using GetCookieAsyncCallback = void(CALLBACK*)(LPCSTR cookie_json,
                                               LPVOID param);
void WINAPI GetCookiesAsync(CookieManagerData* obj,
                            LPCSTR url,
                            GetCookieAsyncCallback callback,
                            LPVOID param) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<CookieManagerData> obj, LPCSTR url,
         GetCookieAsyncCallback callback, LPVOID param) {
        obj->core_manager->GetCookies(
            Utf8Conv::Utf8ToUtf16(url).c_str(),
            WRL::Callback<ICoreWebView2GetCookiesCompletedHandler>(
                [callback, param](HRESULT result,
                                  ICoreWebView2CookieList* cookieList) {
                  json cookie_json;

                  uint32_t cookie_size = 0;
                  cookieList->get_Count(&cookie_size);
                  for (uint32_t i = 0; i < cookie_size; ++i) {
                    WRL::ComPtr<ICoreWebView2Cookie> cookie;
                    cookieList->GetValueAtIndex(i, &cookie);

                    wil::unique_cotaskmem_string value = nullptr;
                    json item = json::array();

                    cookie->get_Name(&value);
                    item["name"] = Utf8Conv::Utf16ToUtf8(value.get());

                    cookie->get_Value(&value);
                    item["value"] = Utf8Conv::Utf16ToUtf8(value.get());

                    cookie->get_Domain(&value);
                    item["domain"] = Utf8Conv::Utf16ToUtf8(value.get());

                    cookie->get_Path(&value);
                    item["path"] = Utf8Conv::Utf16ToUtf8(value.get());

                    cookie_json.push_back(std::move(item));
                  }

                  callback(cookie_json.dump().c_str(), param);

                  return S_OK;
                })
                .Get());
      },
      scoped_refptr(obj), url, callback, param));
}

DWORD fnCookieManagerTable[] = {
    (DWORD)GetCookies,
    (DWORD)AddOrUpdateCookie,
    (DWORD)DeleteCookie,
    (DWORD)GetCookiesAsync,
};

void TransferRequestJSON(const json& from, RequestData* to) {
  to->url = WrapComString(from["url"].template get<std::string>().c_str());
  to->method =
      WrapComString(from["method"].template get<std::string>().c_str());

  std::string str_headers;
  for (auto& it : from["headers"].items())
    str_headers +=
        it.key() + ": " + it.value().template get<std::string>() + "\r\n";
  to->headers = WrapComString(str_headers.c_str());

  if (from.find("postData") != from.end()) {
    auto post_data = from["postData"].template get<std::string>();
    to->post_data = WrapComString(post_data.c_str());
  }

  to->has_post_data = false;
  if (from.find("hasPostData") != from.end())
    to->has_post_data = from["hasPostData"].template get<bool>();

  to->initial_priority = WrapComString(
      from["initialPriority"].template get<std::string>().c_str());
  to->referrer_policy =
      WrapComString(from["referrerPolicy"].template get<std::string>().c_str());
}

void FreeJSONRequest(RequestData* obj) {
  FreeComString(obj->url);
  FreeComString(obj->method);
  FreeComString(obj->headers);
  FreeComString(obj->post_data);
  FreeComString(obj->initial_priority);
  FreeComString(obj->referrer_policy);
}

void WINAPI ContinueRequest(ResourceRequestCallback* obj,
                            RequestData* request) {
  json continue_args;
  continue_args["requestId"] = obj->event_parameter["requestId"];
  continue_args["interceptResponse"] = true;

  if (request) {
    continue_args["url"] = request->url;
    continue_args["method"] = request->method;
    continue_args["postData"] = request->post_data;

    std::string headers_raw = request->headers;
    json header_arr = json::array();
    std::vector<std::string> headers = SplitString(headers_raw, "\n");
    for (auto& it : headers) {
      std::string key, value;
      ExtractKeyValue(it, key, value);
      key = TrimString(key);
      value = TrimString(value);

      json item = json::object();
      item["key"] = key;
      item["value"] = value;
      header_arr.push_back(std::move(item));
    }
    continue_args["headers"] = header_arr;
  }

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ResourceRequestCallback> obj, json continue_args) {
        obj->browser->core_webview->CallDevToolsProtocolMethod(
            L"Fetch.continueRequest",
            Utf8Conv::Utf8ToUtf16(continue_args.dump()).c_str(), nullptr);
      },
      scoped_refptr(obj), std::move(continue_args)));
}

void WINAPI FailedRequest(ResourceRequestCallback* obj, LPCSTR failed_reason) {
  json continue_args;
  continue_args["requestId"] = obj->event_parameter["requestId"];

  std::string reason(failed_reason);
  if (reason.empty())
    reason = "Failed";
  continue_args["errorReason"] = reason;

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ResourceRequestCallback> obj, json continue_args) {
        obj->browser->core_webview->CallDevToolsProtocolMethod(
            L"Fetch.failRequest",
            Utf8Conv::Utf8ToUtf16(continue_args.dump()).c_str(), nullptr);
      },
      scoped_refptr(obj), std::move(continue_args)));
}

void WINAPI FulfillRequest(ResourceRequestCallback* obj,
                           ResponseData* response,
                           LPBYTE data,
                           uint32_t size) {
  json continue_args;
  continue_args["requestId"] = obj->event_parameter["requestId"];
  if (response) {
    continue_args["responseCode"] = response->response_code;
    continue_args["responsePhrase"] = response->response_phrase;

    std::string headers_raw = response->response_headers;
    json header_arr = json::array();
    std::vector<std::string> headers = SplitString(headers_raw, "\n");
    for (auto& it : headers) {
      std::string key, value;
      ExtractKeyValue(it, key, value);
      key = TrimString(key);
      value = TrimString(value);

      json item = json::object();
      item["name"] = key;
      item["value"] = value;
      header_arr.push_back(std::move(item));
    }

    continue_args["responseHeaders"] = header_arr;
  } else {
    continue_args["responseCode"] = obj->event_parameter["responseStatusCode"];
    continue_args["responseHeaders"] = obj->event_parameter["responseHeaders"];
  }

  std::string mem;
  mem.assign(size, 0);
  memcpy(&mem.front(), mem.data(), size);
  continue_args["body"] = modp_b64_encode(mem);

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ResourceRequestCallback> obj, json continue_args) {
        obj->browser->core_webview->CallDevToolsProtocolMethod(
            L"Fetch.fulfillRequest",
            Utf8Conv::Utf8ToUtf16(continue_args.dump()).c_str(), nullptr);
      },
      scoped_refptr(obj), std::move(continue_args)));
}

DWORD fnResourceRequestCallbackTable[] = {
    (DWORD)ContinueRequest,
    (DWORD)FailedRequest,
    (DWORD)FulfillRequest,
};

void WINAPI ContinueResponse(ResourceResponseCallback* obj,
                             ResponseData* response) {
  json continue_args;
  continue_args["requestId"] = obj->event_parameter["requestId"];
  if (response) {
    continue_args["responseCode"] = response->response_code;
    continue_args["responsePhrase"] = response->response_phrase;

    std::string headers_raw = response->response_headers;
    json header_arr = json::array();
    std::vector<std::string> headers = SplitString(headers_raw, "\n");
    for (auto& it : headers) {
      std::string key, value;
      ExtractKeyValue(it, key, value);
      key = TrimString(key);
      value = TrimString(value);

      json item = json::object();
      item["name"] = key;
      item["value"] = value;
      header_arr.push_back(std::move(item));
    }

    continue_args["responseHeaders"] = header_arr;
  } else {
    continue_args["responseCode"] = obj->event_parameter["responseStatusCode"];
    continue_args["responseHeaders"] = obj->event_parameter["responseHeaders"];
  }

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ResourceResponseCallback> obj, json continue_args) {
        obj->browser->core_webview->CallDevToolsProtocolMethod(
            L"Fetch.continueResponse",
            Utf8Conv::Utf8ToUtf16(continue_args.dump()).c_str(), nullptr);
      },
      scoped_refptr(obj), std::move(continue_args)));
}

using ReceivedResponseCallback = void(CALLBACK*)(LPVOID ptr,
                                                 uint32_t size,
                                                 LPVOID param);
void WINAPI GetResponseBodyData(ResourceResponseCallback* obj,
                                ReceivedResponseCallback callback,
                                LPVOID param) {
  json continue_args;
  continue_args["requestId"] = obj->event_parameter["requestId"];

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ResourceResponseCallback> obj, json continue_args,
         ReceivedResponseCallback callback, LPVOID param) {
        obj->browser->core_webview->CallDevToolsProtocolMethod(
            L"Fetch.getResponseBody",
            Utf8Conv::Utf8ToUtf16(continue_args.dump()).c_str(),
            WRL::Callback<
                ICoreWebView2CallDevToolsProtocolMethodCompletedHandler>(
                [weak_ptr = obj->browser, callback, param](
                    HRESULT errorCode, LPCWSTR returnObjectAsJson) {
                  json ret_args =
                      json::parse(Utf8Conv::Utf16ToUtf8(returnObjectAsJson));

                  weak_ptr->parent->PostEvent(base::BindOnce(
                      [](const json& ret_args,
                         ReceivedResponseCallback callback, LPVOID param) {
                        std::string body;
                        if (ret_args.find("body") != ret_args.end()) {
                          body = ret_args["body"].template get<std::string>();
                          if (ret_args.find("base64Encoded") !=
                                  ret_args.end() &&
                              ret_args["base64Encoded"].template get<bool>()) {
                            body = modp_b64_decode(body);
                          }
                        }

                        LPVOID lpMem = nullptr;
                        if (!body.empty()) {
                          lpMem = edgeview_MemAlloc(body.size());
                          RtlCopyMemory(lpMem, body.data(), body.size());
                        }

                        callback(lpMem, body.size(), param);

                        if (lpMem)
                          edgeview_MemFree(lpMem);
                      },
                      std::move(ret_args), callback, param));

                  return S_OK;
                })
                .Get());
      },
      scoped_refptr(obj), std::move(continue_args), callback, param));
}

void WINAPI GetResponseBodyDataSync(ResourceResponseCallback* obj,
                                    LPVOID* data_ptr,
                                    int32_t* data_size) {
  json continue_args;
  continue_args["requestId"] = obj->event_parameter["requestId"];

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ResourceResponseCallback> obj,
         scoped_refptr<Semaphore> sync, json continue_args, LPVOID* data_ptr,
         int32_t* data_size) {
        obj->browser->core_webview->CallDevToolsProtocolMethod(
            L"Fetch.getResponseBody",
            Utf8Conv::Utf8ToUtf16(continue_args.dump()).c_str(),
            WRL::Callback<
                ICoreWebView2CallDevToolsProtocolMethodCompletedHandler>(
                [sync, data_ptr, data_size](HRESULT errorCode,
                                            LPCWSTR returnObjectAsJson) {
                  json ret_args =
                      json::parse(Utf8Conv::Utf16ToUtf8(returnObjectAsJson));

                  std::string body;
                  if (ret_args.find("body") != ret_args.end()) {
                    body = ret_args["body"].template get<std::string>();
                    if (ret_args.find("base64Encoded") != ret_args.end() &&
                        ret_args["base64Encoded"].template get<bool>()) {
                      body = modp_b64_decode(body);
                    }
                  }

                  LPVOID lpMem = nullptr;
                  if (!body.empty()) {
                    lpMem = edgeview_MemAlloc(body.size());
                    RtlCopyMemory(lpMem, body.data(), body.size());
                  }

                  *data_ptr = lpMem;
                  *data_size = body.size();

                  sync->Notify();

                  return S_OK;
                })
                .Get());
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(),
      std::move(continue_args), data_ptr, data_size));
  obj->browser->parent->SyncWaitIfNeed();
}

void WINAPI FulfillResponse(ResourceResponseCallback* obj,
                            ResponseData* response,
                            LPBYTE data,
                            uint32_t size) {
  json continue_args;
  continue_args["requestId"] = obj->event_parameter["requestId"];
  if (response) {
    continue_args["responseCode"] = response->response_code;
    continue_args["responsePhrase"] = response->response_phrase;

    std::string headers_raw = response->response_headers;
    json header_arr = json::array();
    std::vector<std::string> headers = SplitString(headers_raw, "\n");
    for (auto& it : headers) {
      std::string key, value;
      ExtractKeyValue(it, key, value);
      key = TrimString(key);
      value = TrimString(value);

      json item = json::object();
      item["name"] = key;
      item["value"] = value;
      header_arr.push_back(std::move(item));
    }

    continue_args["responseHeaders"] = header_arr;
  } else {
    continue_args["responseCode"] = obj->event_parameter["responseStatusCode"];
    continue_args["responseHeaders"] = obj->event_parameter["responseHeaders"];
  }

  std::string mem;
  mem.assign(size, 0);
  memcpy(&mem.front(), data, size);
  continue_args["body"] = modp_b64_encode(mem);

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ResourceResponseCallback> obj, json continue_args) {
        obj->browser->core_webview->CallDevToolsProtocolMethod(
            L"Fetch.fulfillRequest",
            Utf8Conv::Utf8ToUtf16(continue_args.dump()).c_str(), nullptr);
      },
      scoped_refptr(obj), std::move(continue_args)));
}

DWORD fnResourceResponseCallbackTable[] = {
    (DWORD)ContinueResponse,
    (DWORD)GetResponseBodyData,
    (DWORD)FulfillResponse,
    (DWORD)GetResponseBodyDataSync,
};

void WINAPI SetAuthInfo(BasicAuthenticationCallback* obj,
                        LPCSTR username,
                        LPCSTR password) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BasicAuthenticationCallback> obj, std::string uname,
         std::string passwd) {
        WRL::ComPtr<ICoreWebView2BasicAuthenticationResponse> response;
        obj->core_callback->get_Response(&response);

        response->put_UserName(Utf8Conv::Utf8ToUtf16(uname).c_str());
        response->put_Password(Utf8Conv::Utf8ToUtf16(passwd).c_str());

        obj->core_callback->put_Cancel(FALSE);
        obj->internal_deferral->Complete();
      },
      scoped_refptr(obj), std::string(username), std::string(password)));
}

void WINAPI CancelAuth(BasicAuthenticationCallback* obj) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<BasicAuthenticationCallback> obj) {
        WRL::ComPtr<ICoreWebView2BasicAuthenticationResponse> response;
        obj->core_callback->put_Cancel(TRUE);
        obj->internal_deferral->Complete();
      },
      scoped_refptr(obj)));
}

DWORD fnBasicAuthenticationCallbackTable[] = {
    (DWORD)SetAuthInfo,
    (DWORD)CancelAuth,
};

}  // namespace edgeview
