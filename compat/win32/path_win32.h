#ifndef WINDD_PATH_H_
#define WINDD_PATH_H_

/* */
enum EDeviceFile
{
  EMULATED_WDX  = -2,
  NT_DEVICE     = -1,
  DEV_NONE      =  0,

/* MS-DOS devices: */
  /* read only */
  MSDOS_CLOCK   =  1,
  DEV_RO        = MSDOS_CLOCK,

  /* read/write */
  MSDOS_COM     =  2,
  MSDOS_LPT     =  3,
  MSDOS_CON     =  4,
  MSDOS_PRN     =  5,
  MSDOS_AUX     =  6,
  DEV_RW        = MSDOS_COM,

  /* write only */
  MSDOS_NUL     =  7,
  DEV_WO        = MSDOS_NUL,
  DEV_WINNATIVE = MSDOS_NUL,  /* All preceding devices are supported natively on WIN32! */

/* Linux devices: */
  /* read only */
  DEV_ZERO      =  8,         /*/dev/zero*/
  DEV_LINNATIVE = DEV_ZERO,
  DEV_RANDOM    =  9,         /*/dev/random*/
  DEV_URANDOM   = 10,         /*/dev/urandom*/

  /* write only */
  DEV_NULL      = MSDOS_NUL,

  DEV_INVALID   = 11
};

enum EVolumePath
{
  VOL_DET_FAILED   = -1,
  VOL_DET_OTHER    =  0,
  VOL_DRIVE_LETTER =  1,
  VOL_MOUNT_POINT  =  2
};

/* Representations of paths by operating system and shell:
   https://en.wikipedia.org/wiki/Path_(computing)#Representations_of_paths_by_operating_system_and_shell

      relative to %CD%                 MAX_PATH      readme.txt
   or [drive_letter]:                  MAX_PATH      C:readme.txt
   or \                                MAX_PATH      \Users\%USERNAME%\Documents\readme.txt
   or [drive_letter]:\                 MAX_PATH      C:\Users\%USERNAME%\Documents\readme.txt
   or \\?\[drive_spec]:\               UNC:32K       \\?\C:\Users\%USERNAME%\Documents\readme.txt

      \\?\[drive_spec]:                LOG-VOLUME    \\?\C:    NOTE: must not end in a trailing '\\'!
   or \\?\{VOLUME_GUID}                LOG-VOLUME    \\?\Volume{4c1b02c1-d990-11dc-99ae-806e6f6e6963}
   or \\.\[physical_device]\           PHYS-DISK     \\.\PhysicalDrive0

   or \\[server]\[sharename]\          NET-SHARE     \\%COMPUTERNAME%\C$\Windows
   or \\?\[server]\[sharename]\        NET-SHARE     \\?\%COMPUTERNAME%\C$\Windows        (UNC)
   or \\?\UNC\[server]\[sharename]\    NET-SHARE     \\?\UNC\%COMPUTERNAME%\C$\Windows    (Long UNC)
*/

/* Basic PATH functions: */
extern int
  NetSharePrefixA (char const file []);
extern int
  NetSharePrefixW (wchar_t const file []);

#ifdef UNICODE
#define NetSharePrefix NetSharePrefixW
#else
#define NetSharePrefix NetSharePrefixA
#endif

extern int
  UNCPrefixA (char const file []);
extern int
  UNCPrefixW (wchar_t const file []);

#ifdef UNICODE
#define UNCPrefix UNCPrefixW
#else
#define UNCPrefix UNCPrefixA
#endif

extern int
  LongUNCPrefixA (char const file []);
extern int
  LongUNCPrefixW (wchar_t const file []);

#ifdef UNICODE
#define LongUNCPrefix LongUNCPrefixW
#else
#define LongUNCPrefix LongUNCPrefixA
#endif

extern int
  PathPrefixA (char const file []);
extern int
  PathPrefixW (wchar_t const file []);

#ifdef UNICODE
#define PathPrefix PathPrefixW
#else
#define PathPrefix PathPrefixA
#endif

extern void
  RemoveTrailingBackslashA (char path []);
extern void
  RemoveTrailingBackslashW (wchar_t path []);

#ifdef UNICODE
#define RemoveTrailingBackslash RemoveTrailingBackslashW
#else
#define RemoveTrailingBackslash RemoveTrailingBackslashA
#endif

extern void
  AddTrailingBackslashA (char path []);
extern void
  AddTrailingBackslashW (wchar_t path []);

#ifdef UNICODE
#define AddTrailingBackslash AddTrailingBackslashW
#else
#define AddTrailingBackslash AddTrailingBackslashA
#endif

extern bool
  PathEqualsA (char const path1 [], char const path2 []);
extern bool
  PathEqualsW (wchar_t const path1 [], wchar_t const path2 []);

#ifdef UNICODE
#define PathEquals PathEqualsW
#else
#define PathEquals PathEqualsA
#endif

/* PATH normalization and classification: */
bool
  NTDeviceNameA (char const file []);
bool
  NTDeviceNameW (wchar_t const file []);

#ifdef UNICODE
#define NTDeviceName NTDeviceNameW
#else
#define NTDeviceName NTDeviceNameA
#endif

wchar_t const *
  MSDOSDeviceName (char const file [], enum MSDOSDeviceName *msdos_device);
extern enum EDeviceFile
  LinuxDeviceName (char const *file);
extern bool
  IsDeviceFile (char const file[], bool *nt_device_name);

extern int
  IsDirectoryPath (TCHAR const path []);
extern enum EVolumePath
  IsVolumePath (TCHAR const fullpath [], TCHAR volpath []);
extern bool
  GetVolumePath (TCHAR const fullpath [], TCHAR volpath [], size_t buflen);
extern bool
  IsNetworkPath (TCHAR const path []);

#endif
