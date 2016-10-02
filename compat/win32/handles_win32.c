#include "win32utf16.h"           /* switch to UTF-16 in this file! */

#include <config.h>

#pragma warning (push)
#pragma warning (disable : 4013)  /* warning C4013: 'getc_unlocked' undefined; assuming extern returning int */
#include <strsafe.h>              /* StringCchCopyNW() */
#pragma warning (pop)

#include "win32\handles_win32.h"

#include <winternl.h>             /* NTSTATUS, NtQuerySystemInformation() */


#define GET_FINAL_PATHNAME_BY_HANDLE

/* NtQuerySystemInformation (SystemHandleInformation):
   https://msdn.microsoft.com/en-us/library/windows/hardware/ff567052(v=vs.85).aspx
*/
static enum _SYSTEM_INFORMATION_CLASS
  SystemHandleInformation = (enum _SYSTEM_INFORMATION_CLASS)16;  /* Undocumented enum value */

#ifndef STATUS_INFO_LENGTH_MISMATCH
#define STATUS_INFO_LENGTH_MISMATCH  ((NTSTATUS)0xc0000004L)
#endif

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO
{
  ULONG ProcessId;
  UCHAR ObjectTypeNumber;
  UCHAR Flags;
  USHORT Handle;
  PVOID Object;
  ACCESS_MASK GrantedAccess;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO, *PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

typedef struct _SYSTEM_HANDLE_INFORMATION
{
  ULONG NumberOfHandles;
  SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[1];
} SYSTEM_HANDLE_INFORMATION, *PSYSTEM_HANDLE_INFORMATION;

typedef NTSTATUS (WINAPI * PFN_NTQUERYSYSTEMINFORMATION) (
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
  );


#ifndef GET_FINAL_PATHNAME_BY_HANDLE
/* NtQueryInformationFile (FileNameInformation):
   https://msdn.microsoft.com/en-us/library/windows/hardware/ff567052(v=vs.85).aspx
*/
static enum _FILE_INFORMATION_CLASS
  FileNameInformation = (enum _FILE_INFORMATION_CLASS)9;  /* Undocumented enum value */

typedef struct _FILE_NAME_INFORMATION {
  ULONG FileNameLength;
  WCHAR FileName[1];
} FILE_NAME_INFORMATION, *PFILE_NAME_INFORMATION;

typedef NTSTATUS (WINAPI * PFN_NTQUERYINFORMATIONFILE) (
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FileInformation,
    IN ULONG Length,
    IN FILE_INFORMATION_CLASS FileInformationClass
);
#endif


/* Windows Sysinternals "HOWTO: Enumerate handles"
   http://forum.sysinternals.com/howto-enumerate-handles_topic18892.html
*/
extern bool EnumFileHandles (EnumFileHandlesCallbackFunc EnumFileHandlesCallbackPtr, size_t *handle_count)
{ bool
    succ = false;
  static WCHAR
    handle_filename [MAX_UNCPATH + 1];
  HMODULE
    hNtdll;
  PFN_NTQUERYSYSTEMINFORMATION
    NtQuerySystemInformationPtr;
  #ifndef GET_FINAL_PATHNAME_BY_HANDLE
  PFN_NTQUERYINFORMATIONFILE
    NtQueryInformationFilePtr;
  #endif

  if (handle_count != NULL)
   *handle_count = 0;

  hNtdll = LoadLibrary (TEXT("ntdll.dll"));
  if (not hNtdll)
    goto LoadLibraryFailed;

  NtQuerySystemInformationPtr = (PFN_NTQUERYSYSTEMINFORMATION)GetProcAddress (hNtdll, "NtQuerySystemInformation");
  if (NtQuerySystemInformationPtr == NULL)
    goto GetProcFailed;

  #ifndef GET_FINAL_PATHNAME_BY_HANDLE
  NtQueryInformationFilePtr = (PFN_NTQUERYINFORMATIONFILE)GetProcAddress (hNtdll, "NtQueryInformationFile");
  if (NtQueryInformationFilePtr == NULL)
    goto GetProcFailed;
  #endif

  { NTSTATUS
      status;
    ULONG
      buffer_size = 0x10000;
    size_t
      i;
    /* handle information: */
    SYSTEM_HANDLE_INFORMATION *
      handle_info;
    ULONG
      handle_info_size;

    handle_info = (SYSTEM_HANDLE_INFORMATION *)MemAlloc (buffer_size, 1);

    while ((status=NtQuerySystemInformationPtr (SystemHandleInformation, handle_info, buffer_size, &handle_info_size)) == STATUS_INFO_LENGTH_MISMATCH)
    {
      buffer_size <<= 1;
      handle_info = (SYSTEM_HANDLE_INFORMATION *)MemRealloc (handle_info, buffer_size, 1);
    }

    if (not NT_SUCCESS(status))
      goto QuerySysInfoFailed;

    { HANDLE
        process_handle = NULL;
      HANDLE
        dup_handle = NULL;
      SYSTEM_HANDLE_TABLE_ENTRY_INFO *
        handle;
      int
        HANDLE_TYPE_FILE = -1;  /* Undocumented ObjectTypeNumber */
      bool
        callback_cont = true;

      for (i = 0; callback_cont && i < (size_t)handle_info->NumberOfHandles; ++i)
      {
        handle = handle_info->Handles + i;

        /*_ftprintf (stdout, TEXT("Handle ObjectTypeNumber %") TEXT(PRId32) TEXT("\n"), (int)handle->ObjectTypeNumber);*/

        if (HANDLE_TYPE_FILE < 0 || handle->ObjectTypeNumber == HANDLE_TYPE_FILE)
        {
          if (handle_count != NULL)
           *handle_count += 1;

          /* With the following access mask GetFinalPathNameByHandle() could hang: */
          if ((handle->GrantedAccess & 0x00120189) == 0x00120189)
            continue;

          if (EnumFileHandlesCallbackPtr != NULL)
          {
            /* Open process handle: */
            if ((process_handle=OpenProcess (PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, handle->ProcessId)) == NULL)
            {
              /* We don't know the path to the file described by handle->Handle! */
              /*
              callback_cont = EnumFileHandlesCallbackPtr (
                                  handle->ProcessId, (HANDLE)handle->Handle,
                                  handle->ObjectTypeNumber, L"");
              */
              continue;
            }

            /* Duplicate the handle so we can query it: */
            if (not DuplicateHandle (process_handle, (HANDLE)handle->Handle,
                                     GetCurrentProcess(), &dup_handle,
                                     MAXIMUM_ALLOWED, FALSE, 0))
            {
              /* We don't know the path to the file described by handle->Handle! */
              /*
              callback_cont = EnumFileHandlesCallbackPtr (
                                  handle->ProcessId, (HANDLE)handle->Handle,
                                  handle->ObjectTypeNumber, L"");
              */
              continue;
            }

            /* Retrieve full path: */
            #ifdef GET_FINAL_PATHNAME_BY_HANDLE
            if (GetFinalPathNameByHandle (dup_handle, handle_filename, _countof(handle_filename)-1, FILE_NAME_NORMALIZED) > 0)
            {
              callback_cont = EnumFileHandlesCallbackPtr (
                                  handle->ProcessId, (HANDLE)handle->Handle,
                                  handle->ObjectTypeNumber, handle_filename);
              HANDLE_TYPE_FILE = handle->ObjectTypeNumber;
            }
            #else
            /*PROBLEM: NtQueryInformationFile() does NOT retrieve drive letter (e.g. "\Users\Administrator")*/
            else
            { FILE_NAME_INFORMATION *
                name_info;
              IO_STATUS_BLOCK
                io_status;

              name_info = (FILE_NAME_INFORMATION *)MemAlloc (sizeof(handle_filename), 1);

              status = NtQueryInformationFilePtr (dup_handle, &io_status, name_info,
                                                  sizeof(handle_filename), FileNameInformation);
              if (status == STATUS_SUCCESS)
              {
                StringCchCopyNW (handle_filename, _countof(handle_filename),
                                 name_info->FileName, name_info->FileNameLength/sizeof(WCHAR));
                callback_cont = EnumFileHandlesCallbackPtr (
                                    handle->ProcessId, (HANDLE)handle->Handle,
                                    handle->ObjectTypeNumber, handle_filename);
              }
              /*
              else
              { LPTSTR pMessage = Win32ErrorMessage ((DWORD)status, true);

                if (pMessage != NULL)
                {
                  TrimRight (pMessage);
                  _ftprintf (stderr, TEXT("\n[%s] %s (NTSTATUS=0x%8") TEXT(PRIX32) TEXT(")\n"),
                                     TEXT(PROGRAM_NAME), pMessage, (uint32_t)status);
                  LocalFree ((HLOCAL)pMessage);
                }
              }
              */

              Free (name_info);
            }
            #endif

            CloseHandle (dup_handle);
            dup_handle = NULL;

            CloseHandle (process_handle);
            process_handle = NULL;
          }
        }
      }
    }

    succ = true;

QuerySysInfoFailed:
    Free (handle_info);
  }

GetProcFailed:
  FreeLibrary (hNtdll);
LoadLibraryFailed:
  return (succ);
}
