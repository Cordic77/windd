#ifndef WINDD_UNICODE_H_
#define WINDD_UNICODE_H_

/* UTF-16 still the default on Windows NT: */
#if !defined(UNICODE)
  #define UNICODE                 /* UNICODE is used by Windows headers */
#endif
#if !defined(_UNICODE)
  #define _UNICODE                /* _UNICODE is used by C-runtime/MFC headers */
#endif

/* http://stackoverflow.com/questions/10000723/visual-studio-swprintf-is-making-all-my-s-formatters-want-wchar-t-instead-of#answer-10001238
   In Visual Studio 13 and earlier %s/%c is mapped to the natural width
   of the function/format string and %S/%C is mapped to the opposite of
   the natural with.

   You can also force a specific width by using a length modifier:
   %ls, %lc, %ws and %wc always mean wchar_t,
   and %hs and %hc are always char.
*/
#ifdef UNICODE
  #define FMT_UNI TEXT("%s")
#else
  #define FMT_UNI "%ls"
#endif

#endif
