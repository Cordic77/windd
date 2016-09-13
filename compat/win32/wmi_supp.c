#include "win32utf16.h"           /* switch to UTF-16 in this file! */

#include "win32\windows_api.h"

#include <config.h>

#include "windows_api.h"              // WIN32 API
#include "com_supp.h"                 // COM support
#include "wmi_supp.h"                 // WMI support


//
extern __declspec(thread) bool        // Thread-local storage: InitializeWMI() may
  InitializeWMId = false;             // be called for each thread!

//
static IWbemLocator
 *acquired_locator; //= NULL;         // IWbemLocator object; DESTRUCT: Free_WMI(); Free_WbemLocator();
static IWbemServices
 *acquired_service; //= NULL;         // IWbemServices proxy; DESTRUCT: Free_WMI(); Free_WbemServices();
static IUnknown
 *acquired_service_unkn; //= NULL;    // IUnknown of IWbemServices; DESTRUCT: Free_WMI();
static IStream//LPSTREAM
 *marshalled_service; //= NULL;       // Marshalled pSvcAcquired; DESTRUCT: unnecessary.


//
IWbemServices
 *wbem_svc; //= NULL;                 // Last unmarshalled services pointer; DESTRUCT: missing.


//
static void Free_WbemLocator (void);
static void Free_WbemServices (void);


//
extern bool InitializeWMI (void)
{ HRESULT
    hres;

  if (InitializeWMId || not InitializeCOM ())
    return (InitializeWMId);

  // Registers security and sets the default security values for the process:
  // 2. COM SECURITY: "If you are using Windows 2000, you need to specify
  // the default authentication credentials for a user by using a
  // SOLE_AUTHENTICATION_LIST structure in the pAuthList parameter of
  // CoInitializeSecurity":
  hres =  CoInitializeSecurity ( // WinNT4, Windows 98, Windows 95 with DCOM
    NULL,                           // Defines the access permissions
    -1,                             // COM authentication
    NULL,                           // Authentication services
    NULL,                           // RES
    RPC_C_AUTHN_LEVEL_CONNECT,      // Authenticate on connect to server (=RPC_C_AUTHN_LEVEL_DEFAULT)
    RPC_C_IMP_LEVEL_IMPERSONATE,    // Default Impersonation
    NULL,                           // Authentication info
    EOAC_NONE,                      // Additional capabilities
    NULL                            // RES
  );

  if (FAILED (hres))
    ComErrReturn (NULL, hres, false);

  // Creates a connection through DCOM to a WMI namespace on the computer:
  // 3. OBTAIN INITIAL LOCATOR TO WMI:
  hres = CoCreateInstance ( // WinNT3.1, Windows 95
    CLSID_REF CLSID_WbemLocator,    // Class identifier
    NULL,                           // Not created as part of an aggregate
    CLSCTX_INPROC_SERVER,           // Only allowed value
    IID_REF IID_IWbemLocator,       // Reference to the interface identifier
    (LPVOID *)&acquired_locator     // Interface pointer
  );

  if (FAILED (hres))
    ComErrReturn (NULL, hres, false);

  { BSTR
      root_cimv2 = SysAllocString (L"root\\cimv2");

    if (root_cimv2 == NULL)
    {
      SetWinErr ();
      goto FreeLocator;
    }

    // 4. CONNECT TO THE root\cimv2 NAMESPACE through the IWbemLocator::ConnectServer
    // using the current user and obtain a pSvc pointer to make IWbemServices calls:
    hres = COM_OBJ(acquired_locator)->ConnectServer ( // WinNT4SP4, Windows 95
      COM_OBJ_THIS_(acquired_locator) // this,
      root_cimv2,                     // Object path of WMI namespace
      NULL,                           // User name (NULL=current user)
      NULL,                           // User password (NULL=current)
      NULL,                           // Locale (NULL indicates current)
      0,                              // Security flags
      NULL,                           // Authority (e.g. Kerberos)
      0,                              // Context object
      &acquired_service               // pointer to IWbemServices proxy
    );

    if (FAILED (hres))
    {
//    MessageBox (NULL, COMErrorMessage(hres), TEXT("Could not connect to root/cimv2 ..."), MB_ICONERROR);
      SetComErr (acquired_locator, hres);
      goto FreeBstr;
    }

    SysFreeString (root_cimv2);
    root_cimv2 = NULL;

    // 5. SET GECURITY LEVELS ON THE PROXY:
    hres = COM_OBJ(acquired_service)->QueryInterface (
             COM_OBJ_THIS_(acquired_service)
             IID_PPV_ARG (IUnknown, &acquired_service_unkn));

    if (FAILED (hres))
    {
      SetComErr (acquired_service, hres);
      goto FreeService;
    }

    hres = CoSetProxyBlanket ( // WinNT4, Windows 98, Windows 95 mit DCOM
//    acquired_service,
      acquired_service_unkn,        // Indicates the proxy to set
      RPC_C_AUTHN_WINNT,            // Authentication service to use
      RPC_C_AUTHZ_NONE,             // Authentication service to use
      NULL,                         // Server principal name
      RPC_C_AUTHN_LEVEL_CALL,       // Authentication level to use
      RPC_C_IMP_LEVEL_IMPERSONATE,  // Impersonation level to use
      NULL,                         // Client identity
      EOAC_NONE                     // Proxy capabilities
    );

    if (FAILED (hres))
    {
      SetComErr (acquired_service, hres);
      goto FreeService;
    }

    // `acquired_service' is not marshalled yet, but otherwise we're now good to go!
    wbem_svc = NULL;
    return ((InitializeWMId = true));

FreeService:
    Free_WbemServices ();
FreeBstr:
    if (root_cimv2)
      SysFreeString (root_cimv2);
FreeLocator:
    Free_WbemLocator ();
  }

  return (InitializeWMId);
}


// Marshals an interface pointer from one thread to another thread in the same
// process. Only needs to be called before spawning a new thread, but we call
// it at the begin of all WMI operations anyway:
extern void WmiMarshalWbemServicePtr (void)
{
  if (!COMAvailable ())
    goto OnError;

  { HRESULT hres = CoMarshalInterThreadInterfaceInStream (
                     IID_REF IID_IWbemServices,  // IID of the interface
//                   acquired_service,
                     acquired_service_unkn,      // interface to be marshaled
                     &marshalled_service);       // IStream: marshaled interface

    if (SUCCEEDED (hres))
      return;

//  MessageBox (NULL, COMErrorMessage(hres), TEXT("Could not marshal WbemServices pointer to stream ..."), MB_ICONERROR);
    SetComErr (NULL, hres);
  }

OnError:;
  marshalled_service = NULL;
}


// Unmarshals a buffer containing an interface pointer and releases the stream
// when an interface pointer has been marshaled from another thread to the
// calling thread. Only needs to be called in the newly created thread after
// COM has been initialized via COM_Init(), but we call call it at the begin
// of all WMI operations anyway:
extern void WmiUnmarshalWbemServicePtr (void)
{
  if (not COMAvailable() || marshalled_service==NULL)
    goto OnError;

  { HRESULT hres = CoGetInterfaceAndReleaseStream (
                     marshalled_service,
                     IID_PPV_ARG (IWbemServices, &wbem_svc));

    // Unmarshalling the service pointer only works once!
    marshalled_service = NULL;

    if (SUCCEEDED (hres))
      return;

//  MessageBox (NULL, COMErrorMessage(hres), TEXT("Could not unmarshal WbemServices class from stream ..."), MB_ICONERROR);
    SetComErr (NULL, hres);
  }

OnError:;
  wbem_svc = NULL;
}


extern bool wmi_avail (void)
{
  return (InitializeWMId && COMAvailable());
}


extern bool svc_valid (void)
{
  return (wbem_svc != NULL);
}


inline static void Free_WbemLocator (void)
{
  CoReleaseObject (acquired_locator);
}


inline static void Free_WbemServices (void)
{
  CoReleaseObject (acquired_service_unkn);
  CoReleaseObject (acquired_service);
}


extern void FreeWMI (void)
{
  Free_WbemServices ();
  Free_WbemLocator ();
}
