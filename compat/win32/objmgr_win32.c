#include "win32utf16.h"           /* switch to UTF-16 in this file! */

#include <config.h>

#include "objmgr_win32.h"

#include <Winternl.h>             /* NTSTATUS, NtQuerySystemInformation() */


/* See ReactOS:lean-explorer/reactos/subsys/system/explorer/shell/shellfs.cpp */
LPCWSTR const ObjectTypesStr [ObjType_Count] = {
    L"Directory", L"SymbolicLink",
    L"Mutant", L"Section", L"Event", L"Semaphore",
    L"Timer", L"Key", L"EventPair", L"IoCompletion",
    L"Device", L"File", L"Controller", L"Profile",
    L"Type", L"Desktop", L"WindowStatiom", L"Driver",
    L"Token", L"Process", L"Thread", L"Adapter", L"Port",
  };


/* See "Enumerating DeviceObjects from user mode"
   https://randomsourcecode.wordpress.com/2015/03/14/enumerating-deviceobjects-from-user-mode/
   Also: "Creating a WinObj-like Tool"
   Part 1: http://blogs.microsoft.co.il/pavely/2014/02/05/creating-a-winobj-like-tool/
   Part 2: http://blogs.microsoft.co.il/pavely/2014/02/09/creating-an-object-manager-browser-part-2viewing-object-information/
*/

/* See `Winternl.h': */
#ifndef InitializeObjectAttributes
#define InitializeObjectAttributes( p, n, a, r, s ) { \
    (p)->Length = sizeof( OBJECT_ATTRIBUTES );          \
    (p)->RootDirectory = r;                             \
    (p)->Attributes = a;                                \
    (p)->ObjectName = n;                                \
    (p)->SecurityDescriptor = s;                        \
    (p)->SecurityQualityOfService = NULL;               \
    }
#endif

/* See `km\wdm.h': */
#ifndef DIRECTORY_QUERY
#define DIRECTORY_QUERY                 (0x0001)
#endif
#ifndef DIRECTORY_TRAVERSE
#define DIRECTORY_TRAVERSE              (0x0002)
#endif
#ifndef STATUS_NEED_MORE_ENTRIES
#define STATUS_NEED_MORE_ENTRIES        (0x0105)
#endif

FORCEINLINE VOID NTAPI RtlInitUnicodeString (PUNICODE_STRING DestinationString, PCWSTR SourceString)
{
  DestinationString->Buffer = (PWSTR)SourceString;
  DestinationString->MaximumLength =
    DestinationString->Length = (USHORT)(wcslen(SourceString) * sizeof(WCHAR));
}

FORCEINLINE VOID NTAPI RtlFreeUnicodeString (PUNICODE_STRING UnicodeString)
{
  UnicodeString->Buffer = NULL;
  UnicodeString->MaximumLength =
    UnicodeString->Length = 0;
}

/* No header file? */
typedef struct _OBJECT_DIRECTORY_INFORMATION {
  UNICODE_STRING Name;
  UNICODE_STRING TypeName;
} OBJECT_DIRECTORY_INFORMATION, *POBJECT_DIRECTORY_INFORMATION;

typedef NTSTATUS (WINAPI *PFN_NTOPENDIRECTORYOBJECT)(
  _Out_  PHANDLE DirectoryHandle,
  _In_   ACCESS_MASK DesiredAccess,
  _In_   POBJECT_ATTRIBUTES ObjectAttributes
  );

typedef NTSTATUS (WINAPI *PFN_NTQUERYDIRECTORYOBJECT)(
  _In_       HANDLE DirectoryHandle,
  _Out_opt_  PVOID Buffer,
  _In_       ULONG Length,
  _In_       BOOLEAN ReturnSingleEntry,
  _In_       BOOLEAN RestartScan,
  _Inout_    PULONG Context,
  _Out_opt_  PULONG ReturnLength
  );

typedef NTSTATUS (WINAPI *PFN_NTOPENSYMBOLICLINKOBJECT)(
  _Out_  PHANDLE LinkHandle,
  _In_   ACCESS_MASK DesiredAccess,
  _In_   POBJECT_ATTRIBUTES ObjectAttributes
  );

typedef NTSTATUS (WINAPI *PFN_NTQUERYSYMBOLICLINKOBJECT)(
  _In_       HANDLE LinkHandle,
  _Inout_    PUNICODE_STRING LinkTarget,
  _Out_opt_  PULONG ReturnedLength
  );


/* Windows Sysinternals "HOWTO: Enumerate handles"
   http://forum.sysinternals.com/howto-enumerate-handles_topic18892.html

*/
extern bool EnumObjectDirectory (LPCWSTR ObjectDirectoryPath, EnumObjectDirectoryCallbackFunc EnumObjectDirectoryCallbackPtr)
{ bool
    succ = false;
  static WCHAR
    target_path [MAX_PATH + 1];  /* UNICODE strings are backed by simple WCHAR arrays */
  HMODULE
    hNtdll;
  PFN_NTOPENDIRECTORYOBJECT
    NtOpenDirectoryObjectPtr;
  PFN_NTQUERYDIRECTORYOBJECT
    NtQueryDirectoryObjectPtr;
  PFN_NTOPENSYMBOLICLINKOBJECT
    NtOpenSymbolicLinkObjectPtr;
  PFN_NTQUERYSYMBOLICLINKOBJECT
    NtQuerySymbolicLinkObjectPtr;

  hNtdll = LoadLibrary (TEXT("ntdll.dll"));
  if (not hNtdll)
    goto LoadLibraryFailed;

  NtOpenDirectoryObjectPtr     = (PFN_NTOPENDIRECTORYOBJECT)GetProcAddress (hNtdll, "NtOpenDirectoryObject");
  NtQueryDirectoryObjectPtr    = (PFN_NTQUERYDIRECTORYOBJECT)GetProcAddress (hNtdll, "NtQueryDirectoryObject");
  NtOpenSymbolicLinkObjectPtr  = (PFN_NTOPENSYMBOLICLINKOBJECT)GetProcAddress (hNtdll, "NtOpenSymbolicLinkObject");
  NtQuerySymbolicLinkObjectPtr = (PFN_NTQUERYSYMBOLICLINKOBJECT)GetProcAddress (hNtdll, "NtQuerySymbolicLinkObject");
  if (not NtOpenDirectoryObjectPtr
   || not NtQueryDirectoryObjectPtr
   || not NtOpenSymbolicLinkObjectPtr
   || not NtQuerySymbolicLinkObjectPtr)
    goto GetProcFailed;

  /* Open the requested directory object: */
  { HANDLE
      dir_handle;
    OBJECT_ATTRIBUTES
      obj_attr;
    UNICODE_STRING
      dir_path;
    NTSTATUS
      status;

    RtlInitUnicodeString (&dir_path, ObjectDirectoryPath);
    InitializeObjectAttributes (&obj_attr, &dir_path, 0, NULL, NULL);
    status = (*NtOpenDirectoryObjectPtr) (&dir_handle, DIRECTORY_QUERY | DIRECTORY_TRAVERSE, &obj_attr);
    if (status < 0)
      goto OpenDirectoryFailed;

    /* Traverse object directory: */
    { OBJECT_DIRECTORY_INFORMATION *
        dir_info = NULL;
      size_t
        buffer_size = 10*sizeof(OBJECT_DIRECTORY_INFORMATION);

      dir_info = (OBJECT_DIRECTORY_INFORMATION *)LocalAlloc (LPTR, buffer_size);

      /* Make calls to `NtQueryDirectoryObject()' as long as we receive STATUS_NEED_MORE_ENTRIES: */
      { ULONG
          curr_index = 0,
          start_index, i;
        BOOLEAN
          first_entry = TRUE;
        NTSTATUS
          data_status;
        bool
          callback_cont = true;

        do {
          start_index = curr_index;
          data_status = (*NtQueryDirectoryObjectPtr) (dir_handle, (PVOID)dir_info, (ULONG)buffer_size,
                                                      FALSE, first_entry, &curr_index, NULL);
          if (status < 0)
            goto TraverseDirectoryFailed;
          first_entry = FALSE;

          for (i=0; callback_cont && i < curr_index-start_index; ++i)
            if (dir_info[i].TypeName.Length == 12*sizeof(WCHAR)
             && _wcsicmp (dir_info[i].TypeName.Buffer, ObjectTypesStr[ObjType_SymbolicLink]) == 0)
            { HANDLE
                entry_handle;
              NTSTATUS
                status;
              UNICODE_STRING
                link_path;
              OBJECT_ATTRIBUTES
                entry_attr;

              entry_handle = NULL;

              /* Open symbolic link object: */
              InitializeObjectAttributes (&entry_attr, &dir_info[i].Name, 0, dir_handle, NULL);
              status = (*NtOpenSymbolicLinkObjectPtr) (&entry_handle, GENERIC_READ, &entry_attr);
              if (status < 0)
                continue;

              /* Query symbolic link object: */
              target_path [0] = L'\0';
              RtlInitUnicodeString (&link_path, target_path);
              link_path.MaximumLength = sizeof(target_path)-sizeof(WCHAR);

              status = (*NtQuerySymbolicLinkObjectPtr) (entry_handle, &link_path, NULL);
              if (status < 0)
                goto FreeLinkUnicodeString;
              target_path [link_path.Length] = L'\0';

              callback_cont = (*EnumObjectDirectoryCallbackPtr) (
                dir_info[i].Name.Buffer, ObjType_SymbolicLink, target_path);

FreeLinkUnicodeString:
              RtlFreeUnicodeString (&link_path);
              if (entry_handle != NULL)
                CloseHandle (entry_handle);
            }
        } while (callback_cont && data_status == STATUS_NEED_MORE_ENTRIES);
      }
      succ = true;

TraverseDirectoryFailed:
      LocalFree (dir_info);
    }

    CloseHandle (dir_handle);
OpenDirectoryFailed:
    RtlFreeUnicodeString (&dir_path);
  }

GetProcFailed:
  FreeLibrary (hNtdll);
LoadLibraryFailed:
  return (succ);
}


extern size_t NormalizeNTDevicePath (wchar_t nt_device_path [])
{
  if (nt_device_path == NULL)
    return (0);

  { size_t
      length = wcslen (nt_device_path);

    if (nt_device_path [length-1] == L'\\')
      nt_device_path [--length] = L'\0';

    /* Paths with a \\?\ prefix specify MS-DOS devices which are symbolic
       links that reside in the \\?\ Object Manager namespace, which usually
       point out to a real device located in the \Device namespace. What makes
       MS-DOS devices special is that they - as opposed to NT device namespace
       paths - can be directly passed to CreateFile(): */
    if (PathPrefixW (nt_device_path) > 0)
      return (length);

    /* Otherwise, the prefix "\\GLOBALROOT" needs to be prepended, before
       the device path can be passed to CreateFile(): */
    { wchar_t const
        global_root [12+1] = L"\\GLOBALROOT\\";

      if (wcsistr (nt_device_path, global_root) != NULL)
        return (length);
      else
      {
        /* \\.\ or \\?\ */
        int path_prefix = PathPrefixW (nt_device_path);
        if (path_prefix > 0)
          --path_prefix;

        /* \Device\Cdrom0  =>  \\?\GLOBALROOT\Device\Cdrom0 */
        memmove (nt_device_path+UNC_DISPARSE_LENGTH+_countof(global_root)-2-1/*NTS*/,
                 nt_device_path+path_prefix, (length+1-path_prefix)*sizeof(wchar_t));
        wcscpy (nt_device_path, UNC_DISPARSE_PREFIX);
        memcpy (nt_device_path+UNC_DISPARSE_LENGTH,
                global_root+1, sizeof(global_root)-2*sizeof(wchar_t));

        return (length+UNC_DISPARSE_LENGTH+_countof(global_root));
      }
    }
  }
}


extern size_t IsDeviceObjectA (char const file [], char const device_prefix [])
{
  if (file != NULL && _memicmp (file, "\\Device\\", 8) == 0)
    if (_memicmp (file+8, device_prefix, strlen(device_prefix)) == 0)
    { char const *
        chr = file+8+strlen(device_prefix);

      do {
        if (not isdigit (*chr))
          return (false);
        ++chr;
      } while (chr[0] != '\0');

      return (chr-file);
    }

  return (0);
}


extern size_t IsDeviceObjectW (wchar_t const file [], wchar_t const device_prefix [])
{
  if (file != NULL && _memicmp (file, L"\\Device\\", 8*sizeof(wchar_t)) == 0)
    if (_memicmp (file+8, device_prefix, wcslen(device_prefix)*sizeof(wchar_t)) == 0)
    { wchar_t const *
        chr = file+8+wcslen(device_prefix);

      do {
        if (not iswdigit (*chr))
          return (false);
        ++chr;
      } while (chr[0] != L'\0');

      return (chr-file);
    }

  return (0);
}


extern bool EqualDeviceObjectA (char const device_path_1 [], char const device_path_2 [], char const device_prefix [])
{
  if (device_path_1 != NULL && device_path_2 != NULL)
  { size_t
      length1, length2;

    if ((length1=IsDeviceObjectA (device_path_1, device_prefix)) == 0
     || (length2=IsDeviceObjectA (device_path_2, device_prefix)) == 0)
      return (false);

    if (length1 != length2)
      return (false);

    return (_memicmp (device_path_1, device_path_2, length1) == 0);
  }

  return (false);
}


extern bool EqualDeviceObjectW (wchar_t const device_path_1 [], wchar_t const device_path_2 [], wchar_t const device_prefix [])
{
  if (device_path_1 != NULL && device_path_2 != NULL)
  { size_t
      length1, length2;

    if ((length1=IsDeviceObjectW (device_path_1, device_prefix)) == 0
     || (length2=IsDeviceObjectW (device_path_2, device_prefix)) == 0)
      return (false);

    if (length1 != length2)
      return (false);

    return (_memicmp (device_path_1, device_path_2, length1*sizeof(wchar_t)) == 0);
  }

  return (false);
}


/* "\Device\CdRom0" */
extern size_t IsCdromDriveA (char const file [])
{
  return (IsDeviceObjectA (file, "CdRom"));
}


extern size_t IsCdromDriveW (wchar_t const file [])
{
  return (IsDeviceObjectW (file, L"CdRom"));
}


extern bool EqualCdromDriveA (char const device_path_1 [], char const device_path_2 [])
{
  return (EqualDeviceObjectA (device_path_1, device_path_2, "CdRom"));
}


extern bool EqualCdromDriveW (wchar_t const device_path_1 [], wchar_t const device_path_2 [])
{
  return (EqualDeviceObjectW (device_path_1, device_path_2, L"CdRom"));
}


/* "\\.\PHYSICALDRIVE" */
extern size_t IsPhysicalDriveA (char const file [])
{
  int path_prefix = PathPrefixA (file);

  if (file != NULL && _memicmp (file+path_prefix-(path_prefix>0), "\\PHYSICALDRIVE", 14) == 0)
  { char const *
      chr = file + path_prefix+13;

    do {
      if (not isdigit (*chr))
        return (false);
      ++chr;
    } while (chr[0] != '\0');

    return (chr-file-path_prefix);
  }

  return (0);
}


extern size_t IsPhysicalDriveW (wchar_t const file [])
{
  int path_prefix = PathPrefixW (file);

  if (file != NULL && _memicmp (file+path_prefix-(path_prefix>0), L"\\PHYSICALDRIVE", 14*sizeof(wchar_t)) == 0)
  { wchar_t const *
      chr = file + path_prefix+13;

    do {
      if (not iswdigit (*chr))
        return (false);
      ++chr;
    } while (chr[0] != L'\0');

    return (chr-file-path_prefix);
  }

  return (0);
}


extern bool EqualPhysicalDriveA (char const device_path_1 [], char const device_path_2 [])
{
  if (device_path_1 != NULL && device_path_2 != NULL)
  { size_t
      length1, length2;

    if ((length1=IsPhysicalDriveA (device_path_1)) == 0
     || (length2=IsPhysicalDriveA (device_path_2)) == 0)
      return (false);

    if (length1 != length2)
      return (false);

    device_path_1 += PathPrefixA (device_path_1);
    device_path_2 += PathPrefixA (device_path_2);

    return (_memicmp (device_path_1, device_path_2, length1) == 0);
  }

  return (false);
}


extern bool EqualPhysicalDriveW (wchar_t const device_path_1 [], wchar_t const device_path_2 [])
{
  if (device_path_1 != NULL && device_path_2 != NULL)
  { size_t
      length1, length2;

    if ((length1=IsPhysicalDriveW (device_path_1)) == 0
     || (length2=IsPhysicalDriveW (device_path_2)) == 0)
      return (false);

    if (length1 != length2)
      return (false);

    device_path_1 += PathPrefixW (device_path_1);
    device_path_2 += PathPrefixW (device_path_2);

    return (_memicmp (device_path_1, device_path_2, length1*sizeof(wchar_t)) == 0);
  }

  return (false);
}


/* "\Device\HarddiskVolume1" */
extern size_t IsHarddiskVolumeA (char const file [])
{
  return (IsDeviceObjectA (file, "HarddiskVolume"));
}


extern size_t IsHarddiskVolumeW (wchar_t const file [])
{
  return (IsDeviceObjectW (file, L"HarddiskVolume"));
}


extern bool EqualHarddiskVolumeA (char const device_path_1 [], char const device_path_2 [])
{
  return (EqualDeviceObjectA (device_path_1, device_path_2, "HarddiskVolume"));
}


extern bool EqualHarddiskVolumeW (wchar_t const device_path_1 [], wchar_t const device_path_2 [])
{
  return (EqualDeviceObjectW (device_path_1, device_path_2, L"HarddiskVolume"));
}


/* "\\?\Volume{916307dc-0000-0000-0000-600000000000}\" */
extern size_t IsLogicalVolumeGuidA (char const file [])
{
  int path_prefix = PathPrefixA (file);

  if (file != NULL && _memicmp (file+path_prefix-(path_prefix>0), "\\Volume{", 8) == 0)
  { char const *
      chr = file + path_prefix+8;

    do {
      if (not isdigit (*chr) && not isalpha (*chr) && *chr != '-')
        return (false);
      ++chr;
    } while (*chr != '\0' && *chr != '}');

    if (*chr++ == '}')
    {
      if (*chr == '\\')
      {
        ++chr;
        ++file;  /* Don't include '\' in comparison! */
      }
      if (*chr == '\0')
        return (chr-file-path_prefix);
    }
  }

  return (false);
}


extern size_t IsLogicalVolumeGuidW (wchar_t const file [])
{
  int path_prefix = PathPrefixW (file);

  if (file != NULL && _memicmp (file+path_prefix-(path_prefix>0), L"\\Volume{", 8*sizeof(wchar_t)) == 0)
  { wchar_t const *
      chr = file + path_prefix+8;

    do {
      if (not iswdigit (*chr) && not iswalpha (*chr) && *chr != L'-')
        return (false);
      ++chr;
    } while (*chr != L'\0' && *chr != L'}');

    if (*chr++ == L'}')
    {
      if (*chr == L'\\')
      {
        ++chr;
        ++file;  /* Don't include '\' in comparison! */
      }
      if (*chr == L'\0')
        return (chr-file-path_prefix);
    }
  }

  return (false);
}


extern bool EqualLogicalVolumeGuidA (char const device_path_1 [], char const device_path_2 [])
{
  if (device_path_1 != NULL && device_path_2 != NULL)
  { size_t
      length1, length2;

    if ((length1=IsLogicalVolumeGuidA (device_path_1)) == 0
     || (length2=IsLogicalVolumeGuidA (device_path_2)) == 0)
      return (false);

    if (length1 != length2)
      return (false);

    device_path_1 += PathPrefixA (device_path_1);
    device_path_2 += PathPrefixA (device_path_2);

    return (_memicmp (device_path_1, device_path_2, length1) == 0);
  }

  return (false);
}


extern bool EqualLogicalVolumeGuidW (wchar_t const device_path_1 [], wchar_t const device_path_2 [])
{
  if (device_path_1 != NULL && device_path_2 != NULL)
  { size_t
      length1, length2;

    if ((length1=IsLogicalVolumeGuidW (device_path_1)) == 0
     || (length2=IsLogicalVolumeGuidW (device_path_2)) == 0)
      return (false);

    if (length1 != length2)
      return (false);

    device_path_1 += PathPrefixW (device_path_1);
    device_path_2 += PathPrefixW (device_path_2);

    return (_memicmp (device_path_1, device_path_2, length1*sizeof(wchar_t)) == 0);
  }

  return (false);
}
