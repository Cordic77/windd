#ifndef WINDD_WINAPI_H_
#define WINDD_WINAPI_H_

/* Windows headers: */
#define WIN32_LEAN_AND_MEAN       /* Exclude APIs such as Cryptography, DDE, RPC, Shell, and Windows Sockets */
#define NOSERVICE                 /* Additional NOservice definitions: NOMCX, NOIME, NOSOUND, NOCOMM, NOKANJI, NORPC, ... */
#define _WINSOCKAPI_              /* Prevent windows.h from including winsock.h */
#define _WIN32_DCOM               /* Include all DCOM (Distributed Component Object Model) centric definitions */
#include <windows.h>              /* ... pull in Windows headers! */

/* Error handling: */
#include <WinError.h>             /* Windows and COM error codes */
#include <LMErr.h>                /* NERR_BASE, MAX_NERR */

/* UNICODE-agnostic macros: */
#include <tchar.h>                /* _tcschr(), _tcscmp() */

/* UNC path support: */
#define MAX_UNCPATH               32767

/* The "\\.\" prefix will access the Win32 device namespace instead of the Win32 file namespace: */
#define UNC_PHYS_DEVICE           TEXT("\\\\.\\")
#define UNC_PHYS_LENGTH           4

/* Like \\.\, but disables all string parsing; the string is sent straight to the file system: */
#define UNC_DISPARSE_PREFIX       TEXT("\\\\?\\")  /* ... thus allows one to specify "extended-length path" */
#define UNC_DISPARSE_LENGTH       4                /* with a total path length of up to 32.767 characters.

/* "\\?\UNC\server\share", where "server" is the name of the computer and "share" is the name of the shared folder: */
#define UNC_NETSHARE_PREFIX       TEXT("\\\\?\\UNC\\")
#define UNC_NETSHARE_LENGTH       8

#endif
