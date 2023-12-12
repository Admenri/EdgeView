#pragma once

#include "edgeview_data.h"
#include "util.h"

namespace edgeview {

using RemoteObject = struct {
  LPCSTR type;
  LPCSTR subtype;
  LPCSTR class_name;
  LPCSTR raw_value;
  LPCSTR unserializable_value;
  LPCSTR description;
  LPCSTR raw_deepSerializedValue;
  LPCSTR objectID;
};

json RemoteObjectToJSON(RemoteObject* ro);
void JSONToRemoteObject(RemoteObject* ro, const json& raw);

void BindEventForWebView(scoped_refptr<BrowserData> browser_wrapper);
void BindEventForUpdate(scoped_refptr<BrowserData> browser_wrapper);

extern DWORD fnBrowserTable[];
extern DWORD fnNewWindowDelegateTable[];
extern DWORD fnScriptDialogDelegateTable[];
extern DWORD fnPermissionDelegateTable[];

}  // namespace edgeview
