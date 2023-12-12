#include "util.h"

#include <atlconv.h>

namespace edgeview {

LPSTR WrapComString(LPCWSTR oriStr) {
  if (!oriStr) return nullptr;
  INT wideStrLen = wcslen(oriStr);
  INT utf8Len =
      WideCharToMultiByte(CP_UTF8, 0, oriStr, wideStrLen, NULL, 0, NULL, NULL);
  LPSTR utf8Str = static_cast<LPSTR>(edgeview_MemAlloc(utf8Len + 1));
  if (!utf8Str) return nullptr;
  WideCharToMultiByte(CP_UTF8, 0, oriStr, wideStrLen, utf8Str, utf8Len, NULL,
                      NULL);
  utf8Str[utf8Len] = '\0';
  return utf8Str;
}

LPSTR WrapComString(LPCSTR oriStr) {
  size_t s = strlen(oriStr);
  LPSTR pstr = (LPSTR)edgeview_MemAlloc(s + 1);
  RtlCopyMemory(pstr, oriStr, s);

  return pstr;
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
  DWORD AryElementCount = 0;
  LPINT* pArryPtr = (LPINT*)GetAryElementInf(pAryData, (LPINT)AryElementCount);

  for (INT i = 0; i < (INT)AryElementCount; i++) {
    void* pElementData = *pArryPtr;
    if (pElementData) {
      free(pElementData);
      *pArryPtr = NULL;
    }
    pArryPtr++;
  }

  free(pAryData);
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
