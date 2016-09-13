#ifndef WINDD_UNICODE_H_
#define WINDD_UNICODE_H_

/* UTF-16 still the default on Windows NT: */
#if !defined(UNICODE)
  #define UNICODE                 /* UNICODE is used by Windows headers */
#endif
#if !defined(_UNICODE)
  #define _UNICODE                /* _UNICODE is used by C-runtime/MFC headers */
#endif

#ifdef UNICODE
  #define FMT_UNI TEXT("%s")
#else
  #define FMT_UNI "%S"
#endif

#endif
