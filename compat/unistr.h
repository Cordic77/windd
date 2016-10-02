#ifndef WINDD_UNISTR_H_
#define WINDD_UNISTR_H_

/* */
#ifndef UTF8_T_DEFINED
  typedef char utf8_t;
  #define UTF8_T_DEFINED
#endif

/* */
extern bool
  FormatInt64 (int64_t const number, wchar_t str [], size_t count);

/* */
extern char * __cdecl
  stristr (char str [], char const substr []); /*strcasecmpA*/
extern wchar_t * __cdecl
  wcsistr (wchar_t str [], wchar_t const substr []); /*strcasecmpW*/

#ifdef _UNICODE
#define _tcsistr wcsistr
#else
#define _tcsistr stristr
#endif

extern void
  TrimStringW (wchar_t str []);
extern void
  TrimStringA (char str []);

#ifdef UNICODE
#define TrimString TrimStringW
#else
#define TrimString TrimStringA
#endif

extern void
  TrimRightA (char str []);
extern void
  TrimRightW (wchar_t str []);

#ifdef UNICODE
#define TrimRight TrimRightW
#else
#define TrimRight TrimRightA
#endif

/* */
utf8_t *
  utf16_to_utf8_ex (__in const wchar_t utf16 [], __in int const additional_bytes);
utf8_t *
  utf16_to_utf8 (__in const wchar_t utf16 []);
wchar_t *
  utf8_to_utf16_ex (__in const char utf8 [], __in int const additional_chars);
wchar_t *
  utf8_to_utf16 (__in const char utf8 []);

#endif
