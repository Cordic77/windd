#include "win32utf16.h"           /* switch to UTF-16 in this file! */

#include <config.h>

#include "assure.h"               /* assure() */

// Additional _WIN32 headers:
#include <shlwapi.h>              /* PathIsUNC(), PathIsNetworkPath() */


//===  _WIN32 Long Path functions  ===

// Naming Files, Paths, and Namespaces
// https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
//
// A UNC name of any format, which always start with two backslash characters ("\\")
//
extern int NetSharePrefixA (char const file [])
{
  if (file != NULL && file[0]=='\\' && file[1]=='\\')
    return (2);

  return (0);
}

extern int NetSharePrefixW (wchar_t const file [])
{
  if (file != NULL && file[0]==L'\\' && file[1]==L'\\')
    return (2);

  return (0);
}


extern inline int UNCPrefixA (char const file [])
{
  if (file != NULL && file[0]=='\\' && file[1]=='\\' // && (file[2]=='?' || file[2]=='.')
   && file[3]=='\\')
    return (4);

  return (0);
}

extern inline int UNCPrefixW (wchar_t const file [])
{
  if (file != NULL && file[0]==L'\\' && file[1]==L'\\' // && (file[2]==L'?' || file[2]==L'.')
   && file[3]==L'\\')
    return (4);

  return (0);
}


extern int LongUNCPrefixA (char const file [])
{
  if (UNCPrefixA (file)
   && file[4]=='U' && file[5]=='N' && file[6]=='C' && file[7]=='\\')
    return (8);

  return (0);
}

extern int LongUNCPrefixW (wchar_t const file [])
{
  if (UNCPrefixW (file)
   && file[4]==L'U' && file[5]==L'N' && file[6]==L'C' && file[7]==L'\\')
    return (8);

  return (0);
}


extern int PathPrefixA (char const file [])
{ int prefix;

  if (not (prefix=LongUNCPrefixA (file)))
    if (not (prefix=UNCPrefixA (file)))
      prefix = NetSharePrefixA (file);

  return (prefix);
}

extern int PathPrefixW (wchar_t const file [])
{ int prefix;

  if (not (prefix=LongUNCPrefixW (file)))
    if (not (prefix=UNCPrefixW (file)))
      prefix = NetSharePrefixW (file);

  return (prefix);
}


extern void RemoveTrailingBackslashA (char path [])
{ size_t
    length;

  if (path == NULL)
    return;

  length = strlen (path);
  if (length > 0 && path [length-1] == '\\')
    path [length-1] = '\0';
}


extern void RemoveTrailingBackslashW (wchar_t path [])
{ size_t
    length;

  if (path == NULL)
    return;

  length = wcslen (path);
  if (length > 0 && path [length-1] == L'\\')
    path [length-1] = L'\0';
}


extern void AddTrailingBackslashA (char path [])
{ size_t
    length;

  if (path == NULL)
    return;

  length = strlen (path);
  if (length > 0 && path [length-1] != '\\')
  {
    path [length]   = '\\';
    path [length+1] = '\0';
  }
}


extern void AddTrailingBackslashW (wchar_t path [])
{ size_t
    length;

  if (path == NULL)
    return;

  length = wcslen (path);
  if (length > 0 && path [length-1] != L'\\')
  {
    path [length]   = L'\\';
    path [length+1] = L'\0';
  }
}


extern bool PathEqualsA (char const path1 [], char const path2 [])
{ size_t
    length1, length2;
  int
    comp;

  path1 += PathPrefixA (path1);
  length1 = strlen (path1);
  if (path1[length1-1] == '\\')
    --length1;

  path2 += PathPrefixA (path2);
  length2 = strlen (path2);
  if (path2[length2-1] == '\\')
    --length2;

  if (length1 != length2)
    return (false);

  comp = CompareStringA (LOCALE_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                         path1, (int)length1, path2, (int)length2);
  assert (comp != 0);  /* The function returns 0 if it does not succeed. */

  /* To maintain the C runtime convention of comparing strings, the
     value 2 can be subtracted from a nonzero return value. Then, the
     meaning of <0, ==0, and >0 is consistent with the C runtime: */
  comp -= 2;
  return (comp == 0);
}


extern bool PathEqualsW (wchar_t const path1 [], wchar_t const path2 [])
{ size_t
    length1, length2;
  int
    comp;

  path1 += PathPrefixW (path1);
  length1 = wcslen (path1);
  if (path1[length1-1] == L'\\')
    --length1;

  path2 += PathPrefixW (path2);
  length2 = wcslen (path2);
  if (path2[length2-1] == L'\\')
    --length2;

  if (length1 != length2)
    return (false);

  comp = CompareStringW (LOCALE_USER_DEFAULT, LINGUISTIC_IGNORECASE,
                         path1, (int)length1, path2, (int)length2);
  assert (comp != 0);  /* The function returns 0 if it does not succeed. */

  /* To maintain the C runtime convention of comparing strings, the
     value 2 can be subtracted from a nonzero return value. Then, the
     meaning of <0, ==0, and >0 is consistent with the C runtime: */
  comp -= 2;
  return (comp == 0);
}


bool NTDeviceNameA (char const file [])
{
  int path_prefix = PathPrefixA (file);
  if (path_prefix > 0)
    --path_prefix;

  if (file != NULL)
  {
    /* Physical drive? */
    if (IsPhysicalDriveA (file))
      return (true);

    /* Logical volume? */
    if (IsLogicalVolumeGuidA (file))
      return (true);

    /* NT device namespace? */
    if (_memicmp (file+path_prefix, "\\Device\\", 8) == 0
     || _memicmp (file+path_prefix, "\\GLOBALROOT\\Device\\", 19) == 0)
      return (true);
  }

  return (false);
}


bool NTDeviceNameW (wchar_t const file [])
{
  int path_prefix = PathPrefixW (file);
  if (path_prefix > 0)
    --path_prefix;

  if (file != NULL)
  {
    /* Physical drive? */
    if (IsPhysicalDriveW (file))
      return (true);

    /* Logical volume? */
    if (IsLogicalVolumeGuidW (file))
      return (true);

    /* NT device namespace? */
    if (_memicmp (file+path_prefix, L"\\Device\\", 8*sizeof(wchar_t)) == 0
     || _memicmp (file+path_prefix, L"\\GLOBALROOT\\Device\\", 19*sizeof(wchar_t)) == 0)
    {
      return (true);
    }
  }

  return (false);
}


// https://support.microsoft.com/en-us/kb/74496#mt1
//
//    Name    Function
//    ----    --------
//    CON     Keyboard and display
//    PRN     System list device, usually a parallel port
//    AUX     Auxiliary device, usually a serial port
//    CLOCK$  System real-time clock (NT and older)
//    NUL     Bit-bucket device
//    A:-Z:   Drive letters
//    COM1    First serial communications port
//    LPT1    First parallel printer port
//    LPT2    Second parallel printer port
//    LPT3    Third parallel printer port
//    COM2    Second serial communications port
//    COM3    Third serial communications port
//    COM4    Fourth serial communications port
//
wchar_t const *MSDOSDeviceName (char const file [], enum EDeviceFile *device_file)
{ enum EDeviceFile
    det_device = DEV_NONE;
  static wchar_t
    dev_file [10 + 1];  /* "\\.\COM255" */
  char
    chr1, chr2;

  int path_prefix = PathPrefixA (file);
  dev_file [0] = L'\0';

  if (file != NULL)
  {
    /* read only */
    if (_stricmp (file+path_prefix, "CLOCK$") == 0)
    {
      chr1 = *(file+path_prefix+6);
      chr2 = *(file+path_prefix+7);

      if (chr1 == '\0' || (chr1 == ':' && chr2 == '\0'))
      {
        /*// Doesn't exist according to `WinObj': NT and older only */
        /*
        det_device = MSDOS_CLOCK;
        wcscpy (dev_file, L"\\\\.\\CLOCK$");
        */
      }
    }

    /* read/write */
    else
    if (_memicmp (file+path_prefix, "COM", 3) == 0
     || _memicmp (file+path_prefix, "LPT", 3) == 0)
    { size_t
        i = 3;

      while (isdigit (*(file+path_prefix+i)))
        ++i;
      chr1 = *(file+path_prefix+i);
      chr2 = *(file+path_prefix+i+1);

      if (chr1 == '\0' || (chr1 == ':' && chr2 == '\0'))
      { static portname [10 + 1];  /* "\\.\COM255" */

        if (_memicmp (file+path_prefix, "COM", 3) == 0)
        {
          _snwprintf (dev_file, sizeof(dev_file), L"\\\\.\\%S", file+path_prefix);
          det_device = MSDOS_COM;
        }
        else
        if (_memicmp (file+path_prefix, "LPT", 3) == 0)
        {
          _snwprintf (dev_file, sizeof(dev_file), L"\\\\.\\%S", file+path_prefix);
          det_device = MSDOS_LPT;
        }
      }
    }
    else
    {
      chr1 = *(file+path_prefix+3);
      chr2 = *(file+path_prefix+4);

      if (chr1 == '\0' || (chr1 == ':' && chr2 == '\0'))
      {
        if (_memicmp (file+path_prefix, "CON", 3) == 0)
        {
          det_device = MSDOS_CON;
          wcscpy (dev_file, L"\\\\.\\CON"); /* L"IN$" <or> L"OUT$" */
        }
        else
        if (_memicmp (file+path_prefix, "PRN", 3) == 0)
        {
          det_device = MSDOS_PRN;
          wcscpy (dev_file, L"\\\\.\\PRN");
        }
        else
        if (_memicmp (file+path_prefix, "AUX", 3) == 0)
        {
          det_device = MSDOS_AUX;
          wcscpy (dev_file, L"\\\\.\\AUX");
        }

        /* write only */
        else
        if (_memicmp (file+path_prefix, "NUL", 3) == 0)
        {
          det_device = MSDOS_NUL;
          wcscpy (dev_file, L"\\\\.\\NUL");
        }
      }
      else
      if (strcmp (file, "/dev/null") == 0)
      {
        det_device = MSDOS_NUL;
        wcscpy (dev_file, L"\\\\.\\NUL");
      }
    }
  }

  /* Valid MS-DOS device file? */
  if (device_file != NULL)
    *device_file = det_device;

  return (dev_file);
}


extern enum EDeviceFile LinuxDeviceName (char const file [])
{
  if (file != NULL)
  {
    /* Special case: forced `/dev/wdx' access? (E.g. "!/dev/wda1") */
    if (file [0] == '!')
      ++file;

    if (memcmp (file, "/dev/", 5) == 0)
    {
      /* Emulated /dev/wdx device file? */
      if (is_wdx_id (file, NULL, NULL))
        return (EMULATED_WDX);
      else
      /* Emulated Linux device file? */
      {
        if (strcmp (file, DEVFILE_NULL) == 0)
        /*return (MSDOS_NUL);*/
          return (DEV_NONE);  /* Handled in MSDOSDeviceName() (and converted to "\\\\.\\NUL") */
        if (strcmp (file, DEVFILE_ZERO) == 0)
          return (DEV_ZERO);
        else
        if (strcmp (file, DEVFILE_URANDOM) == 0)
          return (DEV_URANDOM);
        else
        if (strcmp (file, DEVFILE_RANDOM) == 0)
          return (DEV_RANDOM);
      }

      return (DEV_INVALID);
    }
  }

  return (DEV_NONE);
}


extern bool IsDeviceFile (char const file [], bool *nt_device_name)
{ enum EDeviceFile
    device_file;

  if (nt_device_name != NULL)
   *nt_device_name = false;

  if ((device_file=LinuxDeviceName (file)) != DEV_NONE)
  {
    if (device_file == DEV_INVALID /*|| device_file == EMULATED_WDX*/)
      return (false);

    return (true);
  }
  else
  {
    (void)MSDOSDeviceName (file, &device_file);

    if (device_file != DEV_NONE)
      return (true);
    else
    if (NTDeviceNameA (file))
    {
      if (nt_device_name != NULL)
       *nt_device_name = true;
      return (true);
    }
  }

  return (false);
}


extern int IsDirectoryPath (TCHAR const path [])
{
  if (path != NULL)
  { DWORD
      attr = GetFileAttributes (path);

    if (GetLastError() != ERROR_FILE_NOT_FOUND)
    {
      if (attr == INVALID_FILE_ATTRIBUTES)
        WinErrReturn (-1);
      else
      if ((attr & FILE_ATTRIBUTE_DIRECTORY) == FILE_ATTRIBUTE_DIRECTORY)
        return (1);
    }
  }

  return (0);
}


extern enum EVolumePath IsVolumePath (TCHAR const fullpath [], TCHAR volpath [])
{
  if (fullpath == NULL || volpath == NULL)
    return (VOL_DET_FAILED);

  RemoveTrailingBackslash (volpath);

  /* Volume specified via a drive letter? */
  if (_tcscmp (fullpath, volpath) != 0)
    return (VOL_DET_OTHER);

  { TCHAR const *
      colon;

    /* Extract and search the specified drive letter: */
    if (IsDriveLetter (volpath, true, &colon, NULL))
    {
      if (select_drive_by_letter (colon-1))
        return (VOL_DRIVE_LETTER);
    }
    else
    {
      /* Select the specified volume mount point: */
      if (select_drive_by_mountpoint (volpath))
        return (VOL_MOUNT_POINT);
    }
  }

  return (VOL_DET_OTHER);
}


extern bool GetVolumePath (TCHAR const fullpath [], TCHAR volpath [], size_t buflen)
{
  // Try to determine drive letter:
  TCHAR *colon;

  if ((colon=_tcschr (fullpath, L':')) != NULL)
  {
    if (colon == fullpath || not _IS_DRIVE_LETTER((char)*(colon-1))  // "[^A-Z]:"
     || colon > fullpath+1 && _IS_DRIVE_LETTER((char)*(colon-2)))    // "[A-Z]{2,}:"
      ErrnoReturn (EINVAL, false);
  }

  // Drive letter specified?
  if (colon != NULL)
  {
    _sntprintf (volpath, 7, TEXT("\\\\.\\%c:"), *(colon-1));
    return (true);
  }
  else
  {
    // Retrieve the volume mount point where the specified path is mounted:
    if (GetVolumePathName (fullpath, volpath, (DWORD)buflen))
      return (true);

     WinErrReturn (false);
  }
}


extern bool IsNetworkPath (TCHAR const path [])
{
  return (PathIsNetworkPath (path));
}
