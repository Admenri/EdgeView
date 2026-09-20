#pragma once

#pragma comment(lib, "Shlwapi.lib")

#include <Shlwapi.h>
#include <stdint.h>
#include <wil/com.h>
#include <wil/resource.h>
#include <wil/win32_helpers.h>
#include <windows.h>
#include <wrl/event.h>

#undef __has_attribute
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

// RAII holder for strings returned by WrapComString. The caller owns the
// memory and must release it with FreeComString / edgeview_MemFree.
// Use it for temporary strings (e.g. event dispatch arguments) so the
// matching FreeComString can never be forgotten.
// Do NOT use it to hold pointers that EPL owns (e.g. values returned to
// EPL), and never wrap a pointer that was not allocated by WrapComString.
class ScopedComString {
 public:
  ScopedComString() = default;

  // Takes over the pointer returned by WrapComString (nullptr is allowed).
  ScopedComString(LPCSTR str) : str_(str) {}  // NOLINT: implicit on purpose

  ScopedComString(const ScopedComString&) = delete;
  ScopedComString& operator=(const ScopedComString&) = delete;

  ScopedComString(ScopedComString&& other) noexcept : str_(other.str_) {
    other.str_ = nullptr;
  }

  ScopedComString& operator=(ScopedComString&& other) noexcept {
    if (this != &other) {
      Reset();
      str_ = other.str_;
      other.str_ = nullptr;
    }
    return *this;
  }

  ~ScopedComString() { Reset(); }

  // Replaces the held string, releasing the previous one immediately.
  ScopedComString& operator=(LPCSTR str) {
    if (str_ != str) {
      Reset();
      str_ = str;
    }
    return *this;
  }

  void Reset() {
    if (str_) {
      FreeComString(str_);
      str_ = nullptr;
    }
  }

  // Gives up the ownership; the caller becomes responsible for releasing it.
  LPCSTR Release() {
    LPCSTR str = str_;
    str_ = nullptr;
    return str;
  }

  LPCSTR get() const { return str_; }
  operator LPCSTR() const { return str_; }

 private:
  LPCSTR str_ = nullptr;
};

LPBYTE GetAryElementInf(void* pAryData, LPINT pnElementCount);
void FreeAryElement(void* pAryData);

std::vector<std::string> SplitString(const std::string& str,
                                     const std::string& delimiter);

std::string TrimString(const std::string& str);

}  // namespace edgeview
