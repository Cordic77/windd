#include "win32utf16.h"           /* switch to UTF-16 in this file! */

#include <config.h>


/* */
static LPTSTR
  Win32ErrorMessage (DWORD error_msg_id, bool NTStatusCode);
static LPTSTR
  COMErrorMessage (HRESULT hresult);


/* Function: */
__declspec(thread) char const *   /* name of last (thread-local) function */
  last_fn;
__declspec(thread) int            /* line number within said function */
  line_no;

extern void RecordFunction (char const function [], int linenum)
{
  last_fn = function;
  line_no = linenum;
}


/* */
static __declspec(thread) int
  last_gle  = ERROR_SUCCESS;      /* last (thread-local) Windows error code */
static __declspec(thread) HRESULT
  last_hr   = S_OK;               /* last (thread-local) HRESULT */
static __declspec(thread) IErrorInfo *
  err_info  = NULL;               /* last (thread-local) IErrorInfo object */

extern void RecordWinErr (void)
{
  last_gle = GetLastError ();
  errno = EWINLASTERR;
}

extern void RecordComErr (void *comobj, HRESULT hres)
{
  last_hr = hres;
  CoReleaseObject (err_info);
  err_info = COMGetErrorInfo (comobj);
  errno = EWINHRESULT;
}

extern void RecordPosixErr (int errnum)
{
  errno = errnum;
}


/* */
extern wchar_t *FormatStringW (wchar_t const user_msg [], ...)
{ wchar_t *str;

  va_list argptr;
  va_start (argptr, user_msg);
  {
  /*size_t chars = _vsctprintf (user_msg, argptr);*/
    size_t chars = _vscwprintf (user_msg, argptr);
    str = (wchar_t *)malloc ((chars + 1)*sizeof(wchar_t));
  /*_vsntprintf (str, chars, user_msg, argptr);*/
    _vsnwprintf (str, chars, user_msg, argptr);
  }
  va_end (argptr);

  TrimStringW (str);
  return (str);
}

extern char *FormatStringA (char const user_msg [], ...)
{ char *str;

  va_list argptr;
  va_start (argptr, user_msg);
  {
  /*size_t chars = _vsctprintf (user_msg, argptr);*/
    size_t chars = _vscprintf (user_msg, argptr);
    str = (char *)malloc (chars + 1);
  /*_vsntprintf (str, chars, user_msg, argptr);*/
    _vsnprintf (str, chars, user_msg, argptr);
  }
  va_end (argptr);

  TrimStringA (str);
  return (str);
}


/*===  Errors in Windows - DWORD (GetLastError) vs HRESULT vs LSTATUS  ===
  http://stackoverflow.com/questions/19547419/errors-in-windows-dword-getlasterror-vs-hresult-vs-lstatus#answer-19548610
*/
extern void SystemErrorEx (int status, char const user_msg [], va_list args)
{
  /* User error message provided? */
  if (user_msg != NULL && user_msg[0] != '\0')
  {
    fprintf (stderr, "\n[%s] ",
                         PROGRAM_NAME);
    vfprintf (stderr, user_msg, args);
    fprintf (stderr, ":\n");
  }
  /* Just print function name! */
  else if (user_msg == MSG_CURR_OP)
  {
    fprintf (stderr, "\n[%s] %s:\n",
                         PROGRAM_NAME, last_fn);
  }

  /* Do we have a Windows: */
  if (errno >= EWINERROR)
  {
    /* Win32 API? */
    if (errno == EWINLASTERR)
    {
      LPTSTR pMessage = Win32ErrorMessage (last_gle, false);

      if (pMessage != NULL)
      {
        TrimRight (pMessage);
        _ftprintf (stderr, TEXT("\n[%s] %s (GetLastError=%") TEXT(PRId32) TEXT(")\n"),
                               TEXT(PROGRAM_NAME), pMessage, last_gle);
        LocalFree ((HLOCAL)pMessage);
      }
      else
        fprintf (stderr, "\n[%s] Unknown Win32 error (GetLastError=%" PRId32 ")\n",
                             PROGRAM_NAME, last_gle);
    }
    // COM HRESULT?
    else// if (errno == EWINHRESULT)
    {
      LPTSTR pMessage = COMErrorMessage (last_hr);
      TrimRight (pMessage);
      _ftprintf (stderr, TEXT("\n[%s] %s (COM error 0x%8") TEXT(PRIX32) TEXT(")\n"),
                             TEXT(PROGRAM_NAME), pMessage, (uint32_t)last_hr);
      free (pMessage);
    }
  }
  // <or> a POSIX error code?
  else
  {
    _ftprintf (stderr, TEXT("\n[%s] %s\n"),
                           TEXT(PROGRAM_NAME), _tcserror (errno));
  }

  fflush (stderr);

  if (status != STATUS_CONTINUE)
    exit (status);
}


extern void SystemError (int status, char const user_msg [], ...)
{ va_list
    argptr;

  va_start (argptr, user_msg);
  SystemErrorEx (status, user_msg, argptr);
  va_end (argptr);
}


/* Overwrite dd's `nl_error()' function: */
extern int progress_len;

extern void
verror (int status, int errnum, const char *format, va_list args);

extern void (nl_error_win32) (int status, int errnum, const char *fmt, ...)
{
  if (0 < progress_len)
    {
      fputc ('\n', stderr);
      progress_len = 0;
    }

  {  /*windd: C89 compatibility*/
  va_list ap;
  va_start (ap, fmt);

  if (errno >= EWINERROR)
    SystemErrorEx (STATUS_CONTINUE, fmt, ap);
  else
    verror (status, errnum, fmt, ap);

  va_end (ap);
  fflush (stderr);
  if (status)
    exit (status);
  }
}


/* Win32 error code message: */
extern void NTStatusError (NTSTATUS errnum)
{
}


/*
// "How to obtain error message descriptions using the FormatMessage API"
// https://support.microsoft.com/en-us/kb/256348
*/
static LPTSTR Win32ErrorMessage (DWORD error_msg_id, bool NTStatusCode)
{ HLOCAL
    pBuffer;      /* Buffer to hold the textual error description */
  HINSTANCE
    hInst;        /* Instance handle for DLL */
  DWORD
    ret;          /* Temp space to hold a return value */

  /* (1) <winerror.h> list several error facilities: */
  if (HRESULT_FACILITY(error_msg_id) == FACILITY_MSMQ)
  {
    /* Load the MSMQ library containing the error message strings: */
    hInst = LoadLibrary (TEXT("MQUTIL.DLL"));

    if(hInst != 0)
    {
      ret = FormatMessage(
          FORMAT_MESSAGE_ALLOCATE_BUFFER |  /* Function will handle memory allocation */
          FORMAT_MESSAGE_FROM_HMODULE |     /* Using a module's message table */
          FORMAT_MESSAGE_IGNORE_INSERTS,    /* No inserts used */
          hInst,                            /* Handle to the DLL */
          error_msg_id,                     /* Message identifier */
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),  /* Default language */
          (LPTSTR)&pBuffer,                 /* Buffer that will hold the text string */
          0,                                /* Allocate at least this many chars for pBuffer */
          NULL                              /* No insert values */
        );
      FreeLibrary (hInst);
    }
  }

  /* NTSTATUS kernel error? */
  else if (NTStatusCode)
  {
    /* Load ntdll.dll library containing the error message strings: */
    hInst = LoadLibrary (TEXT("NTDLL.DLL"));

    if(hInst != 0)
    {
      ret = FormatMessage(
          FORMAT_MESSAGE_ALLOCATE_BUFFER |  /* Function will handle memory allocation */
          FORMAT_MESSAGE_FROM_SYSTEM |      /* System wide message */
          FORMAT_MESSAGE_FROM_HMODULE |     /* Using a module's message table */
          FORMAT_MESSAGE_IGNORE_INSERTS,    /* No inserts used */
          hInst,                            /* Handle to the DLL */
          error_msg_id,                     /* Message identifier */
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),  /* Default language */
          (LPTSTR)&pBuffer,                 /* Buffer that will hold the text string */
          0,                                /* Allocate at least this many chars for pBuffer */
          NULL                              /* No insert values */
        );
      FreeLibrary (hInst);
    }
  }

  /* Could be a network error: */
  else if (error_msg_id >= NERR_BASE && error_msg_id <= MAX_NERR)
  {
    /* Load the library containing network messages: */
    hInst = LoadLibrary (TEXT("NETMSG.DLL"));

    if (hInst != 0)
    {
      ret = FormatMessage(
          FORMAT_MESSAGE_ALLOCATE_BUFFER |  /* The function will allocate memory for the message */
          FORMAT_MESSAGE_FROM_HMODULE |     /* Message definition is in a module */
          FORMAT_MESSAGE_IGNORE_INSERTS,    /* No inserts used */
          hInst,                            /* Handle to the module containing the definition */
          error_msg_id,                     /* Message identifier */
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),  /* Default language */
          (LPTSTR)&pBuffer,                 /* Buffer to hold the text string */
          0,                                /* Smallest size that will be allocated for pBuffer */
          NULL                              /* No insert values */
        );
      FreeLibrary (hInst);
    }
  }

  /* Unknown message source: */
  else
  {
    /* Get the message string from the system: */
    ret = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |  /* The function will allocate space for pBuffer */
        FORMAT_MESSAGE_FROM_SYSTEM |      /* System wide message */
        FORMAT_MESSAGE_IGNORE_INSERTS,    /* No inserts */
        NULL,                             /* Message is not in a module */
        error_msg_id,                     /* Message identifier */
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
        (LPTSTR)&pBuffer,                 /* Buffer to hold the text string */
        0,                                /* The function will allocate at least this much for pBuffer */
        NULL                              /* No insert values */
      );
  }

  /* Number of TCHARs stored in the output buffer greater than zero? */
  if (ret > 0)
    return ((LPTSTR)pBuffer);

  /* Free the buffer: */
  LocalFree (pBuffer);
  return (NULL);
}


/* COM error code message: */
/*
// FACILITY_ITF Error Codes
// http://thrysoee.dk/InsideCOM+/ch06b.htm#147
// c.f. "%VCINSTALLDIR%\include\comdef.h"
*/
static HRESULT const
  WCODE_HRESULT_FIRST = MAKE_HRESULT (SEVERITY_ERROR, FACILITY_ITF, FACILITY_ITF_RESERVED),
  WCODE_HRESULT_LAST  = MAKE_HRESULT (SEVERITY_ERROR, FACILITY_ITF + 1, 0) - 1;

static inline WORD HRESULTToWCode (HRESULT hr)
{
  return ((hr >= WCODE_HRESULT_FIRST && hr <= WCODE_HRESULT_LAST)? (WORD)(hr-WCODE_HRESULT_FIRST) : 0);
}


extern IErrorInfo *COMGetErrorInfo (IUnknown *pUnkn)
{ ISupportErrorInfo *
    pISER;
  HRESULT
    hres;

  /* Does this COM Object support COM Exceptions? */
  hres = COM_OBJ(pUnkn)->QueryInterface (COM_OBJ_THIS_(pUnkn) IID_PPV_ARG (ISupportErrorInfo, &pISER));

  if (SUCCEEDED (hres))
  { IErrorInfo *
      pEI;

    CoReleaseObject (pISER);

    /* Retrieve ErrorInfo object: */
    hres = GetErrorInfo (0, &pEI);

    if (FAILED (hres))
      pEI = NULL;

    return (pEI);
  }

  return (NULL);
}


static LPTSTR COMErrorMessage (HRESULT hresult)
{ TCHAR *
    error_desc = NULL;

  if (err_info != NULL)
  {
    BSTR error_bstr;
    HRESULT hr = COM_OBJ(err_info)->GetDescription (COM_OBJ_THIS_(err_info) &error_bstr);

    if (SUCCEEDED (hr))
    {
      WORD wCode = HRESULTToWCode (last_hr);

      if (wCode != 0)
        error_desc = FormatString (FMT_UNI TEXT(" (IDispatch error #%") TEXT(PRId32) TEXT(")"), error_bstr, (int)wCode);
      else
        error_desc = FormatString (FMT_UNI TEXT(" (HRESULT 0x%8") TEXT(PRIX32) TEXT(")"), error_bstr, (uint32_t)last_hr);

      SysFreeString (error_bstr);
      CoReleaseObject (err_info);
      return (error_desc);
    }
  }

  if (error_desc == NULL)
    error_desc = FormatString (TEXT("Operation failed (HRESULT 0x%8") TEXT(PRIX32) TEXT(")"), (uint32_t)last_hr);

  return (error_desc);
}
