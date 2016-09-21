#include "win32utf16.h"           /* switch to UTF-16 in this file! */

#include <config.h>


/*                                // Thread-local storage: CoInitializeEx() must */
extern __declspec(thread) bool /* // be called exactly once for each thread! */
  initialized_com = false;


/* */
extern bool InitializeCOM (void)
{ HRESULT
    hres;

  if (initialized_com)
    return (true);

  /* 1. INITIALIZE COM: */
  hres = CoInitializeEx (0, COINIT_MULTITHREADED);  /* WinNT4, Windows 98, Windows 95 with DCOM */

  if (FAILED (hres))
  {
    /* Only record that we initialized COM if that is what we really
    // did. An error code of RPC_E_CHANGED_MODE means that the call
    // to CoInitialize failed because COM had already been initialized
    // on another mode - which isn't a fatal condition and so in this
    // case we don't want to call CoUninitialize:
    */
    if (hres != RPC_E_CHANGED_MODE)
    {
      SetComErr (NULL, hres);
      SystemError (STATUS_ABORT, MSG_CURR_OP);
    }
  }

  initialized_com = true;
  return (initialized_com);
}


extern bool COMAvailable (void)
{
  return (initialized_com);
}


extern TCHAR *BSTR2TCHAR (BSTR bstr)
{ TCHAR *
    str;

  size_t len = wcslen (bstr);
  str = (TCHAR *)MemAlloc (len + 1, sizeof(TCHAR));

  #if defined(UNICODE)
  _tcscpy (str, bstr);
  #else
  if (WideCharToMultiByte (CP_OEMCP, 0, bstr, len, str, len, NULL, NULL) == 0)
    WinErrReturn (NULL);
  #endif

  TrimString (str);
  return (str);
}


extern TCHAR *VAR2TCHAR (VARIANT const *var)
{ TCHAR *
    str;

  if ((var->vt & VT_NULL) == VT_NULL)
  {
    str = (TCHAR *)MemAlloc (1, sizeof(TCHAR));
    str [0] = '\0';
    return (str);
  }

  return (BSTR2TCHAR (var->bstrVal));
}


extern TCHAR *GUID2TCHAR (GUID const *guid)
{ OLECHAR *
    guid_str;
  TCHAR *
    str;

  StringFromCLSID (guid, &guid_str);
  str = (TCHAR *)MemAlloc(_tcslen(guid_str) + 1, sizeof(TCHAR));
  _tcscpy (str, guid_str);
  CoTaskMemFree (guid_str);

  return (str);
}


extern void UninitializeCOM (void)
{
  CoUninitialize ();
  initialized_com = false;
}
