#ifndef WINDD_WMI_H_
#define WINDD_WMI_H_

#include <wbemcli.h>              /* IWbemLocator, IWbemServices */

/* */
extern IWbemServices *
  wbem_svc;                       /* Last unmarshalled services pointer; DESTRUCT: missing. */

/* */
extern bool
  InitializeWMI (void);
extern void
  WmiMarshalWbemServicePtr (void);
extern void
  WmiUnmarshalWbemServicePtr (void);
extern bool
  wmi_avail (void);
extern bool
  svc_valid (void);
extern void
  FreeWMI (void);

#endif
