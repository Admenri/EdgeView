#ifndef GIOVANNI_DICANIO_UTF8CONV_HPP_INCLUDED
#define GIOVANNI_DICANIO_UTF8CONV_HPP_INCLUDED

////////////////////////////////////////////////////////////////////////////////
//
//            *** Unicode UTF-8 <-> UTF-16 Conversion Helpers ***
//
//                     Copyright (C) by Giovanni Dicanio
//
//
// First version: 2016, September 1st
// Last update:   2022, October 17th
//
// E-mail: <first name>.<last name> AT REMOVE_THIS gmail.com
//
// This header-only C++ library implements conversion helper functions
// to convert strings between Unicode UTF-8 and Unicode UTF-16.
//
// Unicode UTF-16 (LE) is the de facto standard Unicode encoding
// of the Windows APIs, while UTF-8 is widely used to store and exchange
// text across the Internet and in a multi-platform way.
// So, in Windows C++ applications, many times there's a need to convert
// between UTF-16 and UTF-8.
//
// Unicode UTF-8 strings are represented using the std::string class;
// Unicode UTF-16 strings are represented using the std::wstring class.
// ATL's CString is not used, to avoid dependencies from ATL or MFC.
//
// ---------------------------------------------------------------------------
//
// For more information, please read the article I wrote that was published
// on the 2016 September issue of MSDN Magazine:
//
//  C++ - Unicode Encoding Conversions with STL Strings and Win32 APIs
//  https://msdn.microsoft.com/magazine/mt763237
//
// ---------------------------------------------------------------------------
//
// Compiler: Visual Studio 2019
// C++ Language Standard: ISO C++17 Standard (/std:c++17)
// Code compiles cleanly at warning level 4 (/W4) on both 32-bit and 64-bit
// builds.
//
// Requires building in Unicode mode (which has been the default since VS2005).
//
// ===========================================================================
//
// The MIT License(MIT)
//
// Copyright(c) 2016-2022 by Giovanni Dicanio
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files(the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
//
// Safe Integer Overflow Checks
// ============================
//
// Windows APIs used to convert between UTF-8 and UTF-16
// (e.g. MultiByteToWideChar) require string lengths expressed as int,
// while STL strings store their lengths using size_t.
// In case of *VERY* long strings there can be an overflow when converting
// from size_t (STL std::[w]string) to int (for Win32 API calls).
//
// The default behavior for this library is to *always* perform those checks
// at runtime, and throw a std::overflow_error exception on overflow.
//
// If you want to change this default behavior and want to _disable_
// those runtime checks in Release Builds, you can #define the following macro:
//
// // If defined, integer overflow checks happen in debug builds only
// #define GIOVANNI_DICANIO_UTF8CONV_CHECK_INTEGER_OVERFLOWS_ONLY_IN_DEBUG
//
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//                              Includes
//------------------------------------------------------------------------------

#include <Windows.h>  // Windows Platform SDK
#include <crtdbg.h>   // _ASSERTE

#include <limits>        // For std::numeric_limits
#include <stdexcept>     // For std::overflow_error
#include <string>        // For std::string, std::wstring
#include <string_view>   // For std::string_view, std::wstring_view
#include <system_error>  // For std::system_error

namespace Utf8Conv {

//
// Forward declarations and Function Prototypes
//

// Exception class representing an error occurred during a Unicode conversion
class Utf8ConversionException;

//==============================================================================
//                      *** UTF-16 --> UTF-8 ***
//==============================================================================

//------------------------------------------------------------------------------
// Convert the input string from UTF-16 to UTF-8.
// Throws Utf8ConversionException on conversion errors
// (e.g. invalid UTF-16 sequence found in input string).
//------------------------------------------------------------------------------
[[nodiscard]] std::string Utf16ToUtf8(const std::wstring& utf16);

//------------------------------------------------------------------------------
// Convert the input string *view* from UTF-16 to UTF-8.
// Throws Utf8ConversionException on conversion errors
// (e.g. invalid UTF-16 sequence found in input string).
//------------------------------------------------------------------------------
[[nodiscard]] std::string Utf16ToUtf8(std::wstring_view utf16);

//------------------------------------------------------------------------------
// Convert the input NUL-terminated C-style string pointer from UTF-16 to UTF-8.
// Throws Utf8ConversionException on conversion errors
// (e.g. invalid UTF-16 sequence found in input string).
//------------------------------------------------------------------------------
[[nodiscard]] std::string Utf16ToUtf8(_In_opt_z_ const wchar_t* utf16);

//==============================================================================
//                      *** UTF-8 --> UTF-16 ***
//==============================================================================

//------------------------------------------------------------------------------
// Convert the input string from UTF-8 to UTF-16.
// Throws Utf8ConversionException on conversion errors
// (e.g. invalid UTF-8 sequence found in input string).
//------------------------------------------------------------------------------
[[nodiscard]] std::wstring Utf8ToUtf16(const std::string& utf8);

//------------------------------------------------------------------------------
// Convert the input string *view* from UTF-8 to UTF-16.
// Throws Utf8ConversionException on conversion errors
// (e.g. invalid UTF-8 sequence found in input string).
//------------------------------------------------------------------------------
[[nodiscard]] std::wstring Utf8ToUtf16(std::string_view utf8);

//------------------------------------------------------------------------------
// Convert the input NUL-terminated C-style string pointer from UTF-8 to UTF-16.
// Throws Utf8ConversionException on conversion errors
// (e.g. invalid UTF-8 sequence found in input string).
//------------------------------------------------------------------------------
[[nodiscard]] std::wstring Utf8ToUtf16(_In_opt_z_ const char* utf8);

//==============================================================================

//------------------------------------------------------------------------------
// Error occurred during UTF-8 conversions
//------------------------------------------------------------------------------
class Utf8ConversionException : public std::system_error {
 public:
  // Possible conversion "directions"
  enum class ConversionType { FromUtf8ToUtf16 = 0, FromUtf16ToUtf8 };

  // Initialize with last Win32 error code, error message raw C-string, and
  // conversion direction
  Utf8ConversionException(DWORD errorCode, const char* message,
                          ConversionType type);

  // Initialize with last Win32 error code, error message string, and conversion
  // direction
  Utf8ConversionException(DWORD errorCode, const std::string& message,
                          ConversionType type);

  // Direction of the conversion (e.g. from UTF-8 to UTF-16)
  [[nodiscard]] ConversionType Direction() const noexcept;

 private:
  // Direction of the conversion
  ConversionType m_conversionType;
};

//------------------------------------------------------------------------------
//          Utf8ConversionException Inline Method Implementations
//------------------------------------------------------------------------------

inline Utf8ConversionException::Utf8ConversionException(
    const DWORD errorCode, const char* const message, const ConversionType type)
    : std::system_error(errorCode, std::system_category(), message),
      m_conversionType(type) {}

inline Utf8ConversionException::Utf8ConversionException(
    const DWORD errorCode, const std::string& message,
    const ConversionType type)
    : std::system_error(errorCode, std::system_category(), message),
      m_conversionType(type) {}

inline Utf8ConversionException::ConversionType
Utf8ConversionException::Direction() const noexcept {
  return m_conversionType;
}

//------------------------------------------------------------------------------
//                          Private Helper Functions
//------------------------------------------------------------------------------

namespace detail {

//------------------------------------------------------------------------------
// Returns true if the input 'size_t' value overflows the maximum value
// that can be stored in an 'int'
//------------------------------------------------------------------------------
[[nodiscard]] inline bool ValueOverflowsInt(size_t value) {
  if (value > static_cast<size_t>((std::numeric_limits<int>::max)())) {
    return true;
  } else {
    return false;
  }
}

//------------------------------------------------------------------------------
// Returns true if the input string pointer is null or it points to an empty
// string ('\0')
//------------------------------------------------------------------------------
[[nodiscard]] inline bool IsNullOrEmpty(_In_opt_z_ const char* psz) {
  if (psz == nullptr) {
    return true;
  }

  if (*psz == '\0') {
    return true;
  }

  return false;
}

//------------------------------------------------------------------------------
// Returns true if the input string pointer is null or it points to an empty
// string (L'\0')
//------------------------------------------------------------------------------
[[nodiscard]] inline bool IsNullOrEmpty(_In_opt_z_ const wchar_t* psz) {
  if (psz == nullptr) {
    return true;
  }

  if (*psz == L'\0') {
    return true;
  }

  return false;
}

}  // namespace detail

//
// Convenience macro:
//
// ..._ALWAYS_CHECK_INTEGER_OVERFLOWS =
// !(..._CHECK_INTEGER_OVERFLOWS_ONLY_IN_DEBUG)
//
// NOTE: Users of this library should #define (or undefine) the macro
// GIOVANNI_DICANIO_UTF8CONV_CHECK_INTEGER_OVERFLOWS_ONLY_IN_DEBUG
// (see comments at the beginning of this header file).
//
// This ...UTF8CONV_ALWAYS_CHECK_INTEGER_OVERFLOWS macro here
// is for library's *private code* only.
//
#ifndef GIOVANNI_DICANIO_UTF8CONV_CHECK_INTEGER_OVERFLOWS_ONLY_IN_DEBUG
#define GIOVANNI_DICANIO_UTF8CONV_ALWAYS_CHECK_INTEGER_OVERFLOWS
#endif

//------------------------------------------------------------------------------
//             Unicode Encoding Conversion Function Implementations
//------------------------------------------------------------------------------

inline std::string Utf16ToUtf8(const std::wstring& utf16) {
  if (utf16.empty()) {
    return std::string{};
  }

  return Utf16ToUtf8(std::wstring_view{utf16.data(), utf16.size()});
}

inline std::string Utf16ToUtf8(_In_opt_z_ const wchar_t* utf16) {
  if (detail::IsNullOrEmpty(utf16)) {
    return std::string{};
  }

//
// The following line generates a Warning C6387
// when compiled with VS2019 v16.9.1:
//
// ---------------------------------------------------------------------------------------
// 'utf16' could be '0':  this does not adhere to the specification
// for the function 'std::basic_string_view<wchar_t,std::char_traits<wchar_t>
// >::{ctor}'.
// ---------------------------------------------------------------------------------------
//
// But the code analyzer was unable to understand that I *did* a proper check
// for nullptr in the above detail::IsNullOrEmpty() call.
//
// So, this is actually a spurious warning, that I'm disabling it here:
//
#pragma warning(suppress : 6387)
  return Utf16ToUtf8(std::wstring_view{utf16});
}

inline std::string Utf16ToUtf8(std::wstring_view utf16) {
  if (utf16.empty()) {
    return std::string{};
  }

  // Ignore invalid char
  constexpr DWORD kFlags = 0;

#ifdef GIOVANNI_DICANIO_UTF8CONV_ALWAYS_CHECK_INTEGER_OVERFLOWS
  // Safely cast the length of the source UTF-16 string from size_t
  // (returned by std::wstring_view::length()) to int
  // for the WideCharToMultiByte API.
  // If the size_t value is too big, throw an exception to prevent overflows.
  if (detail::ValueOverflowsInt(utf16.length())) {
    throw std::overflow_error(
        "[Utf8Conv::Utf16ToUt8] Input string too long: size_t-length doesn't "
        "fit into int.");
  }
#else
  // Only check in debug-builds
  _ASSERTE(!detail::ValueOverflowsInt(utf16.length()));
#endif

  const int utf16Length = static_cast<int>(utf16.length());

  // Get the length, in chars, of the resulting UTF-8 string
  const int utf8Length = ::WideCharToMultiByte(
      CP_UTF8,          // convert to UTF-8
      kFlags,           // conversion flags
      utf16.data(),     // source UTF-16 string
      utf16Length,      // length of source UTF-16 string, in wchar_ts
      nullptr,          // unused - no conversion required in this step
      0,                // request size of destination buffer, in chars
      nullptr, nullptr  // unused
  );
  if (utf8Length == 0) {
    // Conversion error: capture error code and throw
    const DWORD error = ::GetLastError();
    throw Utf8ConversionException(
        error,
        error == ERROR_NO_UNICODE_TRANSLATION
            ? "[Utf8Conv::Utf16ToUtf8] Invalid UTF-16 sequence found in input "
              "string."
            : "[Utf8Conv::Utf16ToUtf8] Cannot get result string length when "
              "converting "
              "from UTF-16 to UTF-8 (WideCharToMultiByte failed).",
        Utf8ConversionException::ConversionType::FromUtf16ToUtf8);
  }

  // Make room in the destination string for the converted bits
  std::string utf8(utf8Length, ' ');

  // Do the actual conversion from UTF-16 to UTF-8
  int result = ::WideCharToMultiByte(
      CP_UTF8,          // convert to UTF-8
      kFlags,           // conversion flags
      utf16.data(),     // source UTF-16 string
      utf16Length,      // length of source UTF-16 string, in wchar_ts
      utf8.data(),      // pointer to destination buffer
      utf8Length,       // size of destination buffer, in chars
      nullptr, nullptr  // unused
  );
  if (result == 0) {
    // Conversion error: capture error code and throw
    const DWORD error = ::GetLastError();
    throw Utf8ConversionException(
        error,
        error == ERROR_NO_UNICODE_TRANSLATION
            ? "[Utf8Conv::Utf16ToUtf8] Invalid UTF-16 sequence found in input "
              "string."
            : "[Utf8Conv::Utf16ToUtf8] Cannot convert from UTF-16 to UTF-8 "
              "(WideCharToMultiByte failed).",
        Utf8ConversionException::ConversionType::FromUtf16ToUtf8);
  }

  return utf8;
}

inline std::wstring Utf8ToUtf16(const std::string& utf8) {
  if (utf8.empty()) {
    return std::wstring{};
  }

  return Utf8ToUtf16(std::string_view{utf8.data(), utf8.size()});
}

inline std::wstring Utf8ToUtf16(_In_opt_z_ const char* utf8) {
  if (detail::IsNullOrEmpty(utf8)) {
    return std::wstring{};
  }

//
// The following line generates a Warning C6387
// when compiled with VS2019 v16.9.1:
//
// ---------------------------------------------------------------------------------
// 'utf8' could be '0':  this does not adhere to the specification
// for the function 'std::basic_string_view<char,std::char_traits<char>
// >::{ctor}'.
// ---------------------------------------------------------------------------------
//
// But the code analyzer was unable to understand that I *did* a proper check
// for nullptr in the above detail::IsNullOrEmpty() call.
//
// So, this is actually a spurious warning, that I'm disabling it here:
//
#pragma warning(suppress : 6387)
  return Utf8ToUtf16(std::string_view{utf8});
}

inline std::wstring Utf8ToUtf16(std::string_view utf8) {
  if (utf8.empty()) {
    return std::wstring{};
  }

  // Ignore invalid char
  constexpr DWORD kFlags = 0;

#ifdef GIOVANNI_DICANIO_UTF8CONV_ALWAYS_CHECK_INTEGER_OVERFLOWS
  // Safely cast the length of the source UTF-8 string from size_t
  // (returned by std::string_view::length()) to int
  // for the MultiByteToWideChar API.
  // If the size_t value is too big, throw an exception to prevent overflows.
  if (detail::ValueOverflowsInt(utf8.length())) {
    throw std::overflow_error(
        "[Utf8Conv::Utf8ToUtf16] Input string too long: size_t-length doesn't "
        "fit into int.");
  }
#else
  // Only check in debug-builds
  _ASSERTE(!detail::ValueOverflowsInt(utf8.length()));
#endif

  const int utf8Length = static_cast<int>(utf8.length());

  // Get the size of the destination UTF-16 string
  const int utf16Length = ::MultiByteToWideChar(
      CP_UTF8,      // source string is in UTF-8
      kFlags,       // conversion flags
      utf8.data(),  // source UTF-8 string pointer
      utf8Length,   // length of the source UTF-8 string, in chars
      nullptr,      // unused - no conversion done in this step
      0             // request size of destination buffer, in wchar_ts
  );
  if (utf16Length == 0) {
    // Conversion error: capture error code and throw
    const DWORD error = ::GetLastError();
    throw Utf8ConversionException(
        error,
        error == ERROR_NO_UNICODE_TRANSLATION
            ? "[Utf8Conv::Utf8ToUtf16] Invalid UTF-8 sequence found in input "
              "string."
            : "[Utf8Conv::Utf8ToUtf16] Cannot get result string length when "
              "converting "
              "from UTF-8 to UTF-16 (MultiByteToWideChar failed).",
        Utf8ConversionException::ConversionType::FromUtf8ToUtf16);
  }

  // Make room in the destination string for the converted bits
  std::wstring utf16(utf16Length, ' ');

  // Do the actual conversion from UTF-8 to UTF-16
  int result = ::MultiByteToWideChar(
      CP_UTF8,       // source string is in UTF-8
      kFlags,        // conversion flags
      utf8.data(),   // source UTF-8 string pointer
      utf8Length,    // length of source UTF-8 string, in chars
      utf16.data(),  // pointer to destination buffer
      utf16Length    // size of destination buffer, in wchar_ts
  );
  if (result == 0) {
    // Conversion error: capture error code and throw
    const DWORD error = ::GetLastError();
    throw Utf8ConversionException(
        error,
        error == ERROR_NO_UNICODE_TRANSLATION
            ? "[Utf8Conv::Utf8ToUtf16] Invalid UTF-8 sequence found in input "
              "string."
            : "[Utf8Conv::Utf8ToUtf16] Cannot convert from UTF-8 to UTF-16 "
              "(MultiByteToWideChar failed).",
        Utf8ConversionException::ConversionType::FromUtf8ToUtf16);
  }

  return utf16;
}

}  // namespace Utf8Conv

#endif  // GIOVANNI_DICANIO_UTF8CONV_HPP_INCLUDED
