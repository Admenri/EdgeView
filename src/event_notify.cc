#include "event_notify.h"

#include "ev_browser.h"
#include "ev_contextmenu.h"
#include "ev_download.h"
#include "ev_frame.h"
#include "struct_class.h"

namespace edgeview {

BrowserEventDispatcher::BrowserEventDispatcher(
    base::WeakPtr<BrowserData> browser, LPVOID callback)
    : self(browser), ecallback(callback) {}

BrowserEventDispatcher::~BrowserEventDispatcher() {}

void BrowserEventDispatcher::OnCreated() {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			push TempBrowser;
			push ecx;
			call[edx + 0x08];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
  }
}

void BrowserEventDispatcher::OnCloseRequested() {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			push TempBrowser;
			push ecx;
			call[edx + 0x0C];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
  }
}

void BrowserEventDispatcher::OnNewWindowRequested(
    scoped_refptr<NewWindowDelegate> delegate) {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    delegate->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    IMP_NEWECLASS(TempDelegate, delegate.get(),
                  eClass::m_pVfTable_NewWindowDelegate,
                  fnNewWindowDelegateTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			push TempDelegate;
			push TempBrowser;
			push ecx;
			call[edx + 0x10];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
    delegate->Release();
  }
}

void BrowserEventDispatcher::OnDocumentTitleChanged(LPCSTR title) {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			lea eax, title;
			push eax;
			push TempBrowser;
			push ecx;
			call[edx + 0x14];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
  }
}

void BrowserEventDispatcher::OnFullscreenModeChanged(BOOL fullscreen) {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			push fullscreen;
			push TempBrowser;
			push ecx;
			call[edx + 0x18];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
  }
}

BOOL BrowserEventDispatcher::OnBeforeNavigation(LPCSTR url, BOOL user_gesture,
                                                BOOL is_redirect,
                                                LPCSTR headers,
                                                uint64_t nav_id) {
  scoped_refptr<BrowserData> browser(self.get());
  BOOL bRetVal = FALSE;
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    uint32_t nav_id_a = HIGH_32BIT(nav_id);
    uint32_t nav_id_b = LOW_32BIT(nav_id);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			push nav_id_b;
			push nav_id_a;
			lea eax, headers;
			push eax;
			push is_redirect;
			push user_gesture;
			lea eax, url;
			push eax;
			push TempBrowser;
			push ecx;
			call[edx + 0x1C];
			mov bRetVal, eax;
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
  }
  return bRetVal;
}

void BrowserEventDispatcher::OnContentLoading(BOOL error_page,
                                              uint64_t nav_id) {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    uint32_t nav_id_a = HIGH_32BIT(nav_id);
    uint32_t nav_id_b = LOW_32BIT(nav_id);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			push nav_id_b;
			push nav_id_a;
			push error_page;
			push TempBrowser;
			push ecx;
			call[edx + 0x20];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
  }
}

void BrowserEventDispatcher::OnSourceChanged(BOOL new_document) {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			push new_document;
			push TempBrowser;
			push ecx;
			call[edx + 0x24];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
  }
}

void BrowserEventDispatcher::OnHistoryChanged() {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			push TempBrowser;
			push ecx;
			call[edx + 0x28];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
  }
}

void BrowserEventDispatcher::OnNavigationComplete(BOOL success,
                                                  int error_status,
                                                  uint64_t nav_id) {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    uint32_t nav_id_a = HIGH_32BIT(nav_id);
    uint32_t nav_id_b = LOW_32BIT(nav_id);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			push nav_id_b;
			push nav_id_a;
			push error_status;
			push success;
			push TempBrowser;
			push ecx;
			call[edx + 0x2C];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
  }
}

void BrowserEventDispatcher::OnScriptDialogRequested(
    LPCSTR url, int kind, LPCSTR message, LPCSTR deftext,
    scoped_refptr<ScriptDialogDelegate> delegate) {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    delegate->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    IMP_NEWECLASS(TempDelegate, delegate.get(),
                  eClass::m_pVfTable_ScriptDialogDelegate,
                  fnScriptDialogDelegateTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			push TempDelegate;
			lea eax, deftext;
			push eax;
			lea eax, message;
			push eax;
			push kind;
			lea eax, url;
			push eax;
			push TempBrowser;
			push ecx;
			call[edx + 0x30];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
    delegate->Release();
  }
}

void BrowserEventDispatcher::OnContextMenuRequested(
    scoped_refptr<ContextMenuParams> params) {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    params->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    IMP_NEWECLASS(TempParams, params.get(),
                  eClass::m_pVfTable_ContextMenuParams,
                  fnContextMenuParamsTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			push TempParams;
			push TempBrowser;
			push ecx;
			call[edx + 0x34];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
    params->Release();
  }
}

void BrowserEventDispatcher::OnContextMenuExecute(
    scoped_refptr<ContextMenuItem> item) {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    item->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    IMP_NEWECLASS(TempItem, item.get(), eClass::m_pVfTable_ContextMenuItem,
                  fnContextMenuItemTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			push TempItem;
			push TempBrowser;
			push ecx;
			call[edx + 0x38];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
    item->Release();
  }
}

void BrowserEventDispatcher::OnPermissionRequested(
    LPCSTR url, int kind, BOOL user_gesture,
    scoped_refptr<PermissionDelegate> delegate) {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    delegate->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    IMP_NEWECLASS(TempDelegate, delegate.get(),
                  eClass::m_pVfTable_PermissionDelegate,
                  fnPermissionDelegateTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			push TempDelegate;
			push user_gesture;
			push kind;
			lea eax, url;
			push eax;
			push TempBrowser;
			push ecx;
			call[edx + 0x3C];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
    delegate->Release();
  }
}

void BrowserEventDispatcher::OnResourceRequested(json request_parameter) {
  scoped_refptr<BrowserData> browser(self.get());
  scoped_refptr<ResourceRequestCallback> callback =
      new ResourceRequestCallback();
  callback->browser = browser->weak_ptr_.GetWeakPtr();
  callback->event_parameter = request_parameter;

  LPCSTR network_id = WrapComString(
      request_parameter["requestId"].template get<std::string>().c_str());
  LPCSTR frame_id = WrapComString(
      request_parameter["frameId"].template get<std::string>().c_str());

  std::unique_ptr<RequestData> request(new RequestData());
  RequestData* pReq = request.get();
  TransferRequestJSON(request_parameter["request"], request.get());

  LPCSTR resource_type = WrapComString(
      request_parameter["resourceType"].template get<std::string>().c_str());

  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    callback->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    IMP_NEWECLASS(TempCallback, callback.get(),
                  eClass::m_pVfTable_ResourceRequestCallback,
                  fnResourceRequestCallbackTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			push TempCallback;
			lea eax, resource_type;
			push eax;
			lea eax, frame_id;
			push eax;
			lea eax, pReq;
			push eax;
			lea eax, network_id;
			push eax;
			push TempBrowser;
			push ecx;
			call[edx + 0x40];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
    callback->Release();
  }

  FreeComString(network_id);
  FreeComString(frame_id);
  FreeComString(resource_type);

  FreeJSONRequest(request.get());
}

void BrowserEventDispatcher::OnResourceReceiveResponse(json request_parameter) {
  scoped_refptr<BrowserData> browser(self.get());
  scoped_refptr<ResourceResponseCallback> callback =
      new ResourceResponseCallback();
  callback->browser = browser->weak_ptr_.GetWeakPtr();
  callback->event_parameter = request_parameter;

  LPCSTR network_id = WrapComString(
      request_parameter["requestId"].template get<std::string>().c_str());
  LPCSTR frame_id = WrapComString(
      request_parameter["frameId"].template get<std::string>().c_str());

  std::unique_ptr<RequestData> request(new RequestData());
  RequestData* pReq = request.get();
  TransferRequestJSON(request_parameter["request"], request.get());

  LPCSTR resource_type = WrapComString(
      request_parameter["resourceType"].template get<std::string>().c_str());

  std::unique_ptr<ResponseData> response(new ResponseData());
  ResponseData* pResponse = response.get();

  {
    response->response_code =
        request_parameter["responseStatusCode"].template get<int>();
    response->response_phrase =
        WrapComString(request_parameter["responseStatusText"]
                          .template get<std::string>()
                          .c_str());

    std::string header_raw;
    auto headers = request_parameter["responseHeaders"];
    for (auto& it : headers) {
      header_raw += it["name"].template get<std::string>() + ": " +
                    it["value"].template get<std::string>() + "\r\n";
    }

    response->response_headers = WrapComString(header_raw.c_str());
  }

  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    callback->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    IMP_NEWECLASS(TempCallback, callback.get(),
                  eClass::m_pVfTable_ResourceResponseCallback,
                  fnResourceResponseCallbackTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			push TempCallback;
			lea eax, pResponse;
			push eax;
			lea eax, resource_type;
			push eax;
			lea eax, frame_id;
			push eax;
			lea eax, pReq;
			push eax;
			lea eax, network_id;
			push eax;
			push TempBrowser;
			push ecx;
			call[edx + 0x44];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
    callback->Release();
  }

  FreeComString(network_id);
  FreeComString(frame_id);
  FreeComString(resource_type);

  FreeComString(response->response_phrase);
  FreeComString(response->response_headers);

  FreeJSONRequest(request.get());
}

BOOL BrowserEventDispatcher::OnKeyEvent(
    COREWEBVIEW2_KEY_EVENT_KIND kind, uint32_t virtual_key, int lparam,
    COREWEBVIEW2_PHYSICAL_KEY_STATUS* status) {
  scoped_refptr<BrowserData> browser(self.get());
  BOOL bRetVal = FALSE;

  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			lea eax, status;
			push eax;
			push lparam;
			push virtual_key;
			push kind;
			push TempBrowser;
			push ecx;
			call[edx + 0x48];
			mov bRetVal, eax;
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
  }

  return bRetVal;
}

void BrowserEventDispatcher::BasicAuthRequested(
    LPCSTR url, LPCSTR challenge,
    scoped_refptr<BasicAuthenticationCallback> callback) {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    callback->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    IMP_NEWECLASS(TempCallback, callback.get(),
                  eClass::m_pVfTable_BasicAuthenticationCallback,
                  fnBasicAuthenticationCallbackTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			push TempCallback;
			lea eax, challenge;
			push eax;
			lea eax, url;
			push eax;
			push TempBrowser;
			push ecx;
			call[edx + 0x4C];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
    callback->Release();
  }
}

void BrowserEventDispatcher::OnReceivedWebMessage(
    scoped_refptr<FrameData> frame, LPCSTR source_url, LPCSTR json_args) {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    IMP_NEWECLASS(TempFrame, frame.get(), eClass::m_pVfTable_Frame,
                  fnFrameTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			lea eax, json_args;
			push eax;
			lea eax, source_url;
			push eax;
			push TempFrame;
			push TempBrowser;
			push ecx;
			call[edx + 0x50];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
  }
}

void BrowserEventDispatcher::OnFileChooserRequested(LPCSTR frame_id,
                                                    BOOL multiselect,
                                                    int node_id) {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			push node_id;
			push multiselect;
			lea eax, frame_id;
			push eax;
			push TempBrowser;
			push ecx;
			call[edx + 0x54];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
  }
}

void BrowserEventDispatcher::OnConsoleMessage(json args) {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);

    json console_event = args["message"];

    LPCSTR pSrc = WrapComString(
        console_event["source"].template get<std::string>().c_str());
    LPCSTR pLvl = WrapComString(
        console_event["level"].template get<std::string>().c_str());
    LPCSTR pTxt = WrapComString(
        console_event["text"].template get<std::string>().c_str());

    LPCSTR pURL = nullptr;
    int line = 0;
    int column = 0;
    if (console_event.find("url") != console_event.end()) {
      pURL = WrapComString(
          console_event["url"].template get<std::string>().c_str());
      line = console_event["line"].template get<int>();
      column = console_event["column"].template get<int>();
    }

    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			push column;
			push line;
			lea eax, pURL;
			push eax;
			lea eax, pTxt;
			push eax;
			lea eax, pLvl;
			push eax;
			lea eax, pSrc;
			push eax;
			push TempBrowser;
			push ecx;
			call[edx + 0x58];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();

    FreeComString(pSrc);
    FreeComString(pLvl);
    FreeComString(pTxt);
    FreeComString(pURL);
  }
}

void BrowserEventDispatcher::OnBeforeDownload(
    scoped_refptr<DownloadConfirm> confirm) {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    IMP_NEWECLASS(TempConfirm, confirm.get(),
                  eClass::m_pVfTable_DownloadConfirm, fnDownloadConfirmTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			push TempConfirm;
			push TempBrowser;
			push ecx;
			call[edx + 0x5C];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
  }
}

void BrowserEventDispatcher::OnFaviconChanged(LPCSTR favicon) {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			lea eax, favicon;
			push eax;
			push TempBrowser;
			push ecx;
			call[edx + 0x60];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
  }
}

void BrowserEventDispatcher::OnAudioStateChanged(BOOL audible) {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			push audible;
			push TempBrowser;
			push ecx;
			call[edx + 0x64];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
  }
}

void BrowserEventDispatcher::OnStatusTextChanged(LPCSTR status) {
  scoped_refptr<BrowserData> browser(self.get());
  if (ecallback) {
    LPVOID pClass = ecallback;
    browser->AddRef();
    IMP_NEWECLASS(TempBrowser, browser.get(), eClass::m_pVfTable_Browser,
                  fnBrowserTable);
    __asm {
			push ecx;
			push ebx;
			push edi;
			push esi;
			mov ebx, pClass;
			mov edx, [ebx];
			lea ecx, pClass;
			lea eax, status;
			push eax;
			push TempBrowser;
			push ecx;
			call[edx + 0x68];
			pop esi;
			pop edi;
			pop ebx;
			pop ecx;
    }
    browser->Release();
  }
}

}  // namespace edgeview
