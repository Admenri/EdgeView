#pragma once

#pragma comment(lib, "Shlwapi.lib")

#include <Shlwapi.h>
#include <stdint.h>
#include <wil/com.h>
#include <wil/resource.h>
#include <wil/win32_helpers.h>
#include <windows.h>
#include <wrl/event.h>

#include "Utf8Conv.hpp"
#include "WebView2.h"
#include "WebView2EnvironmentOptions.h"
#include "nlohmann/json.hpp"

using namespace Microsoft;
using json = nlohmann::json;

#define EV_EXPORTS(name, type) \
  extern "C" __declspec(dllexport) type __cdecl edgeview_##name

namespace edgeview {

LPSTR WrapComString(LPCWSTR oriStr);
LPSTR WrapComString(LPCSTR oriStr);
LPSTR WrapComString(const wil::unique_cotaskmem_string& str);
LPBYTE WrapEStream(const std::string& mem);

#define HIGH_32BIT(v) v >> 32
#define LOW_32BIT(v) v & 0xFFFFFFFF

EV_EXPORTS(MemAlloc, LPVOID)(size_t size);
EV_EXPORTS(MemFree, BOOL)(LPVOID mem);

#define FreeComString(ptr) edgeview_MemFree((LPVOID)ptr)

LPBYTE GetAryElementInf(void* pAryData, LPINT pnElementCount);
void FreeAryElement(void* pAryData);

std::vector<std::string> SplitString(const std::string& str,
                                     const std::string& delimiter);

std::string TrimString(const std::string& str);

}  // namespace edgeview
