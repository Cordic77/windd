#ifndef WINDD_ERROR_H_
#define WINDD_ERROR_H_

/* Windows NT */
#ifndef _NTDEF_
typedef __success(return >= 0) LONG NTSTATUS, *PNTSTATUS;
#endif

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS      ((NTSTATUS)0x00000000L)
#endif

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

/* POSIX */
#include <errno.h>                          /* EBADF, EIO */
#  ifndef ENOTSUP
#   define ENOTSUP   129
#   define GNULIB_defined_ENOTSUP 1
#  endif

#   define EOPNOTSUPP      130

/* COM */
#include <oaidl.h>                          /* IErrorInfo */

/* */
#define EWINERROR    ((1UL << 30) - 2)      /* Range to encode the type of error that has occurred */
#define EWINHRESULT  (EWINERROR + 0)        /* Error code stored in global `HRESULT' variable */
#define EWINLASTERR  (EWINERROR + 1)        /* [GetLastError()] code stored in `global' int variable */

#define STATUS_CONTINUE  0                  /* Continue program execution */
#define STATUS_ABORT     (!STATUS_CONTINUE)

#define MSG_CURR_OP      NULL

/* */
extern wchar_t *
  FormatErrorA (char const user_msg [], ...);
extern wchar_t *
  FormatErrorW (wchar_t const user_msg [], ...);

#ifdef UNICODE
#define FormatString FormatStringW
#else
#define FormatString FormatStringA
#endif

/* */
extern void
  SystemError (int status, char const user_msg [], ...);
extern void
  (nl_error_win32) (int status, int errnum, const char *fmt, ...);
extern IErrorInfo *
  COMGetErrorInfo (IUnknown *pUnkn);

/* Helper functions: */
extern void
  RecordFunction (char const function [], int linenum);
extern void
  RecordWinErr (void);
extern void
  RecordComErr (void *comobj, HRESULT hres);
extern void
  RecordPosixErr (int errnum);

/* WIN32 errors: */
#define SetWinErr() \
  do {                                          \
    RecordWinErr ();                            \
    RecordFunction (__FUNCTION__, __LINE__);    \
  } while (0)
#define WinErrReturn(retval)                    \
  do {                                          \
    RecordWinErr ();                            \
    RecordFunction (__FUNCTION__, __LINE__);    \
    return (retval);                            \
  } while (0)

/* COM errors: */
extern IErrorInfo *
  com_get_error_info (void);

#define SetComErr(comobj, hresult)              \
  do {                                          \
    RecordComErr (comobj, hresult);             \
    RecordFunction (__FUNCTION__, __LINE__);    \
  } while (0)
#define ComErrReturn(comobj, hresult, retval)   \
  do {                                          \
    RecordComErr (comobj, hresult);             \
    RecordFunction (__FUNCTION__, __LINE__);    \
    return (retval);                            \
  } while (0)

/* POSIX errors: */
#define SetErrno(errnum)                        \
  do {                                          \
    RecordPosixErr (errnum);                    \
    RecordFunction (__FUNCTION__, __LINE__);    \
  } while (0)
#define ErrnoReturn(errnum, retval)             \
  do {                                          \
    RecordPosixErr (errnum);                    \
    RecordFunction (__FUNCTION__, __LINE__);    \
    return (retval);                            \
  } while (0)

#endif
