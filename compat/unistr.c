#include "win32/windows_api.h"

#ifndef _SSIZE_T_DEFINED
  #include <BaseTsd.h>            /* SSIZE_T */
  #undef ssize_t
  typedef SSIZE_T ssize_t;
  #define _SSIZE_T_DEFINED
#endif

#include <stdio.h>                /* fprintf() */
#include <stdlib.h>               /* exit(), _countof() */
#include <stdbool.h>              /* bool */
#include <ctype.h>                /* toupper() */
#include <stdint.h>               /* int64_t */
#include <iso646.h>               /* not, and, or, xor */

#include "unistr.h"

#include "safe_malloc.h"


/* */
static wchar_t
  decimal_sep [3+1], /* = L""; */
  thousand_sep [3+1], /* = L""; */
  grouping_str [9+1]; /* = L""; */
static NUMBERFMTW *
  int_number_fmt; /* = NULL; */


/* See "Easy way to get NUMBERFMT populated with defaults?"
   http://stackoverflow.com/questions/7214669/easy-way-to-get-numberfmt-populated-with-defaults

   Some of the finer points are discussed in this post:
   "Oddities of LOCALE_SGROUPING, NumberGroupSizes and NUMBERFMT"
   https://blogs.msdn.microsoft.com/shawnste/2006/07/17/oddities-of-locale_sgrouping-numbergroupsizes-and-numberfmt/
*/
static void SetupUserDefaultNumberFormat (void)
{
  if (int_number_fmt == NULL)
  {
    int_number_fmt = (NUMBERFMTW *)MemAlloc (1, sizeof(*int_number_fmt));
    memset (int_number_fmt, 0, sizeof(*int_number_fmt));

    /* NumDigits */
    GetLocaleInfoEx (LOCALE_NAME_USER_DEFAULT,
                     LOCALE_IDIGITS | LOCALE_RETURN_NUMBER,
                     (LPWSTR)&int_number_fmt->NumDigits,
                     sizeof(int_number_fmt->NumDigits) / sizeof(WCHAR));

    /* LeadingZero */
    GetLocaleInfoEx (LOCALE_NAME_USER_DEFAULT,
                     LOCALE_ILZERO | LOCALE_RETURN_NUMBER,
                     (LPWSTR)&int_number_fmt->LeadingZero,
                     sizeof(int_number_fmt->LeadingZero) / sizeof(WCHAR));

    /* Grouping */
    GetLocaleInfoEx (LOCALE_NAME_USER_DEFAULT,
                     LOCALE_SGROUPING,
                     grouping_str, _countof(grouping_str));

    /* DecimalSep */
    GetLocaleInfoEx (LOCALE_NAME_USER_DEFAULT,
                     LOCALE_SDECIMAL,
                     decimal_sep, _countof(decimal_sep));
    int_number_fmt->lpDecimalSep  = decimal_sep;

    /* ThousandSep */
    GetLocaleInfoEx (LOCALE_NAME_USER_DEFAULT,
                     LOCALE_STHOUSAND,
                     thousand_sep, _countof(thousand_sep));
    int_number_fmt->lpThousandSep = thousand_sep;

    /* NegativeOrder */
    GetLocaleInfoEx (LOCALE_NAME_USER_DEFAULT,
                     LOCALE_INEGNUMBER | LOCALE_RETURN_NUMBER,
                     (LPWSTR)&int_number_fmt->NegativeOrder,
                     sizeof(int_number_fmt->NegativeOrder) / sizeof(WCHAR));

    /* Overwrite default: */
    int_number_fmt->NumDigits     = 0;
    int_number_fmt->Grouping      = 3;
    int_number_fmt->NegativeOrder = 1;
  }
}


extern bool FormatInt64 (int64_t const number, wchar_t str [], size_t count)
{ wchar_t
    temp [31+1];

  SetupUserDefaultNumberFormat ();
  _snwprintf (temp, _countof(temp), L"%I64d", number);

  return (GetNumberFormatEx (LOCALE_NAME_USER_DEFAULT, 0, temp,
                             int_number_fmt, str, (int)count) != 0);
}


extern char * __cdecl stristr (char str [], char const substr [])
{ char const *
    p_pos = str;

  if (not *substr)  /* == TEXT('\0') */
    return ((char *)p_pos);

  while (*p_pos)  /* != TEXT('\0') */
  {
    if (toupper (*p_pos) == toupper (*substr))
    {
      char const *cur1 = p_pos + 1;
      char const *cur2 = substr + 1;

      while (*cur1  /* != TEXT('\0') */
          && *cur2  /* != TEXT('\0') */
          && toupper (*cur1) == toupper (*cur2))
      {
        ++cur1;
        ++cur2;
      }

      if (not *cur2)  /* == TEXT('\0') */
        return ((char *)p_pos);
    }

    ++p_pos;
  }

  return (NULL);
}

extern wchar_t * __cdecl wcsistr (wchar_t str [], wchar_t const substr [])
{ wchar_t const *
    p_pos = str;

  if (not *substr)  /* == TEXT('\0') */
    return ((wchar_t *)p_pos);

  while (*p_pos)  /* != TEXT('\0') */
  {
    if (towupper (*p_pos) == towupper (*substr))
    {
      wchar_t const *cur1 = p_pos + 1;
      wchar_t const *cur2 = substr + 1;

      while (*cur1  /* != TEXT('\0') */
          && *cur2  /* != TEXT('\0') */
          && towupper (*cur1) == towupper (*cur2))
      {
        ++cur1;
        ++cur2;
      }

      if (not *cur2)  /* == TEXT('\0') */
        return ((wchar_t *)p_pos);
    }

    ++p_pos;
  }

  return (NULL);
}


extern void TrimStringW (wchar_t str [])
{ size_t
    s, e, l;

  if ((l=wcslen (str)) == 0)
    return;

  for (s=0; s < l; s++)
    if (!iswspace (str [s]) && !iswcntrl (str [s]))
      break;

  for (e=l-1; e >= s; e--)
    if (!iswspace (str [e]) && !iswcntrl (str [e]))
      break;

  if (e >= s)
  {
    if (s > 0)
      memmove (str, str + s, (e - s + 1)*sizeof(wchar_t));
    str [e - s + 1] = L'\0';
  }
  else
    str [0] = L'\0';
}

extern void TrimStringA (char str [])
{ size_t
    s, e, l;

  if ((l=strlen (str)) == 0)
    return;

  for (s=0; s < l; s++)
    if (!isspace (str [s]) && !iscntrl (str [s]))
      break;

  for (e=l-1; e >= s; e--)
    if (!isspace (str [e]) && !iscntrl (str [e]))
      break;

  if (e >= s)
  {
    if (s > 0)
      memmove (str, str + s, e - s + 1);
    str [e - s + 1] = '\0';
  }
  else
    str [0] = '\0';
}


extern void TrimRightW (wchar_t str [])
{ ssize_t
    end,
    last;

  if ((last=wcslen (str)) == 0)
    return;

  /* Right: */
  for (end=last-1; end >= 0; --end)
    if (!iswspace (str [end]))
      break;

  /* Truncate: */
  if (end >= 0)
    str [end + 1] = '\0';
  else
    str [0] = '\0';
}


extern void TrimRightA (char str [])
{ ssize_t
    end,
    last;

  if ((last=strlen (str)) == 0)
    return;

  /* Right: */
  for (end=last-1; end >= 0; --end)
    if (!isspace (str [end]))
      break;

  /* Truncate: */
  if (end >= 0)
    str [end + 1] = '\0';
  else
    str [0] = '\0';
}


/* Inspired by Giovanni Dicanio "Routines to convert between Unicode UTF-8
   and Unicode UTF-16", giovanni.dicanio@gmail.com, Last update: 2010-01-02
*/
utf8_t *utf16_to_utf8_ex (__in const wchar_t utf16 [], __in int const additional_bytes)
{
  /* Special case of NULL or empty input string */
  if (utf16 == NULL || utf16[0] == L'\0')
  {
    utf8_t *empty = MemAlloc (1, sizeof(char));
    empty[0] = '\0';
    return (empty);
  }

  /* Convert input string: */
  {
    /* WC_ERR_INVALID_CHARS flag is set to fail if invalid input character
    // is encountered.
    // This flag is supported on Windows Vista and later.
    // Don't use it on Windows XP and previous. */
    #if (WINVER >= 0x0600)
    DWORD dwConversionFlags = WC_ERR_INVALID_CHARS;
    #else
    DWORD dwConversionFlags = 0;
    #endif

    /* Source string length (including end-of-string \0): */
    size_t utf16_char_len = wcslen (utf16) + 1;

    /* Get required destination string length: */
    size_t utf8_byte_len = (size_t)WideCharToMultiByte (
        CP_UTF8,              /* convert to UTF-8 */
        dwConversionFlags,    /* specify conversion behavior */
        utf16,                /* source UTF-16 string */
        (int)utf16_char_len,  /* source string length in WCHAR's */
        NULL,                 /* unused - no conversion required in this step */
        0,                    /* request buffer size */
        NULL, NULL            /* unused */
      );

    /* Consider this to be a critical error! */
    if (utf8_byte_len == 0)
    {
      fprintf (stderr, "\nUTF16 to UTF8 conversion: destination byte length is zero\n");
      exit (252);
    }

    /* Allocate destination buffer for UTF-8 string */
    {
      char *utf8 = (char *)MemAlloc (utf8_byte_len + additional_bytes, sizeof(char));

      /* ... and do the conversion from UTF-16 to UTF-8: */
      if (WideCharToMultiByte (
            CP_UTF8,              /* convert to UTF-8 */
            dwConversionFlags,    /* specify conversion behavior */
            utf16,                /* source UTF-16 string */
            (int)utf16_char_len,  /* source string length in WCHAR's */
            utf8,                 /* destination buffer */
            (int)utf8_byte_len,   /* destination buffer size, in bytes */
            NULL, NULL) == 0)     /* unused */
      {
        fprintf (stderr, "\nUTF16 to UTF8 conversion: WideCharToMultiByte() failed\n");
        exit (252);
      }

      /* Return resulting UTF-8 string: */
      return (utf8);
    }
  }
}

utf8_t *utf16_to_utf8 (__in const wchar_t utf16 [])
{
  return (utf16_to_utf8_ex (utf16, 0));
}


wchar_t *utf8_to_utf16_ex (__in const char utf8 [], __in int const additional_chars)
{
  /* Special case of NULL or empty input string */
  if (utf8 == NULL || utf8[0] == '\0')
  {
    wchar_t *empty = MemAlloc (1, sizeof(wchar_t));
    empty[0] = '\0';
    return (empty);
  }

  /* Convert input string: */
  {
    /* Source string length (including end-of-string \0): */
    size_t utf8_byte_len = strlen (utf8) + 1;

    /* Get required destination string length: */
    size_t utf16_char_len = (size_t)MultiByteToWideChar (
        CP_UTF8,              /* convert to UTF-8 */
        MB_ERR_INVALID_CHARS, /* specify conversion behavior */
        utf8,                 /* source UTF-8 string */
        (int)utf8_byte_len,   /* source string length in char's */
        NULL,                 /* unused - no conversion done in this step */
        0                     /* request size of destination buffer, in WCHAR's */
      );

    /* Consider this to be a critical error! */
    if (utf16_char_len == 0)
    {
      fprintf (stderr, "\nUTF8 to UTF16 conversion: destination char length is zero\n");
      exit (251);
    }

    /* Allocate destination buffer for UTF-8 string */
    {
      wchar_t *utf16 = (wchar_t *)MemAlloc (utf16_char_len + additional_chars, sizeof(wchar_t));

      /* ... and do the conversion from UTF-8 to UTF-16: */
      if (MultiByteToWideChar (
            CP_UTF8,              /* convert to UTF-8 */
            MB_ERR_INVALID_CHARS, /* specify conversion behavior */
            utf8,                 /* source UTF-8 string */
            (int)utf8_byte_len,   /* source string length in char's */
            utf16,                /* destination buffer */
            (int)utf16_char_len) == 0)  /* size of destination buffer, in WCHAR's */
      {
        fprintf (stderr, "\nUTF8 to UTF16 conversion: MultiByteToWideChar() failed\n");
        exit (251);
      }

      /* Return resulting UTF-16 string: */
      return (utf16);
    }
  }
}

wchar_t *utf8_to_utf16 (__in const char utf8 [])
{
  return (utf8_to_utf16_ex (utf8, 0));
}
