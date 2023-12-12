#pragma once

#include "util.h"

namespace edgeview {

using RequestData = struct {
  LPCSTR url;
  LPCSTR method;
  LPCSTR headers;
  LPCSTR post_data;
  BOOL has_post_data;
  LPCSTR initial_priority;
  LPCSTR referrer_policy;
};

using ResponseData = struct {
  int response_code;
  LPCSTR response_phrase;
  LPCSTR response_headers;
};

void TransferRequestJSON(const json& from, RequestData* to);
void TransferRequestJSON(RequestData* from, json& ori);

extern DWORD fnCookieManagerTable[];
extern DWORD fnResourceRequestCallbackTable[];
extern DWORD fnResourceResponseCallbackTable[];
extern DWORD fnBasicAuthenticationCallbackTable[];

}  // namespace edgeview
