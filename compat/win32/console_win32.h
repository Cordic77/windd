#ifndef WINDD_CONSOLE_H_
#define WINDD_CONSOLE_H_

#include <conio.h>          /* _getch() */

/* Win32 console colors: */
enum EConsoleColor
{
  DEFAULT_COLOR = -1,
  BLACK         =  0,
  BLUE          = FOREGROUND_BLUE,
  GREEN         = FOREGROUND_GREEN,
  CYAN          = FOREGROUND_GREEN | FOREGROUND_BLUE,
  RED           = FOREGROUND_RED,
  MAGENTA       = FOREGROUND_RED | FOREGROUND_BLUE,
  BROWN         = FOREGROUND_RED | FOREGROUND_GREEN,
  LIGHTGRAY     = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
  DARKGRAY      = FOREGROUND_INTENSITY,
  LIGHTBLUE     = FOREGROUND_INTENSITY | FOREGROUND_BLUE,
  LIGHTGREEN    = FOREGROUND_INTENSITY | FOREGROUND_GREEN,
  LIGHTCYAN     = FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE,
  LIGHTRED      = FOREGROUND_INTENSITY | FOREGROUND_RED,
  LIGHTMAGENTA  = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE,
  YELLOW        = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN,
  WHITE         = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
};

/* */
extern _locale_t
  cons_locale;
extern FILE
 *kbd_stdin,
 *scr_stdout,
 *scr_stderr;

/* */
extern bool
  InitializeConsole (void);
extern void
  RestoreConsole (void);
extern int
  IsDescriptorRedirected (int fd);
extern int
  IsFileRedirected (FILE *stream);
extern bool
  IsHandleRedirected (HANDLE handle);
extern void
  FlushStdout (void);
extern void
  FlushStderr (void);

/* Windows console: */
extern int
  gotoxy (int x, int y);
extern int
  getcursorxy (int *x, int *y);
extern int
  getcursory (void);
extern int
  clrscr (void);
extern int
  textcolor (enum ETextColor color);
extern int
  textbackground (enum ETextColor color);
extern FILE *
  GetConsoleFile (FILE *stream, bool *print_twice);
extern char
  PromptUserYesNo (void);


/*===  scanf functions  ===*/

/* Unfortunately, VS2008 doesn't support vfscanf() as it's a C99 feature. */

/* int vfscanf (FILE *stream, char const format [], va_list ap);
   http://pubs.opengroup.org/onlinepubs/9699919799/functions/vfscanf.html
*/
#if _MSC_VER >= 1800  /* VS2013 */
  _Check_return_opt_ int
    (kbdvfscanf) (_Inout_ FILE * const stream, _In_z_ _Printf_format_string_ char const* const format, va_list argptr);
  _Check_return_opt_ int
    (kbdvfwscanf) (_Inout_ FILE * const stream, _In_z_ _Printf_format_string_ wchar_t const* const format, va_list argptr);

  #ifdef vfscanf
  #undef vfscanf
  #endif
  #define vfscanf       kbdvfscanf

  #ifdef vfwscanf
  #undef vfwscanf
  #endif
  #define vfwscanf      kbdvfwscanf

  #ifdef _vftscanf
  #undef _vftscanf
  #endif

  #ifdef _UNICODE
  #define _kbdvftscanf  kbdvfwscanf
  #define _vftscanf     kbdvfwscanf
  #else
  #define _kbdvftscanf  kbdvfscanf
  #define _vftscanf     kbdvfscanf
  #endif
#endif

/* int fscanf (FILE *stream, char const format [], ...);
   http://pubs.opengroup.org/onlinepubs/9699919799/functions/fscanf.html
*/
extern _Check_return_opt_ int
  (kbdfscanf) (_Inout_ FILE * const stream, _In_z_ _Scanf_format_string_ char const* const format, ...);
extern _Check_return_opt_ int
  (kbdfwscanf) (_Inout_ FILE * const stream, _In_z_ _Scanf_format_string_ wchar_t const* const format, ...);

#if _MSC_VER < 1800  /* VS2013 */
#define kbdfscanf(stream, format, ...) (_fscanf_l) (GetConsoleFile(stream,NULL), format, cons_locale, __VA_ARGS__)
#define kbdfwscanf(stream, format, ...) (_fwscanf_l) (GetConsoleFile(stream,NULL), format, cons_locale, __VA_ARGS__)
#endif

#ifdef fscanf
#undef fscanf
#endif
#define fscanf        kbdfscanf

#ifdef fwscanf
#undef fwscanf
#endif
#define fwscanf       kbdfwscanf

#ifdef _ftscanf
#undef _ftscanf
#endif

#ifdef _UNICODE
#define _kbdftscanf   kbdfwscanf
#define _ftscanf      kbdfwscanf
#else
#define _kbdftscanf   kbdfscanf
#define _ftscanf      kbdfscanf
#endif


/* int vscanf (char const format [], va_list ap);
   http://pubs.opengroup.org/onlinepubs/9699919799/functions/vscanf.html
*/
#if _MSC_VER >= 1800  /* VS2013 */
  _Check_return_opt_ int
    (kbdvscanf) (_In_z_ _Printf_format_string_ char const* const format, va_list argptr);
  _Check_return_opt_ int
    (kbdvwscanf) (_In_z_ _Printf_format_string_ wchar_t const* const format, va_list argptr);

  #ifdef vscanf
  #undef vscanf
  #endif
  #define vscanf        kbdvscanf

  #ifdef vwscanf
  #undef vwscanf
  #endif
  #define vwscanf       kbdvwscanf

  #ifdef _vtscanf
  #undef _vtscanf
  #endif

  #ifdef _UNICODE
  #define _kbdvtscanf   kbdvwscanf
  #define _vtscanf      kbdvwscanf
  #else
  #define _kbdvtscanf   kbdvscanf
  #define _vtscanf      kbdvscanf
  #endif
#endif

/* int scanf (char const format [], ...);
   http://pubs.opengroup.org/onlinepubs/9699919799/functions/scanf.html
*/
_Check_return_opt_ int
  (kbdscanf) (_In_z_ _Scanf_format_string_ char const* const format, ...);
_Check_return_opt_ int
  (kbdwscanf) (_In_z_ _Scanf_format_string_ wchar_t const* const format, ...);

#if _MSC_VER < 1800  /* VS2013 */
#define kbdscanf(stream, format, ...) (_fscanf_l) (GetConsoleFile(stdin,NULL), format, cons_locale, __VA_ARGS__)
#define kbdwscanf(stream, format, ...) (_fwscanf_l) (GetConsoleFile(stdin,NULL), format, cons_locale, __VA_ARGS__)
#endif

#ifdef scanf
#undef scanf
#endif
#define scanf         kbdscanf

#ifdef wscanf
#undef wscanf
#endif
#define wscanf        kbdwscanf

#ifdef _tscanf
#undef _tscanf
#endif

#ifdef _UNICODE
#define _kbdtscanf    kbdwscanf
#define _tscanf       kbdwscanf
#else
#define _kbdtscanf    kbdscanf
#define _tscanf       kbdscanf
#endif


/*===  printf functions  ===*/

/* int vfprintf (FILE *stream, char const format [], va_list ap);
   http://pubs.opengroup.org/onlinepubs/9699919799/functions/vfprintf.html
*/
_Check_return_opt_ int
  (scrvfprintf) (_Inout_ FILE * const stream, _In_z_ _Printf_format_string_ char const* const format, va_list argptr);
_Check_return_opt_ int
  (scrvfwprintf) (_Inout_ FILE * const stream, _In_z_ _Printf_format_string_ wchar_t const* const format, va_list argptr);

#ifdef vfprintf
#undef vfprintf
#endif
#define vfprintf      scrvfprintf

#ifdef vfwprintf
#undef vfwprintf
#endif
#define vfwprintf     scrvfwprintf

#ifdef _vftprintf
#undef _vftprintf
#endif

#ifdef _UNICODE
#define _scrvftprintf scrvfwprintf
#define _vftprintf    scrvfwprintf
#else
#define _scrvftprintf scrvfprintf
#define _vftprintf    scrvfprintf
#endif

/* int fprintf (FILE *stream, char const format [], ...);
   http://pubs.opengroup.org/onlinepubs/009695399/functions/fprintf.html
*/
_Check_return_opt_ int
  (scrfprintf) (_Inout_ FILE * const stream, _In_z_ _Printf_format_string_ char const* const format, ...);
_Check_return_opt_ int
  (scrfwprintf) (_Inout_ FILE * const stream, _In_z_ _Printf_format_string_ wchar_t const* const format, ...);

#ifdef fprintf
#undef fprintf
#endif
#define fprintf       scrfprintf

#ifdef fwprintf
#undef fwprintf
#endif
#define fwprintf      scrfwprintf

#ifdef _ftprintf
#undef _ftprintf
#endif

#ifdef _UNICODE
#define _scrftprintf  scrfwprintf
#define _ftprintf     scrfwprintf
#else
#define _scrftprintf  scrfprintf
#define _ftprintf     scrfprintf
#endif


/* int vprintf (char const format [], va_list ap);
   http://pubs.opengroup.org/onlinepubs/9699919799/functions/vprintf.html
*/
_Check_return_opt_ int
  (scrvprintf) (_In_z_ _Printf_format_string_ char const* const format, va_list argptr);
_Check_return_opt_ int
  (scrvwprintf) (_In_z_ _Printf_format_string_ wchar_t const* const format, va_list argptr);

#ifdef vprintf
#undef vprintf
#endif
#define vprintf       scrvprintf

#ifdef vwprintf
#undef vwprintf
#endif
#define vwprintf      scrvwprintf

#ifdef _vtprintf
#undef _vtprintf
#endif

#ifdef _UNICODE
#define _scrvtprintf  scrvwprintf
#define _vtprintf     scrvwprintf
#else
#define _scrvtprintf  scrvprintf
#define _vtprintf     scrvprintf
#endif

/* int printf (char const format [], ...);
   http://pubs.opengroup.org/onlinepubs/9699919799/utilities/printf.html
*/
_Check_return_opt_ int
  (scrprintf) (_In_z_ _Printf_format_string_ char const* const format, ...);
_Check_return_opt_ int
  (scrwprintf) (_In_z_ _Printf_format_string_ wchar_t const* const format, ...);

#ifdef printf
#undef printf
#endif
#define printf        scrprintf

#ifdef wprintf
#undef wprintf
#endif
#define wprintf       scrwprintf

#ifdef _tprintf
#undef _tprintf
#endif

#ifdef _UNICODE
#define _scrtprintf   scrwprintf
#define _tprintf      scrwprintf
#else
#define _scrtprintf   scrprintf
#define _tprintf      scrprintf
#endif

#endif
