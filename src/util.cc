#include "util.h"

#include <atlconv.h>

namespace edgeview {

LPSTR WrapComString(LPCWSTR oriStr) {
  if (!oriStr) return nullptr;

  INT wideStrLen = static_cast<INT>(wcslen(oriStr));
  INT utf8Len =
      WideCharToMultiByte(CP_UTF8, 0, oriStr, wideStrLen, NULL, 0, NULL, NULL);

  // Empty string or failed conversion: still return a safe (allocated)
  // empty string so callers can always dereference the result.
  if (utf8Len <= 0) {
    LPSTR emptyStr = static_cast<LPSTR>(edgeview_MemAlloc(1));
    if (emptyStr) emptyStr[0] = '\0';
    return emptyStr;
  }

  LPSTR utf8Str = static_cast<LPSTR>(edgeview_MemAlloc(utf8Len + 1));
  if (!utf8Str) return nullptr;

  if (!WideCharToMultiByte(CP_UTF8, 0, oriStr, wideStrLen, utf8Str, utf8Len,
                           NULL, NULL)) {
    edgeview_MemFree(utf8Str);
    return nullptr;
  }

  utf8Str[utf8Len] = '\0';
  return utf8Str;
}

LPSTR WrapComString(LPCSTR oriStr) {
  if (!oriStr) return nullptr;

  size_t s = strlen(oriStr);
  LPSTR pstr = (LPSTR)edgeview_MemAlloc(s + 1);
  if (!pstr) return nullptr;

  // Copy s + 1 bytes so the terminating '\0' is written as well.
  // Do not rely on the HEAP_ZERO_MEMORY behaviour of edgeview_MemAlloc here.
  RtlCopyMemory(pstr, oriStr, s + 1);

  return pstr;
}

LPSTR WrapComString(const wil::unique_cotaskmem_string& str) {
  return WrapComString(str.get());
}

LPBYTE WrapEStream(const std::string& mem) {
  LPBYTE ptr = (LPBYTE)edgeview_MemAlloc(mem.size() + 4);

  *(int*)ptr = mem.size();
  memcpy(ptr + 4, mem.data(), mem.size());

  return ptr;
}

LPBYTE GetAryElementInf(void* pAryData, LPINT pnElementCount) {
  LPINT pnData = (LPINT)pAryData;
  INT nArys = *pnData++;
  INT nElementCount = 1;
  while (nArys > 0) {
    nElementCount *= *pnData++;
    nArys--;
  }

  if (pnElementCount != NULL) *pnElementCount = nElementCount;
  return (LPBYTE)pnData;
}

void FreeAryElement(void* pAryData) {
  if (!pAryData) return;

  INT nElementCount = 0;
  LPINT* pArryPtr =
      (LPINT*)GetAryElementInf(pAryData, (LPINT)&nElementCount);

  for (INT i = 0; i < nElementCount; i++) {
    void* pElementData = (void*)(*pArryPtr);
    if (pElementData) {
      // Elements are allocated by edgeview_MemAlloc, so they must be
      // released with HeapFree. The old code mixed in free(), which fails
      // silently for process heap blocks and leaked every element.
      edgeview_MemFree(pElementData);
      *pArryPtr = 0;
    }
    pArryPtr++;
  }

  edgeview_MemFree(pAryData);
}

EV_EXPORTS(MemAlloc, LPVOID)(size_t size) {
  return ::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}

EV_EXPORTS(MemFree, BOOL)(LPVOID mem) {
  return ::HeapFree(::GetProcessHeap(), 0, mem);
}

std::vector<std::string> SplitString(const std::string& str,
                                     const std::string& delimiter) {
  std::vector<std::string> result;
  std::string::size_type start = 0;
  std::string::size_type end = str.find(delimiter, start);

  while (end != std::string::npos) {
    result.push_back(str.substr(start, end - start));
    start = end + delimiter.length();
    end = str.find(delimiter, start);
  }

  result.push_back(str.substr(start));

  return result;
}

std::string TrimString(const std::string& str) {
  std::string result = str;

  size_t start = result.find_first_not_of(" \t\n\r\f\v");
  if (start != std::string::npos) {
    result = result.substr(start);
  }

  size_t end = result.find_last_not_of(" \t\n\r\f\v");
  if (end != std::string::npos) {
    result = result.substr(0, end + 1);
  }

  return result;
}

}  // namespace edgeview
