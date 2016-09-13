#include "win32utf16.h"           /* switch to UTF-16 in this file! */

#include <config.h>

#include "assure.h"               /* assure() */

/* Windows Management Instrumentation: */
#include <wbemcli.h>              /* CLSID_WbemLocator, IID_IWbemLocator */
#include <wbemidl.h>

#include "..\wmi_supp.h"


struct VolumeInfo;

/* (1) Physical disks: */
struct PartInfo
{
  bool                is_windows_part;      /* Partition containing a Windows Filesystem? */
  bool                locked_dismounted;    /* Partition lock and/or dismounted? */

  /* QueryDosDevice() */
  TCHAR              *harddisk_partition;   /* "\Device\Harddisk0\Partition1" */
  TCHAR              *harddisk_volume;      /* "\Device\HarddiskVolume1" */
  struct VolumeInfo  *volume;               /* Pointer to corresponding logical volume */

  /* Win32_DiskPartition: */
  bool                valid;
  uint32_t            DiskNumber;           /* 0 */
  uint32_t            PartNumber;           /* 0 */
  TCHAR              *device_id;            /* "Disk #0, Partition #0" */

  #if 0  /* WMIQueryPartitions() is no longer used! */
  off_t               start_offset;         /* 6291456 [bytes] */
  off_t               part_size;            /* 85425389568 [bytes] */
  #endif

  /* IOCTL_DISK_GET_DRIVE_LAYOUT_EX */
  PARTITION_INFORMATION_EX PartitionEntry;  /* see `ntdddisk.h': returned by `DRIVE_LAYOUT_INFORMATION_EX' */
};

struct DiskInfo
{
  enum EDriveType     type;
  bool                contains_system;      /* disk the system boots from? */

  /* Detected drives: */
  TCHAR              *caption;              /* "Samsung SSD 850 EVO 1TB" / "HL-DT-ST DVDRAM GUC0N" */
  TCHAR              *firmware_rev;         /* "EMT02B6Q"                / "T.02" */
  WCHAR              *objmgr_path;          /* "\Device\Harddisk0" or L"\Device\CdRom0" */
  TCHAR              *device_path;          /* "\\.\PHYSICALDRIVE0"      / "\\.\N:" */

  /* Harddisks only: */
  uint32_t            DiskNumber;           /* 0 */
  off_t               size_bytes;           /* 1000202273280 [bytes] */
  TCHAR              *serial_number;        /* "S21DNXAGB02351F" */

  /* Partitions: */
  PARTITION_STYLE     PartitionStyle;       /* see `ntdddisk.h': returned by `DRIVE_LAYOUT_INFORMATION_EX' */
  uint32_t            part_count;           /* "6" */
  uint32_t            log_part;             /* "3" */
  struct PartInfo    *part;                 /* Array of `part_count' partition table entries */
};

/* (2) Logical volumes: */
struct ExtentsInfo
{
  struct PartInfo    *part;                 /* I.a. stores harddisk volume: "\Device\HarddiskVolume1" */
  DISK_EXTENT         info;                 /* A single Starting Offset and Extent Length */
};

struct VolumeInfo
{
  bool                valid;
  bool                is_system_drive;      /* Windows drive (i.e. %SystemDrive%)? */

  TCHAR              *volume_guid;          /* "\\?\Volume{916307dc-0000-0000-0000-600000000000}\" */
  TCHAR             **mount_point;          /* drive letters and mounted folders (may be NULL) */

  uint32_t            extent_count;         /* Usually only one, but Dynamic Disks may have multiple Volume Extents */
  struct ExtentsInfo *extent;               /* Array of `extent_count' volume extent entries */
};


/* */
static BSTR
  wql; /* = SysAllocString (L"WQL"); */
bool
  force_operations; /*= false;*/  /* Force operations, ignore any open handles? */
bool
  always_confirm; /* = false; */      /* Always ask back before doing anything critical */

/* Local storage: */
static bool
  sel_nt_devname; /* = false; */  /* Selection via `select_drive_by_devicename()'? (Irrespective if it was successful.) */

/* Physical disks: */
static struct DiskInfo
 *physical_disk; /*= NULL;*/
#define INVALID_DISK    (uint32_t)-1
static uint32_t
  sel_disk = INVALID_DISK;        /* Index of currently selected drive (or -1 if none) */
#define INVALID_PART    (uint32_t)-1
static uint32_t
  sel_part = INVALID_PART;        /* Index of currently selected partition (or -1 if none) */

/* Logical volumes: */
static struct VolumeInfo
 *logical_volume; /* = NULL; */
#define INVALID_VOLUME  (uint32_t)-1
static uint32_t
  sel_volume = INVALID_VOLUME;    /* Index of currently selected drive (or -1 if none) */
#define INVALID_EXTENT  (uint32_t)-1
static uint32_t
  sel_extent = INVALID_EXTENT;    /* Index of currently selected drive (or -1 if none) */
static TCHAR const *
  sel_mount_point; /*= NULL;*/    /* Drive letter of current drive (or NULL if none) */


extern bool dskmgmt_initialize (void)
{
  if ((wql=SysAllocString (L"WQL")) == NULL)
  {
    SetWinErr ();
    return (false);
  }

  return (true);
}


extern void dskmgmt_free (void)
{
  Free (physical_disk);
}


/* Useless, as `Win32_DiskDriveToDiskPartition' only returns primary
   parititions, but no logical ones. */
#if 0
/* Get-WmiObject -Query "ASSOCIATORS OF {Win32_DiskDrive.DeviceID='\\.\PHYSICALDRIVE0'} WHERE AssocClass = Win32_DiskDriveToDiskPartition" */
static bool WMIQueryPartitions (struct DiskInfo const *disk)
{ wchar_t const *
    WMI_PARTITION_PROP[] = {        /* WMI: Physical disk partitions. */
      L"DiskIndex",                 /* 0 */
      L"Index",                     /* 0 */
      L"DeviceID",                  /* "Disk #0, Partition #0" */
      L"StartingOffset",            /* 6291456 [bytes] */
      L"Size"                       /* 85425389568 [bytes] */
    };
  /* Win32_DiskPartition: */
  struct PartInfo
    cur_part;
  int
    p = -1;
  /* */
  bool
    succ = false;

  /* 6. USE THE IWbemServices POINTER TO MAKE REQUESTS OF WMI: */
  { wchar_t
      query_string [127 + 1];
    BSTR
      query;

    /* The Win32_DiskDriveToDiskPartition association WMI class relates a disk drive and a partition existing on it.
    // Antecedent Data type:  Win32_DiskDrive
    // Dependent Data type:   Win32_DiskPartition
    */
    _snwprintf (query_string, _countof(query_string),
                L"ASSOCIATORS OF {Win32_DiskDrive.DeviceID='%s'} "
                L"WHERE AssocClass = Win32_DiskDriveToDiskPartition",
                disk->device_path);  /* "\\.\PHYSICALDRIVE0" */

    query = SysAllocString (query_string);

    if (query == NULL)
    {
      SetWinErr ();
      return (false);
    }

    { IEnumWbemClassObject
       *partEnum = NULL;
      HRESULT
        hres;

      hres = COM_OBJ(wbem_svc)->ExecQuery (
          COM_OBJ_THIS_(wbem_svc)     /* this, */
          IN  wql,                    /* WMI Query Language */
          IN  query,                  /* Win32_DiskDriveToDiskPartition */
          IN  WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, /* Suggested value */
          IN  NULL,                   /* Pointer to an IWbemContext */
          OUT &partEnum               /* Pointer to an Enumerator */
        );

      if (FAILED (hres))
      {
      /*MessageBox (NULL, COMErrorMessage(hres), TEXT("SvcExecQuery failed ..."), MB_ICONERROR);*/
        SetComErr (wbem_svc, hres);
        goto FreeQuery;
      }

      /* 7. GET THE DATA FROM THE QUERY IN STEP 6: */
      { IWbemClassObject *
          classObj = NULL;
        ULONG
          hasNext;

        while (partEnum != NULL)
        {
          CoReleaseObject (classObj);

          /* Move to next Object: */
          hres = COM_OBJ(partEnum)->Next (
                   COM_OBJ_THIS_(partEnum)
                   WBEM_INFINITE, 1, &classObj, &hasNext);

          if (FAILED (hres))
          {
          /*MessageBox (NULL, GetErrorString (hres), TEXT("Enumerating partitions ..."), MB_ICONERROR);*/
            SetComErr (partEnum, hres);
          /*Free (disk->part);*/  /* Is now allocated in `QueryDriveLayout()' */
            goto FreePartEnum;
          }

          if (hasNext==0)
            break;

          /* Get Properties: */
          { size_t
              i;
            VARIANT
              varProp;

            for (i=_countof(WMI_PARTITION_PROP)-1; i < _countof(WMI_PARTITION_PROP); --i)
            {
              hres = COM_OBJ(classObj)->Get (
                       COM_OBJ_THIS_(classObj)
                       WMI_PARTITION_PROP[i], 0, &varProp, 0, 0);

              if (FAILED (hres))
              {
              /*MessageBox (NULL, GetErrorString (hres), TEXT("Get partition properties ..."), MB_ICONERROR);*/
                SetComErr (classObj, hres);
              /*Free (disk->part);*/  /* Is now allocated in `QueryDriveLayout()' */
                goto FreeClassObj;
              }

              /* Inspect property value: */
              switch (i)
              {
              case 0: /* "DiskIndex" */
                disk->part[p].disk_index = (uint32_t)varProp.lVal;
                break;

              case 1: /* "Index" */
                disk->part[p].part_index = varProp.lVal;
                break;

              case 2: /* "DeviceID" */
                disk->part[p].device_id = VAR2TCHAR (&varProp);
                /*TODO: query drive letter?
                // Get-WmiObject -Query "ASSOCIATORS OF {Win32_DiskPartition.DeviceID='Disk #0, Partition #0'} WHERE AssocClass = Win32_LogicalDiskToPartition"
                */
                break;

/* In order to be able to merge the WMI results with the partition
   information returned by IOCTL_DISK_GET_DRIVE_LAYOUT_EX, the
   following two properties must be valid: */
              case 3: /* "StartingOffset" */
                if ((varProp.vt & VT_NULL) == VT_NULL || FAILED (
                       VarI8FromStr (varProp.bstrVal, LOCALE_SYSTEM_DEFAULT,
                                     LOCALE_NOUSEROVERRIDE, (LONG64 *)&cur_part.start_offset)))
                {
                /*cur_part.start_offset = (off_t)INT64_UNKNOWN;*/
                  i = 0;  /* unrecoverable error: cont. with next partition! */
                }
                else
                {
                  for (p=0; p < (int)disk->part_count; ++p)
                  {
                    if (disk->part[p].PartitionEntry.StartingOffset.QuadPart == (LONGLONG)cur_part.start_offset
                     && disk->part[p].PartitionEntry.PartitionLength.QuadPart == (LONGLONG)cur_part.part_size)
                      break;
                  }

                  if (p >= (int)disk->part_count)
                    i = 0;  /* unrecoverable error: cont. with next partition! */
                  else
                  {
                    disk->part[p].valid = true;
                    disk->part[p].start_offset = cur_part.start_offset;
                    disk->part[p].part_size = cur_part.part_size;
                  }
                }
                break;

              case 4: /* "Size" */
                if ((varProp.vt & VT_NULL) == VT_NULL || FAILED (
                       VarI8FromStr (varProp.bstrVal, LOCALE_SYSTEM_DEFAULT,
                                     LOCALE_NOUSEROVERRIDE, (LONG64 *)&cur_part.part_size)))
                {
                /*cur_part.part_size = (off_t)INT64_UNKNOWN;*/
                  i = 0;  /* unrecoverable error: cont. with next partition! */
                }
                break;
              }

              VariantClear (&varProp);
            }
          }
        }

        succ = true;

FreeClassObj:
        CoReleaseObject (classObj);
      }

FreePartEnum:
      CoReleaseObject (partEnum);
    }

FreeQuery:;
    SysFreeString (query);
  }

  return (succ);
}
#endif


static TCHAR *
  NTDeviceNameToVolumeGuid (TCHAR const nt_device_name []);
static bool
  NTDeviceNamespaceVolumeName (struct DiskInfo const *disk, struct PartInfo *diskpart);
static bool
  IsSameDrive (TCHAR const mount_point2 [], TCHAR const mount_point1 []);
static bool
  IsSameDriveLetter (TCHAR const drive_letter, TCHAR const *mount_point);


/* Inspired by Ext3FSD's Ext2Mgr:enumDisk.cpp:Ext2QueryDriveLayout() function */
static inline bool ValidPartition (PARTITION_INFORMATION_EX const *cur_part)
{
  if (cur_part)
  {
    if (cur_part->PartitionStyle == PARTITION_STYLE_MBR)
      return (cur_part->Mbr.PartitionType != PARTITION_ENTRY_UNUSED);
    else
    if (cur_part->PartitionStyle == PARTITION_STYLE_GPT)
      return (memcmp (&cur_part->Gpt.PartitionType, &GUID_NULL, sizeof(GUID_NULL)) != 0);
  }

  return (false);
}

static inline bool IsExtendedPartition (PARTITION_INFORMATION_EX const *cur_part)
{
  if (cur_part && cur_part->PartitionStyle == PARTITION_STYLE_MBR)
  {
    return (cur_part->Mbr.PartitionType == PARTITION_EXTENDED
         || cur_part->Mbr.PartitionType == PARTITION_XINT13_EXTENDED);
  }

  return (false);
}

static inline bool IsLogicalPartition (PARTITION_INFORMATION_EX const *cur_part,
                                       PARTITION_INFORMATION_EX const *ext_part)
{
  if (ext_part)  /* implies `cur_part->PartitionStyle == PARTITION_STYLE_MBR' */
  {
    if (cur_part && not IsExtendedPartition (cur_part))
    {
      LONGLONG const ext_part_start = ext_part->StartingOffset.QuadPart;
      LONGLONG const ext_part_end   = ext_part_start + ext_part->PartitionLength.QuadPart;

      return (cur_part->StartingOffset.QuadPart >= ext_part_start
           && cur_part->StartingOffset.QuadPart < ext_part_end);
    }
  }

  return (false);
}

static inline bool IsPrimaryPartitionIndex (uint32_t const disk_index, uint32_t const part_index)
{
  return (physical_disk[disk_index].log_part == 0 || part_index <
            (physical_disk[disk_index].part_count-physical_disk[disk_index].log_part-1));
}

static inline bool IsExtendedPartitionIndex (uint32_t const disk_index, uint32_t const part_index)
{
  return (physical_disk[disk_index].log_part > 0 && part_index ==
            (physical_disk[disk_index].part_count-physical_disk[disk_index].log_part-1));
}

static inline bool IsLogicalPartitionIndex (uint32_t const disk_index, uint32_t const part_index)
{
  return (not IsPrimaryPartitionIndex (disk_index, part_index)
       && not IsExtendedPartitionIndex (disk_index, part_index));
}

static int SortByStartingSector (void const *e1, void const *e2)
{ PARTITION_INFORMATION_EX const *p1 = (PARTITION_INFORMATION_EX const *)e1;
  PARTITION_INFORMATION_EX const *p2 = (PARTITION_INFORMATION_EX const *)e2;

  return ((p1->StartingOffset.QuadPart > p2->StartingOffset.QuadPart)
        - (p2->StartingOffset.QuadPart > p1->StartingOffset.QuadPart));
}

/* Is there any way of detecting if a drive is a SSD?
   As an alternative to the algorithm chosen below, the following post
   describes a really cool, alternative, solution:
   http://stackoverflow.com/questions/908188/is-there-any-way-of-detecting-if-a-drive-is-a-ssd#answer-908246
*/
#pragma warning (push)
#pragma warning (disable : 4091)  /* 'typedef ': ignored on left of '_NV_SEP_WRITE_CACHE_TYPE' when no variable is declared */
#include <ntddscsi.h>
#pragma warning (pop)

typedef struct
{
    ATA_PASS_THROUGH_EX header;
    WORD data[256];
} ATAIdentifyDeviceQuery;

static bool IsSolidStateDisk (struct DiskInfo const *disk)
{
  /* Somewhat crude, but seems to work ok: */
  bool is_ssd = (disk->caption && _tcsistr (disk->caption, TEXT("SSD")) != NULL);

  /* See if disk reports a nominal media rotation rate of 1:
     https://emoacht.wordpress.com/2012/11/06/csharp-ssd/
     https://blogs.technet.microsoft.com/filecab/2009/11/25/windows-7-disk-defragmenter-user-interface-overview/
     A "C" only implementation can be found on StackOverflow:
     http://stackoverflow.com/questions/23363115/detecting-ssd-in-windows#answer-33359142
  */
  if (is_ssd && elevated_process)
  { struct VolumeDesc const *
      vol;
    HANDLE
      vol_handle;
    ATAIdentifyDeviceQuery
      id_query = {0};
    DWORD
      dwBytesReturned;

    if ((vol=OpenVolumeRW (-1, disk->device_path)) == NULL)
      return (false);

    if ((vol_handle=GetVolumeHandle (vol)) == INVALID_HANDLE_VALUE)
    {
      CloseVolume (&vol);
      SetLastError (ERROR_INVALID_HANDLE);
      WinErrReturn (-1);
    }

    id_query.header.Length             = sizeof(id_query.header);
    id_query.header.AtaFlags           = ATA_FLAGS_DATA_IN;
    id_query.header.DataTransferLength = sizeof(id_query.data);
    id_query.header.TimeOutValue       = 5;     // Timeout in seconds
    id_query.header.DataBufferOffset   = offsetof(ATAIdentifyDeviceQuery, data[0]);
    id_query.header.CurrentTaskFile[6] = 0xEC;  // ATA IDENTIFY DEVICE

    if (DeviceIoControl (vol_handle,            // device to be queried
          IOCTL_ATA_PASS_THROUGH,               // operation to perform
          &id_query, sizeof(id_query),          // input buffer
          &id_query, sizeof(id_query),          // output buffer
          &dwBytesReturned,                     // # bytes returned
          (LPOVERLAPPED)NULL))                  // no overlapped I/O
    {
      //Index of nominal media rotation rate
      //SOURCE: http://www.t13.org/documents/UploadedDocuments/docs2009/d2015r1a-ATAATAPI_Command_Set_-_2_ACS-2.pdf
      //          7.18.7.81 Word 217
      //QUOTE: Word 217 indicates the nominal media rotation rate of the device and is defined in table:
      //          Value           Description
      //          --------------------------------
      //          0000h           Rate not reported
      //          0001h           Non-rotating media (e.g., solid state device)
      //          0002h-0400h     Reserved
      //          0401h-FFFEh     Nominal media rotation rate in rotations per minute (rpm)
      //                                  (e.g., 7 200 rpm = 1C20h)
      //          FFFFh           Reserved
      #define kNominalMediaRotRateWordIndex 217
      is_ssd = (id_query.data[kNominalMediaRotRateWordIndex] == 0x0001);
      #undef kNominalMediaRotRateWordIndex
    }

    CloseVolume (&vol);
  }

  return (is_ssd);
}

static bool QueryDriveLayout (struct DiskInfo *disk)
{ DRIVE_LAYOUT_INFORMATION_EX *
    drive_layout = NULL;
  PARTITION_INFORMATION_EX const *
    ext_part = NULL;
  /* */
  struct PartInfo *
    diskpart;
  int
    u;          /* number of partition entries in use */

  /* IOCTL_DISK_GET_DRIVE_LAYOUT_EX */
  { struct VolumeDesc *
      vol;

    if ((vol=OpenVolumeInfo (-1, disk->device_path)) != NULL)
    { HANDLE
        disk;
      int
        p;      /* number of retrieved partition entries */

      if ((disk=GetVolumeHandle (vol)) == INVALID_HANDLE_VALUE)
      {
        CloseVolume (&vol);
        SetLastError (ERROR_INVALID_HANDLE);
        WinErrReturn (-1);
      }

      for (p=1; ; ++p)
      { DWORD
          OutBufferSize = sizeof(*drive_layout) + (p - 1)*sizeof(*drive_layout->PartitionEntry);
        DWORD
          BytesReturned;

        drive_layout = (DRIVE_LAYOUT_INFORMATION_EX *)MemRealloc (drive_layout, 1, OutBufferSize);

        if (DeviceIoControl (disk,                           /* handle to device */
                             IOCTL_DISK_GET_DRIVE_LAYOUT_EX, /* dwIoControlCode */
                             NULL,                           /* lpInBuffer */
                             0,                              /* nInBufferSize */
                             (LPVOID)drive_layout,           /* output buffer */
                             OutBufferSize,                  /* size of output buffer */
                             &BytesReturned,                 /* number of bytes returned */
                             NULL))                          /* OVERLAPPED structure */
          break;

        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
          WinErrReturn (false);
      }

      CloseVolume (&vol);
    }
  }

  { PARTITION_INFORMATION_EX const *
      cur_part;
    int
      i, l = 0; /* number of logical partitions */

    /* Sort partitions (with respect to their starting sector): */
    qsort (
        drive_layout->PartitionEntry,
        drive_layout->PartitionCount,
        sizeof(*drive_layout->PartitionEntry),
        SortByStartingSector
      );

    /* Count partitions (must be done after qsort as ext_part may be set): */
    if (drive_layout->PartitionStyle == PARTITION_STYLE_MBR)
    {
      for (i=u=0; i < (int)drive_layout->PartitionCount; ++i)
      {
        cur_part = drive_layout->PartitionEntry + i;

        if (ValidPartition (cur_part))
        {
          ++u;

          if (IsExtendedPartition (cur_part) &&
                (ext_part == NULL ||
                 cur_part->PartitionLength.QuadPart > ext_part->PartitionLength.QuadPart))
          {
            ext_part = cur_part;
          }
        }
      }

      /* Any logical partitions? */
      if (ext_part != NULL)
      {
        for (i=0; i < (int)drive_layout->PartitionCount; ++i)
        {
          cur_part = drive_layout->PartitionEntry + i;

          if (ValidPartition (cur_part))
          {
            if (IsExtendedPartition (cur_part))
              u -= (cur_part != ext_part);
            else
            if (IsLogicalPartition (cur_part, ext_part))
              ++l;
          }
        }
      }
    }
    else
    if (drive_layout->PartitionStyle == PARTITION_STYLE_GPT)
      u = drive_layout->PartitionCount;
    else
      u = 0;

    /* Store partition array: */
    disk->PartitionStyle = drive_layout->PartitionStyle;
    disk->part_count = u;
    disk->log_part = l;

    disk->part = diskpart = (struct PartInfo *)MemAlloc (max(u,1), sizeof(*diskpart));
    memset (disk->part, 0, max(u,1) * sizeof(*diskpart));  /* disk->part[p].valid = false; */

    if (u > 0)
    {
      /* Extended partition? */
      if (ext_part != NULL)
        memcpy (&diskpart [u-l-1].PartitionEntry, ext_part, sizeof(*ext_part));

      /* Primary and logical partitions: */
      { struct PartInfo *
          partinfo;

        for (i=0; i < (int)drive_layout->PartitionCount; ++i)
        {
          cur_part = drive_layout->PartitionEntry + i;

          if (not ValidPartition (cur_part))
            continue;

          /* Copy partition information: */
          if (drive_layout->PartitionStyle == PARTITION_STYLE_GPT)
          {
            partinfo = diskpart;
            ++diskpart; --u;

            /* Partition recognized by Windows? */
            partinfo->is_windows_part = GPTGuidRecognizedByWindows (cur_part);
          }
          else
          {
            if (IsExtendedPartition (cur_part)) /* includes `ext_part' */
              continue;

            /* Ensure that PartitionEntry's of logical partitions (within an
               extended partition) are stored at the end of the array: */
            if (IsLogicalPartition (cur_part, ext_part))
            {
              partinfo = &diskpart [u - l];
              --l;
            }
            else
            {
              partinfo = diskpart;
              ++diskpart; --u;
            }

            /* Partition recognized by Windows? */
            partinfo->is_windows_part = MBRTypeRecognizedByWindows (cur_part);
          }

          memcpy (&partinfo->PartitionEntry, cur_part, sizeof(*cur_part));

          /* Query `harddisk_volume' identifer and find the corresponding volume: */
          if (not NTDeviceNamespaceVolumeName (disk, partinfo))
            partinfo->volume = NULL;
        }
      }
    }
  }

  /* Ensure that all `volume' pointers at least reference a "dummy" volume: */
  { uint32_t
      p;

    for (p=0; p < max(disk->part_count,1); ++p)
    {
      struct PartInfo *partinfo = &disk->part [p];

      if (partinfo->volume == NULL)
      {
        partinfo->volume = (struct VolumeInfo *)MemAlloc (1, sizeof(*partinfo->volume));
        memset (partinfo->volume, 0, sizeof(*partinfo->volume));

        /* Always set `mount_point' to at least one (empty) string: */
        partinfo->volume->mount_point = (TCHAR **)MemAlloc (1, sizeof(*partinfo->volume->mount_point));
        partinfo->volume->mount_point[0] = (TCHAR *)MemAlloc (1, sizeof(**partinfo->volume->mount_point));
        partinfo->volume->mount_point[0][0] = TEXT('\0');
      }
    }
  }

  return (true);
}


static bool SetupCdromDummyPartition (struct DiskInfo *disk)
{ struct PartInfo *
    diskpart;

  /* Store partition array: */
  disk->PartitionStyle = PARTITION_STYLE_MBR;
  disk->part_count = 1;
  disk->log_part = 0;

  disk->part = diskpart = (struct PartInfo *)MemAlloc (1, sizeof(*diskpart));
  memset (disk->part, 0, sizeof(*diskpart));  /* disk->part[0].valid = false; */

  /* L"\\Device\\Cdrom0"  =>  L"\\\\?\\Volume{4617af63-b3d6-11e5-8d6b-806e6f6e6963}" */
  { uint32_t
      v;

    TCHAR *cdrom_volume_guid = NTDeviceNameToVolumeGuid (disk->objmgr_path);

    /* Search corresponding `struct VolumeInfo': */
    if (cdrom_volume_guid != NULL)
    {
      for (v=0; logical_volume[v].valid; ++v)
        if (EqualLogicalVolumeGuid (logical_volume[v].volume_guid, cdrom_volume_guid))
          break;

      if (logical_volume[v].valid)
        Free (cdrom_volume_guid);
      else
      {
        _ftprintf (stderr, TEXT("\n[%s] The CD/DVD/BD GUID '%s'\n")
                           TEXT("has no logical volume entry; this is likely a bug ... aborting!\n"),
                           TEXT(PROGRAM_NAME), cdrom_volume_guid);
        exit (247);
      }

      disk->part[0].volume = logical_volume + v;
    }
    else
    /* Ensure that the `volume' pointer at least reference a "dummy" volume: */
    { struct PartInfo *partinfo = &disk->part[0];

      partinfo->volume = (struct VolumeInfo *)MemAlloc (1, sizeof(*partinfo->volume));
      memset (partinfo->volume, 0, sizeof(*partinfo->volume));

      /* Always set `mount_point' to at least one (empty) string: */
      partinfo->volume->mount_point = (TCHAR **)MemAlloc (1, sizeof(*partinfo->volume->mount_point));
      partinfo->volume->mount_point[0] = (TCHAR *)MemAlloc (1, sizeof(**partinfo->volume->mount_point));
      partinfo->volume->mount_point[0][0] = TEXT('\0');
      return (false);
    }
  }

  return (disk->part[0].volume != NULL);
}


/*
// Get-WmiObject Win32_DiskDrive | Select *
// Get-WmiObject Win32_CDROMDrive | Select *
*/
static int SortByDiskIndex (void const *e1, void const *e2)
{ struct DiskInfo const *d1 = (struct DiskInfo const *)e1;
  struct DiskInfo const *d2 = (struct DiskInfo const *)e2;

  /* ORDER BY disk_index ASC; */
  return ((d1->DiskNumber > d2->DiskNumber) - (d2->DiskNumber > d1->DiskNumber));
}

static int SortByCdromDevice (void const *e1, void const *e2)
{ struct DiskInfo const *d1 = (struct DiskInfo const *)e1;
  struct DiskInfo const *d2 = (struct DiskInfo const *)e2;

  if (d1->objmgr_path == NULL || d1->objmgr_path[0] == TEXT('\0'))
    return (-1);

  if (d2->objmgr_path == NULL || d2->objmgr_path[0] == TEXT('\0'))
    return (+1);

  /* Should work for \Device\CdRom0 up to \Device\Cdrom9 */
  return (_wcsicmp (d1->objmgr_path, d2->objmgr_path));
}

static bool HarddiskObjectDirectoryCallback (LPCWSTR ObjectName, enum EObjectTypes ObjectType, LPCWSTR LinksToName)
{
  if (physical_disk[sel_disk].part_count == 0)
    return (ENUM_ENTRIES_STOP);

  { uint32_t
      p;

    for (p=0; p < physical_disk[sel_disk].part_count; ++p)
      if (physical_disk[sel_disk].part[p].harddisk_volume != NULL
       && _wcsicmp (LinksToName, physical_disk[sel_disk].part[p].harddisk_volume) == 0)
      {
        size_t length = wcslen(physical_disk[sel_disk].objmgr_path) + 1
                      + wcslen(ObjectName);
        physical_disk[sel_disk].part[p].harddisk_partition = (TCHAR *)MemAlloc (
            length+1, sizeof(*physical_disk[sel_disk].part[p].harddisk_partition));
        _sntprintf (physical_disk[sel_disk].part[p].harddisk_partition, length+1, TEXT("%s\\%s"),
                    physical_disk[sel_disk].objmgr_path, ObjectName);
        break;
      }
  }

  return (ENUM_ENTRIES_CONTINUE);
}

extern bool WMIEnumDrives (void)
{ wchar_t const *
    WMI_HARDDISK_PROP[] = {
        L"Caption",           /* "Samsung SSD 850 EVO 1TB" */
        L"FirmwareRevision",  /* "EMT02B6Q" */
        L"DeviceID",          /* "\\.\PHYSICALDRIVE0" */
        /* Win32_DiskDrive */
        L"Index",             /* 0 */
        L"Size",              /* "1000202273280" [bytes] */
        L"SerialNumber",      /* "S21DNXAGB02351F" */
        L"Partitions",        /* e.g. "6" */
      };
  wchar_t const *
    WMI_CDROMDRIVE_PROP[] = {
        L"Caption",           /* "HL-DT-ST DVDRAM GUC0N" */
        L"MfrAssignedRevisionLevel", /* "T.02" */
        L"Drive",             /* "N:" */
      };
  wchar_t const **
    WMI_DRIVE_PROP[] = {WMI_HARDDISK_PROP, WMI_CDROMDRIVE_PROP};
  /* */
  bool
    succ = false;
  uint32_t
    step, disk_count, d;

  /* Preparational step: query logical drive information first, so that drive
     extents can be mapped to partition table entries later on: */
  if (not EnumLogicalVolumes ())
    return (false);

  /* Just to make sure inter-thread WMI access would work (marshaling/unmarshaling
     is only really needed if the service ptr is acquired in a different thread): */
  WmiMarshalWbemServicePtr ();
  WmiUnmarshalWbemServicePtr ();

  if (not svc_valid() || not wmi_avail())
    return (false);

  /* 6. USE THE IWbemServices POINTER TO MAKE REQUESTS OF WMI: */
  for (step=0, d=INVALID_DISK; step < 2; ++step)
  { BSTR
      query;

    if (step == 0)
      query = SysAllocString (L"select * from Win32_DiskDrive");
    else
      query = SysAllocString (L"select * from Win32_CDROMDrive");

    if (query == NULL)
    {
      SetWinErr ();
      return (false);
    }

    { IEnumWbemClassObject
       *diskEnum = NULL;
      HRESULT
        hres;

      hres = COM_OBJ(wbem_svc)->ExecQuery (
          COM_OBJ_THIS_(wbem_svc)     /* this, */
          IN  wql,                    /* WMI Query Language */
          IN  query,                  /* Win32_DiskDrive */
          IN  WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, /* Suggested value */
          IN  NULL,                   /* Pointer to an IWbemContext */
          OUT &diskEnum               /* Pointer to an Enumerator */
        );

      if (FAILED (hres))
      {
      /*MessageBox (NULL, COMErrorMessage(hres), TEXT("SvcExecQuery failed ..."), MB_ICONERROR);*/
        SetComErr (wbem_svc, hres);
        goto FreeQuery;
      }

      /* 7. GET THE DATA FROM THE QUERY IN STEP 6: */
      { IWbemClassObject *
          classObj = NULL;
        ULONG
          hasNext;

        while (diskEnum != NULL)
        {
          CoReleaseObject (classObj);

          /* Move to next Object: */
          hres = COM_OBJ(diskEnum)->Next (
                   COM_OBJ_THIS_(diskEnum)
                   WBEM_INFINITE, 1, &classObj, &hasNext);

          if (FAILED (hres))
          {
          /*MessageBox (NULL, GetErrorString (hres), TEXT("Enumerating harddisks ..."), MB_ICONERROR);*/
            SetComErr (diskEnum, hres);
            Free (physical_disk);
            goto FreeDiskEnum;
          }

          if (hasNext==0)
            break;

          /* Realloc `physical_disk' array: */
          ++d;
          physical_disk = (struct DiskInfo *)MemRealloc (physical_disk, d+2, sizeof(*physical_disk));
          memset (physical_disk + d, 0, 2*sizeof(*physical_disk));

          /* Get Properties: */
          physical_disk[d].type = physical_disk[d+1].type = DTYPE_INVALID;

          { VARIANT
              varProp;
            size_t
              i;

            for (i=_countof(WMI_HARDDISK_PROP)-step*4-1; i < _countof(WMI_HARDDISK_PROP); --i)
            {
              hres = COM_OBJ(classObj)->Get (
                       COM_OBJ_THIS_(classObj)
                       WMI_DRIVE_PROP[step][i], 0, &varProp, 0, 0);

              if (FAILED (hres))
              {
              /*MessageBox (NULL, GetErrorString (hres), TEXT("Get harddisk properties ..."), MB_ICONERROR);*/
                SetComErr (classObj, hres);
                Free (physical_disk);
                goto FreeClassObj;
              }

              /* Inspect property value: */
              #if defined(_DEBUG) && 0
              if ((varProp.vt & VT_I4) == VT_I4)
                wprintf (L"%s = %" TEXT(PRId32) L"\n", WMI_HARDDISK_PROP[i], (int32_t)varProp.lVal);
              else
                wprintf (L"%s = %s\n", WMI_HARDDISK_PROP[i], varProp.bstrVal);
              #endif

              physical_disk[d].type = (step == 0) ? DTYPE_LOC_DISK : DTYPE_OPT_DISK;

              switch (i)
              {
              case 0: /* "Caption" */
                physical_disk[d].caption = VAR2TCHAR (&varProp);
                if (physical_disk[d].type == DTYPE_LOC_DISK && IsSolidStateDisk (physical_disk + d))
                  physical_disk[d].type = (DTYPE_FIXED_SSD | DTYPE_LOC_DISK);
                break;

              case 1: /* "FirmwareRevision" */
                physical_disk[d].firmware_rev = VAR2TCHAR (&varProp);
                break;

              case 2: /* "DeviceID", "Drive" */
                physical_disk[d].device_path = VAR2TCHAR (&varProp);
                physical_disk[d].objmgr_path = (TCHAR *)MemAlloc (MAX_PATH + 1, sizeof (*physical_disk[d].objmgr_path));

                if (IsLocalDisk (physical_disk[d].type))
                {
                  if (not QueryDriveLayout (physical_disk + d)
                      /*|| not WMIQueryPartitions (local_storage + d)*/
                      )
                  {
                    VariantClear (&varProp);
                    Free (physical_disk);
                    goto FreeClassObj;
                  }

                  _sntprintf (physical_disk[d].objmgr_path, MAX_PATH, TEXT("\\Device\\Harddisk%") TEXT(PRIu32),
                                                            physical_disk[d].DiskNumber);
                }
                else
                {
                  physical_disk[d].DiskNumber = (uint32_t)(d - disk_count);

                  /* "\Device\CdRom0" */
                  if (QueryDosDeviceW (physical_disk[d].device_path, physical_disk[d].objmgr_path, MAX_PATH) == 0)
                  {
                    Free (physical_disk[d].objmgr_path);
                  }

                  /* Allocate a single "dummy" partition to store the drive letter: */
                  SetupCdromDummyPartition (physical_disk + d);
                }
                break;

              case 3: /* "Index" */
                physical_disk[d].DiskNumber = (uint32_t)varProp.lVal;
                break;

              case 4: /* "Size" */
                if ((varProp.vt & VT_NULL) == VT_NULL || FAILED (
                  VarI8FromStr (varProp.bstrVal, LOCALE_SYSTEM_DEFAULT,
                                LOCALE_NOUSEROVERRIDE, (LONG64 *)&physical_disk[d].size_bytes)))
                  physical_disk[d].size_bytes = (off_t)INT64_UNKNOWN;
                break;

              case 5: /* "SerialNumber" */
                physical_disk[d].serial_number = VAR2TCHAR (&varProp);
                break;

              case 6: /* "Partitions" */
                physical_disk[d].part_count = (uint32_t)varProp.lVal;

                /* Is now allocated in `QueryDriveLayout()' */
                /*
                local_storage[d].part = (struct PartInfo *)MemAlloc (varProp.lVal, sizeof(*local_storage[d].part));
                memset (local_storage[d].part, 0, varProp.lVal * sizeof(*local_storage[d].part));
                */
                break;
              }

VariantClear (&varProp);
            }

            /* Harddisk? */
            if (step == 0 && elevated_process)
            {
              sel_disk = d;
              EnumObjectDirectory (physical_disk[d].objmgr_path, &HarddiskObjectDirectoryCallback);
              sel_disk = INVALID_DISK;
            }
          }
        }

        /* Sort disks on first iteration, and set `succ' on second one: */
        if (step == 0)
        {
          disk_count = 1 + d;
          qsort (physical_disk, disk_count, sizeof (*physical_disk), &SortByDiskIndex);
        }
        else /*if (step == 1)*/
        {
          qsort (physical_disk + disk_count, d - disk_count + 1, sizeof (*physical_disk), &SortByCdromDevice);
          succ = true;
        }

FreeClassObj:
        CoReleaseObject (classObj);
      }

FreeDiskEnum:
      CoReleaseObject (diskEnum);
    }

FreeQuery:;
    SysFreeString (query);
  }

  /* Allocate a dummy disk to mark the end of the array, if there aren't any: */
  if (physical_disk == NULL)
  {
    physical_disk = (struct DiskInfo *)MemAlloc (1, sizeof(*physical_disk));
    memset (physical_disk, 0, sizeof(*physical_disk));
    physical_disk[0].type = DTYPE_INVALID;
  }

  if (logical_volume == NULL)
  {
    logical_volume = (struct VolumeInfo *)MemAlloc (1, sizeof(*logical_volume));
    memset (logical_volume, 0, sizeof(*logical_volume));
    logical_volume[0].valid = false;
  }

  /* Fix-up `part' pointer of logical volumes: */
  { uint32_t
      p;

    for (d=0; physical_disk[d].type != DTYPE_INVALID; ++d)
      for (p=0; p < physical_disk[d].part_count; ++p)
      { PARTITION_INFORMATION_EX const *
          CurrentPartition = &physical_disk[d].part[p].PartitionEntry;
        uint32_t
          v, e;

        if (IsExtendedPartitionIndex (d, p))
          continue;

        for (v=0; logical_volume[v].valid; ++v)
          for (e=0; e < logical_volume[v].extent_count; ++v)
            if (logical_volume[v].extent[e].info.DiskNumber == physical_disk[d].DiskNumber /* DiskNumber MUST match! */
             && PartitionsOverlap (
                    logical_volume[v].extent[e].info.StartingOffset.QuadPart,
                      logical_volume[v].extent[e].info.ExtentLength.QuadPart,
                    CurrentPartition->StartingOffset.QuadPart,
                      CurrentPartition->PartitionLength.QuadPart))
            { bool
                dynamic_match = false,
                exact_match = (logical_volume[v].extent[e].info.StartingOffset.QuadPart ==
                                 CurrentPartition->StartingOffset.QuadPart
                            && logical_volume[v].extent[e].info.ExtentLength.QuadPart ==
                                 CurrentPartition->PartitionLength.QuadPart);

              if (MaybeDynamicDisk (physical_disk + d))
              {
                dynamic_match = DynamicPartitionMatch (
                                    logical_volume[v].extent[e].info.StartingOffset.QuadPart,
                                      logical_volume[v].extent[e].info.ExtentLength.QuadPart,
                                    CurrentPartition->StartingOffset.QuadPart,
                                      CurrentPartition->PartitionLength.QuadPart);
              }

              if (exact_match || dynamic_match)
              {
                logical_volume[v].extent[e].part = physical_disk[d].part + p;

                if (dynamic_match)
                {
                  /* Correct previously assigned volume reference (see `QueryDriveLayout()'): */
                  if (not physical_disk[d].part[p].volume->valid)
                  {
                    Free (physical_disk[d].part[p].volume->mount_point[0]);
                    Free (physical_disk[d].part[p].volume->mount_point);
                    Free (physical_disk[d].part[p].volume);

                    physical_disk[d].part[p].volume = logical_volume + v;
                  }
                  else
                    if (physical_disk[d].part[p].volume != logical_volume + v)
                    {
                      fprintf (stderr, "\n[%s] physical_disk[%" PRIu32 "].part[%" PRIu32 "].volume != logical_volume[%" PRIu32 "];\n"
                                       "this is likely a bug ... aborting!\n",
                                       PROGRAM_NAME, d, p, v);
                      exit (247);
                    }
                }

                if (logical_volume[v].is_system_drive)
                  physical_disk[d].contains_system = true;
              }
              else
              {
                _ftprintf (stderr, TEXT("\n[%s] The volume '%s'\n")
                                   TEXT("partially overlaps partition (%") TEXT(PRIu32) TEXT(",%") TEXT(PRIu32) TEXT("), but doesn't match it ... aborting!\n"),
                                   TEXT(PROGRAM_NAME), logical_volume[v].volume_guid, d, p);
                exit (245);
              }
            }
      }
  }

  return (true);
}


/* Detected drives: */
extern uint32_t number_of_local_disks (void)
{
  if (physical_disk != NULL)
  { uint32_t
      d, count = 0;

    for (d=0; physical_disk[d].type != DTYPE_INVALID; ++d)
      if (IsLocalDisk (physical_disk[d].type))
        ++count;

    return (count);
  }

  return (0);  /* no drives at all */
}


extern uint32_t index_nth_local_disk (uint32_t disk_index)
{
  if (physical_disk != NULL)
  { uint32_t
      d;

    for (d=0; physical_disk[d].type != DTYPE_INVALID; ++d)
      if (IsLocalDisk (physical_disk[d].type))
        if (disk_index == 0)
          return (d);
        else
          --disk_index;
  }

  return (INVALID_DISK);  /* invalid disk index */
}


extern uint32_t number_of_disk_partitions (uint32_t const disk_index)
{
  uint32_t d = index_nth_local_disk (disk_index);

  if (d == INVALID_DISK)
    return (0);

  return (physical_disk[d].part_count);
}


extern uint32_t number_of_optical_disks (void)
{
  if (physical_disk != NULL)
  { uint32_t
      d, count = 0;

    for (d=0; physical_disk[d].type != DTYPE_INVALID; ++d)
      if (IsOpticalDisk (physical_disk[d].type))
        ++count;

    return (count);
  }

  return (0);  /* no drives at all */
}


extern uint32_t index_nth_optical_disk (uint32_t disk_index)
{
  if (physical_disk != NULL)
  { uint32_t
      d;

    for (d=0; physical_disk[d].type != DTYPE_INVALID; ++d)
      if (IsOpticalDisk (physical_disk[d].type))
        if (disk_index == 0)
          return (d);
        else
          --disk_index;
  }

  return (INVALID_DISK);  /* invalid disk index */
}


extern void DumpDrivesAndPartitions (void)
{ enum EConsoleColor const
    drive_color = LIGHTGREEN,
    part_color = YELLOW,
    cdrom_color = YELLOW,
    ssd_color = WHITE;
  wchar_t
    offstr [31+1],
    lenstr [31+1];
  wdx_t
    wdx_id;

  if (physical_disk[0].type != DTYPE_INVALID)
  { uint32_t
      d, v;

    if (number_of_local_disks () > 0)
    { uint32_t
        p, e;
      TCHAR const
       *cur_mount_point;

      fprintf (stdout,
"LOGICAL VOLUMES:                        offset [byte]             length [byte]\n"
"===============================================================================\n");

      for (v=0; logical_volume[v].valid; ++v)
      {
        /* e.g. "/dev/wda" */
        textcolor (drive_color);
        fprintf (stdout, "Volume %-3d ", v+1);
        textcolor (DEFAULT_COLOR);

        if (logical_volume[v].is_system_drive)
        {
          textcolor (ssd_color);
          fprintf (stdout, "SYS");
          textcolor (DEFAULT_COLOR);
        }
        else if (logical_volume[v].extent_count != 0)
          fprintf (stdout, "   ");

        if (logical_volume[v].extent_count == 0)
          fprintf (stdout, " no volume extents");
        else
        if (logical_volume[v].extent_count == 1)
          fprintf (stdout, "  1 volume ext.");
        else
          fprintf (stdout, "  %" PRIu32 " volume ext.", logical_volume[v].extent_count);

        /* details */
        _ftprintf (stdout, TEXT(" %s"), logical_volume[v].volume_guid);

        if (logical_volume[v].extent_count == 0)
        {
        /*_ftprintf (stdout, TEXT("\n"));
          continue;*/
        }

        e = INVALID_EXTENT;
        do {
          if (logical_volume[v].extent_count > 0)
            ++e;

          if (VolumeToDiskPartition (logical_volume + v, e, &d, &p))
          {
            /* Display /dev/wdx identifer: */
            textcolor (part_color);
            if (IsLocalDisk (physical_disk[d].type))
              diskpart2wdx (physical_disk[d].DiskNumber, p, wdx_id);
            else
              cdrom2wdx (physical_disk[d].DiskNumber, wdx_id);
            fprintf (stdout, "\n    /dev/%-11s", wdx_id);
            textcolor (DEFAULT_COLOR);
          }
          else if (logical_volume[v].extent_count > 0)
            fprintf (stdout, "                     ");

          if (logical_volume[v].extent_count > 0)
          {
          /*_ftprintf (stdout, TEXT("        %25") TEXT(PRIdMAX) TEXT(" %25") TEXT(PRIdMAX),
                               (intmax_t)logical_volume[v].extent[e].info.StartingOffset.QuadPart,
                               (intmax_t)logical_volume[v].extent[e].info.ExtentLength.QuadPart);*/
            FormatInt64 (logical_volume[v].extent[e].info.StartingOffset.QuadPart, offstr, _countof(offstr));
            FormatInt64 (logical_volume[v].extent[e].info.ExtentLength.QuadPart, lenstr, _countof(lenstr));
            _ftprintf (stdout, TEXT("        %25s %25s"), offstr, lenstr);
          }
        } while (e + 1 < logical_volume[v].extent_count);

        /* Volume mount points: */
        { TCHAR
            trunc_mount_point [54 + 1];
          size_t
            i = GetMountPointCount (logical_volume + v);

          fprintf (stdout, "\n                This volume has %" PRIuMAX " mount point", (uintmax_t)i);
          if (i == 0)
            fprintf (stdout, "s.");
          else
          if (i > 1)
            fprintf (stdout, "s:");
          else
            fprintf (stdout, ":");

          for (i=0; logical_volume[v].mount_point[i][0] != TEXT('\0'); ++i)
          { bool
              truncated = (_sntprintf_s (trunc_mount_point, _countof(trunc_mount_point), _TRUNCATE,
                                          TEXT("%s"), logical_volume[v].mount_point[i]) == -1);

            _ftprintf (stdout, TEXT("\n                     %s"), trunc_mount_point);
            if (truncated)
              fprintf (stdout, " ...");
          }
        }

        _ftprintf (stdout, TEXT("\n"));
      }

      fprintf (stdout,
"\nPHYSICAL DISKS:");

      for (d=0; physical_disk[d].type != DTYPE_INVALID; ++d)
        if (IsLocalDisk (physical_disk[d].type))
        {
          if (d > 0)
            fprintf (stdout, "               ");

          fprintf (stdout,
"                         offset [byte]             length [byte]\n"
"===============================================================================\n");
          /* e.g. "/dev/wda" */
          textcolor (drive_color);
        /*_sntprintf (buffer, _countof(buffer), TEXT("/dev/wd%c"), TEXT('a') + local_storage[d].disk_index);*/
          disk2wdx (physical_disk[d].DiskNumber, wdx_id);
          fprintf (stdout, "/dev/%s   ", wdx_id);
          textcolor (DEFAULT_COLOR);

          switch (physical_disk[d].PartitionStyle)
          {
          case PARTITION_STYLE_MBR: _ftprintf (stdout, TEXT("MBR   ")); break;
          case PARTITION_STYLE_GPT: _ftprintf (stdout, TEXT("GPT   ")); break;
          default:
          case PARTITION_STYLE_RAW: _ftprintf (stdout, TEXT("RAW   ")); break;
          }

          if (IsSolidStateDrive (physical_disk[d].type))
          {
            textcolor (ssd_color);
            fprintf (stdout, "SSD ");
            textcolor (DEFAULT_COLOR);
          }
          else
            fprintf (stdout, "    ");

          /* details */
          _ftprintf (stdout, TEXT("   %-25.25s\n"),
                             physical_disk[d].caption);
          _ftprintf (stdout, TEXT("  %-20.20s  %") TEXT(PRIuMAX) TEXT(" bytes, FW rev: %-10.10s\n"),
                             physical_disk[d].device_path,
                             (uintmax_t)physical_disk[d].size_bytes,
                             physical_disk[d].firmware_rev);

          if (physical_disk[d].part_count == 0)
          {
            _ftprintf (stdout, TEXT("\n"));
            continue;
          }

          for (p=0; p < physical_disk[d].part_count; ++p)
          {
            /* e.g. "/dev/sda1" */
            textcolor (drive_color);
            _ftprintf (stdout, TEXT("    "));
            textcolor (part_color);
          /*_sntprintf (buffer, _countof(buffer), TEXT("/sd%c%-3d"), TEXT('a') + local_storage[d].disk_index, local_storage[d].part[p].part_index);*/
            diskpart2wdx (physical_disk[d].DiskNumber, p/*local_storage[d].part[p].PartitionEntry.PartitionNumber*/, wdx_id);
            fprintf (stdout, "/dev/%-11s", wdx_id);
            textcolor (DEFAULT_COLOR);

            /* details */  /*TODO: test if (cur_mount_point = physical_disk[d].part[p].volume != NULL) */
            cur_mount_point = physical_disk[d].part[p].volume->mount_point[0];
            if (not valid_mount_point (cur_mount_point) || IsDirMountPoint (cur_mount_point))
              cur_mount_point = TEXT("");

            FormatInt64 (physical_disk[d].part[p].PartitionEntry.StartingOffset.QuadPart, offstr, _countof(offstr));
            FormatInt64 (physical_disk[d].part[p].PartitionEntry.PartitionLength.QuadPart, lenstr, _countof(lenstr));
          /*_ftprintf (stdout, TEXT(" %c%c %-3.3s %25") TEXT(PRIdMAX) TEXT(" %25") TEXT(PRIdMAX) TEXT("\n"),*/
            _ftprintf (stdout, TEXT(" %c%c %-3.3s %25s %25s\n"),
                               _totupper(*cur_mount_point|('A'^'a')), (*cur_mount_point? L':' : L' '),
                               (IsExtendedPartitionIndex (d, p)? TEXT("EXT") : TEXT("")),
                             /*(intmax_t)local_storage[d].part[p].start_offset,*/offstr,
                             /*(intmax_t)local_storage[d].part[p].part_size);*/lenstr);

            if (physical_disk[d].part[p].harddisk_partition != NULL)
              _ftprintf (stdout, TEXT("    %s  -->  "), physical_disk[d].part[p].harddisk_partition);
            else
            if (physical_disk[d].part[p].harddisk_volume != NULL)
              _ftprintf (stdout, TEXT("    "));

            if (physical_disk[d].part[p].harddisk_volume != NULL)
            {
              _ftprintf (stdout, TEXT("%s"), physical_disk[d].part[p].harddisk_volume);
              /* Somewhat redundant; display volume id if not executed with elevated privileges! */
              if (not elevated_process && physical_disk[d].part[p].volume->volume_guid != NULL)
                _ftprintf (stdout, TEXT(":  %s"), physical_disk[d].part[p].volume->volume_guid);
              _ftprintf (stdout, TEXT("\n"));
            }
          }

          _ftprintf (stdout, TEXT("\n"));
        }
    }

    /* CD/DVD/BD drives? */
    if (number_of_optical_disks () > 0)
    {
      fprintf (stdout, "CD/DVD/BD DRIVES:\n"
"===============================================================================\n");

      for (d=0; physical_disk[d].type != DTYPE_INVALID; ++d)
        if (IsOpticalDisk (physical_disk[d].type))
        {
          textcolor (cdrom_color);
        /*_sntprintf (buffer, _countof(buffer), TEXT("/dev/wr%") TEXT(PRId32) TEXT("), local_storage[d].disk_index);*/
          cdrom2wdx (physical_disk[d].DiskNumber, wdx_id);
          fprintf (stdout, "/dev/%-10s", wdx_id);
          textcolor (DEFAULT_COLOR);
          _ftprintf (stdout, TEXT("      %c%c  %-33.33s  FW rev: %s\n"),
                           /*local_storage[d].device_path,*/
                             _totupper(physical_disk[d].part[0].volume->mount_point[0][0]|('A'^'a')),
                             (physical_disk[d].part[0].volume->mount_point[0][0]? L':' : L' '),
                             physical_disk[d].caption,
                             physical_disk[d].firmware_rev);

          if (physical_disk[d].objmgr_path != NULL)
          {
            _ftprintf (stdout, TEXT("    %s"), physical_disk[d].objmgr_path);

            if (physical_disk[d].part[0].volume->volume_guid != NULL)
              _ftprintf (stdout, TEXT(":           %s\n"), physical_disk[d].part[0].volume->volume_guid);
            else
              _ftprintf (stdout, TEXT("\n"));
          }

        /*_ftprintf (stdout, TEXT("\n"));*/
      }
    }
  }
  else
  {
    fprintf (stdout,
"LOCAL DRIVES:\n"
"---  none found!\n");
  }

  textcolor (DEFAULT_COLOR);
}


/* Drive selection and paths: */
extern void drives_reset_selection (void)
{
  sel_nt_devname = false;

  /* Physical disks: */
  sel_disk = INVALID_DISK;
  sel_part = INVALID_PART;

  /* Logical volumes: */
  sel_volume = INVALID_VOLUME;
  sel_extent = INVALID_EXTENT;
  sel_mount_point = NULL;
}


static void select_diskpartition (uint32_t const disk_index, uint32_t const part_index, TCHAR const *mount_point)
{
  sel_disk = disk_index;
  sel_part = part_index;

  if (part_index != INVALID_PART)
    DiskPartitionToVolume (sel_disk, sel_part, &sel_volume, &sel_extent);
  else
    DiskPartitionToVolume (sel_disk, sel_part, &sel_volume, NULL);

  if (mount_point != NULL)
    sel_mount_point = mount_point;
}


static void select_disk (uint32_t const disk_index, TCHAR const *mount_point)
{
  select_diskpartition (disk_index, INVALID_PART, mount_point);
}


static void select_volume (uint32_t const vol_index, TCHAR const *mount_point)
{
  sel_volume = vol_index;
  sel_extent = INVALID_EXTENT;

  VolumeToDiskPartition (logical_volume + vol_index, sel_extent, &sel_disk, &sel_part);

  if (mount_point != NULL)
    sel_mount_point = mount_point;
}


extern bool select_drive_by_wdxid (wdx_t const wdx_id, TCHAR const **mount_point)
{
  drives_reset_selection ();
  if (mount_point != NULL)
   *mount_point = NULL;

  if (physical_disk != NULL)
  { uint32_t
      disk_index;
    enum EDriveType
      drive_type;
    bool
      drive_partition;

    if (not is_wdx_id (wdx_id, &drive_type, &drive_partition))
    {
      fprintf (stderr, "\n[%s] select_drive(): wdx identifier '%s' is invalid\n",
                           PROGRAM_NAME, wdx_id);
      ErrnoReturn (EINVAL, false);
    }

    /* Harddisk? */
    if (IsLocalDisk (drive_type))
    { uint32_t
        part_index;

      if (drive_partition)
      {
        wdx2diskpart (wdx_id, &disk_index, &part_index);  /*Must succeed*/
      }
      else
      {
        wdx2disk (wdx_id, &disk_index);  /*Must succeed*/
        part_index = INVALID_PART;
      }

      /* Invalid disk? */
      if (disk_index >= number_of_local_disks ())
      {
        fprintf (stderr, "\n[%s] select_drive(): wdx identifier '%s' references unknown disk\n",
                            PROGRAM_NAME, wdx_id);
        ErrnoReturn (EINVAL, false);
      }
      else
      if (drive_partition && part_index >= number_of_disk_partitions (disk_index))
      {
        fprintf (stderr, "\n[%s] select_drive(): wdx identifier '%s' has invalid partition id\n"
                            PROGRAM_NAME, wdx_id);
        ErrnoReturn (EINVAL, false);
      }

      select_diskpartition (index_nth_local_disk (disk_index), part_index, NULL);

      if (sel_part != INVALID_PART
       && GetMountPoint (physical_disk[sel_disk].part[sel_part].volume, &sel_mount_point, false))
      {
        if (drive_partition && mount_point != NULL)
         *mount_point = sel_mount_point;
      }
      return (true);
    }

    /* CD/DVD/BD drives? */
    else
    {
      wdx2cdrom (wdx_id, &disk_index);  /*Must succeed*/

      /* Invalid disk? */
      if (disk_index >= number_of_optical_disks ())
      {
        fprintf (stderr, "\n[%s] select_drive(): wdx identifier '%s' references unknown disk\n",
                             PROGRAM_NAME, wdx_id);
        ErrnoReturn (EINVAL, false);
      }

      select_disk (index_nth_optical_disk (disk_index), NULL);

      if (GetMountPoint (physical_disk[sel_disk].part[0].volume, &sel_mount_point, false))
      {
        if (mount_point != NULL)
         *mount_point = sel_mount_point;
      }
      return (true);
    }
  }
  else
  {
    fprintf (stderr, "\n[%s] select_drive(): no drives detected.\n"
                         PROGRAM_NAME);
    ErrnoReturn (EINVAL, false);
  }
}


extern bool select_drive_by_devicename (TCHAR const nt_device_name [], TCHAR const **mount_point)
{
  drives_reset_selection ();
  if (mount_point != NULL)
   *mount_point = NULL;

  /* (A) Physical disks: */
  if (physical_disk == NULL)
  {
    fprintf (stderr, "\n[%s] select_drive(): no drives detected.\n"
             PROGRAM_NAME);
    ErrnoReturn (EINVAL, false);
  }

  { uint32_t
      d;

    for (d=0; physical_disk[d].type != DTYPE_INVALID; ++d)
      /* Harddisk? */
      if (IsLocalDisk (physical_disk[d].type))
      {
        /* (1) Physical drive? */
        if (EqualPhysicalDrive (nt_device_name, physical_disk[d].device_path))
        {
          sel_nt_devname  = true;
          select_disk (d, NULL);
          return (true);
        }

        /* (2) Logical volume? */
        if (physical_disk[d].part_count > 0)
        { uint32_t
            p;

          for (p=0; p < physical_disk[d].part_count; ++p)
            if (physical_disk[d].part[p].harddisk_volume != NULL)
              /* L"\Device\HarddiskVolume1"? */
              if (EqualHarddiskVolume (nt_device_name, physical_disk[d].part[p].harddisk_volume))
              {
                sel_nt_devname  = true;
                select_diskpartition (d, p, physical_disk[d].part[p].volume->mount_point[0]);
                if (mount_point != NULL)
                 *mount_point = sel_mount_point;
                return (true);
              }
        }
      }
      else
      /* CD/DVD/BD drives? */
      if (physical_disk[d].objmgr_path != NULL)
      {
        /* (3) Optical drive? */
        if (EqualCdromDrive (nt_device_name, physical_disk[d].objmgr_path))
        {
          sel_nt_devname  = true;
          select_disk (d, physical_disk[d].part[0].volume->mount_point[0]);
          if (mount_point != NULL)
           *mount_point = sel_mount_point;
          return (true);
        }
      }
  }

  /* (B) Logical volumes: */
  if (logical_volume != NULL)
  { uint32_t
      v;

    /* We do it this way, because there may be logical volumes with no
       with no associated `PhysicalDisk' entry, e.g. Floppy disk drives. */
    for (v=0; logical_volume[v].valid; ++v)
      if (EqualLogicalVolumeGuid (nt_device_name, logical_volume[v].volume_guid))
      {
        sel_nt_devname  = true;
        select_volume (v, logical_volume[v].mount_point[0]);
        if (mount_point != NULL)
         *mount_point = sel_mount_point;
        return (true);
      }
  }

  return (false);
}


#if 0  /*DELME*/
extern bool select_drive_by_mountpoint (TCHAR const *mount_point)
{ bool
    found = false;

  drives_reset_selection ();

  if (physical_disk == NULL)
  {
    fprintf (stderr, "\n[%s] select_drive_letter(): no drives detected.\n"
                         PROGRAM_NAME);
    ErrnoReturn (EINVAL, false);
  }

  { uint32_t
      d;
    TCHAR const
     *curr_mount_point;
    bool
      is_drive_letter = IsDriveLetter (mount_point, true, NULL, NULL);

    for (d=0; physical_disk[d].type != DTYPE_INVALID; ++d)
      /* Harddisk? */
      if (IsLocalDisk (physical_disk[d].type))
      {
        if (physical_disk[d].part_count > 0)
        { uint32_t
            p;

          for (p=0; p < physical_disk[d].part_count; ++p)
          { size_t
              i;

            for (i=0; ; ++i)
            {
              curr_mount_point = physical_disk[d].part[p].volume->mount_point[i];
              if (not valid_mount_point (curr_mount_point))
                break;

              if (is_drive_letter)
                found = IsSameDriveLetter (*mount_point, curr_mount_point);
              else
                found = PathEquals (mount_point, curr_mount_point);

              if (found)
              {
                select_diskpartition (d, p, physical_disk[d].part[p].volume->mount_point[i]);
                return (true);
              }
            }
          }
        }
      }
      else
      /* CD/DVD/BD drives? */
      { size_t
          i;

        for (i=0; ; ++i)
        {
          curr_mount_point = physical_disk[d].part[0].volume->mount_point[i];
          if (not valid_mount_point (curr_mount_point))
            break;

          if (is_drive_letter)
            found = IsSameDriveLetter (*mount_point, curr_mount_point);
          else
            found = PathEquals (mount_point, curr_mount_point);

          if (found)
          {
            select_disk (d, physical_disk[d].part[0].volume->mount_point[i]);
            return (true);
          }
        }
      }
  }

  return (found);
}
#endif


extern bool select_drive_by_mountpoint (TCHAR const *mount_point)
{ bool
    found = false;

  drives_reset_selection ();

  if (logical_volume == NULL)
  {
    fprintf (stderr, "\n[%s] select_drive_letter(): no drives detected.\n"
                         PROGRAM_NAME);
    ErrnoReturn (EINVAL, false);
  }

  { bool
      is_drive_letter = IsDriveLetter (mount_point, true, NULL, NULL);
    TCHAR const
     *curr_mount_point;
    uint32_t
      v, i;

    for (v=0; logical_volume[v].valid; ++v)
    {
      for (i=0; ; ++i)
      {
        curr_mount_point = logical_volume[v].mount_point[i];
        if (not valid_mount_point (curr_mount_point))
          break;

        if (is_drive_letter)
          found = IsSameDriveLetter (*mount_point, curr_mount_point);
        else
          found = PathEquals (mount_point, curr_mount_point);

        if (found)
        {
          select_volume (v, curr_mount_point);
          return (true);
        }
      }
    }
  }

  return (found);
}


extern bool select_drive_by_letter (TCHAR const *drive_letter)
{
  return (select_drive_by_mountpoint (drive_letter));
}


extern void get_selected_diskpartition (uint32_t *disk_index, uint32_t *part_index)
{
 *disk_index = sel_disk;
 *part_index = sel_part;
}


extern bool get_selected_wdxid (wdx_t wdx_id)
{
  if (sel_disk==INVALID_DISK && sel_part==INVALID_PART)
  {
    wdx_id [0] = '\0';
    return (false);
  }

  if (drives_optdisk_selected ())
    return (cdrom2wdx (sel_disk, wdx_id));
  else
  if (drives_partition_selected ())
    return (diskpart2wdx (sel_disk, sel_part, wdx_id));
  else
    return (disk2wdx (sel_disk, wdx_id));
}


extern enum EDriveType get_drive_type (void)
{
  if (sel_disk == INVALID_DISK)
  {
    /* Removable drive selected via volume GUID? */
    if (sel_volume != INVALID_VOLUME)
      return (DTYPE_REMOVABLE);
    else
      return (DTYPE_INVALID);
  }

  return (physical_disk[sel_disk].type);
}


/* Dismount drives: */
static bool
  open_file_handles; /*= false; */

static bool EnumFileHandlesCallback (ULONG ProcessId, HANDLE Handle, UCHAR ObjectTypeNumber, LPCWSTR FileName)
{
  (void)ProcessId; (void)Handle; (void)ObjectTypeNumber; (void)FileName;
/*fwprintf (stdout, L"%5d  0x%8lX[%3d]  %s\n", (int)ProcessId, (int)Handle, (int)ObjectTypeNumber, FileName);*/

  /* Search selected drive for open files: */
  { DWORD
      attr = GetFileAttributes (FileName);

    if (GetLastError() != ERROR_FILE_NOT_FOUND
        && attr != INVALID_FILE_ATTRIBUTES
        && (attr & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY)
    {
      /* Single drive selected? */
      if (drives_partition_selected ())
      {
        if (IsSameDrive (sel_mount_point, FileName))
        {
          open_file_handles = true;
          return (ENUM_HANDLES_STOP);
        }
      }
      /* Physical disk selected? */
      else
      if (drives_physdisk_selected ())
      { uint32_t
          p;

        for (p=0; p < physical_disk[sel_disk].part_count; ++p)
          if (IsSameDrive (physical_disk[sel_disk].part[p].volume->mount_point[0], FileName))
          {
            open_file_handles = true;
            return (ENUM_HANDLES_STOP);
          }
      }
      else
      {
        open_file_handles = false;
        return (ENUM_HANDLES_STOP);
      }
    }
  }

  return (ENUM_HANDLES_CONTINUE);
}


static size_t dismount_affected_drives (void)
{ size_t
    drives = 0;

  if (drives_partition_selected ())
  {
    if (IsDriveLetter (sel_mount_point, false, NULL, NULL))
      ++drives;
  }
  else
  { uint32_t
      p, m;

    for (p=0; p < physical_disk[sel_disk].part_count; ++p)
      for (m=0; physical_disk[sel_disk].part[p].volume->mount_point[m][0] != TEXT('\0'); ++m)
        if (IsDriveLetter (physical_disk[sel_disk].part[p].volume->mount_point[m], false, NULL, NULL))
          ++drives;
  }

  return (drives);
}


/* Include source for "Sub-components": */
#include "physical_disks.c"
#include "logical_volumes.c"
#include "wdx_identifer.c"


/* See "Setting Page file"
   http://forums.codeguru.com/showthread.php?512267-Setting-Page-file#post_2014617
*/
static bool IsPagefileDrive (TCHAR const drive_letter [])
{ HKEY
    hKey;
  DWORD
    type,
    cbData;
  TCHAR *
    value;

  if (RegOpenKeyEx (HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management"),
                    0, KEY_READ, &hKey) != ERROR_SUCCESS)
    return (false);

  if (RegQueryValueEx (hKey, TEXT("PagingFiles"), NULL, &type, NULL, &cbData) != ERROR_SUCCESS)
    goto QueryTypeFailed;

  if (type != REG_MULTI_SZ)
    goto IncorrectValueType;

  value = (TCHAR *)MemAlloc (cbData+1, sizeof(*value));

  if (RegQueryValueEx (hKey, TEXT("PagingFiles"), NULL, NULL, (LPBYTE)value, &cbData) != ERROR_SUCCESS)
    goto QueryValueFailed;

  RegCloseKey(hKey);

  { bool
      is_pagefile_drive;
    size_t
      i;

    for (i=0; value[i]!=TEXT('\0') || value[i+1]!=TEXT('\0'); ++i)
      if (value[i] == TEXT('\0'))
        value[i] = TEXT(' ');

    is_pagefile_drive = _tcsistr (value, drive_letter) != NULL;

    Free (value);
    return (is_pagefile_drive);
  }

QueryValueFailed:
  Free (value);
IncorrectValueType:
QueryTypeFailed:
  RegCloseKey(hKey);
  return (false);
}




/* See "Lock entire disk / physicaldrive / FSCTL_LOCK_VOLUME"
   http://www.techtalkz.com/microsoft-device-drivers/250612-lock-entire-disk-physicaldrive-fsctl_lock_volume.html
*/
extern bool dismount_selected_volumes (struct VolumeDesc const *vol, TCHAR const display_name [],
                                       bool force_wdx_logdrive, bool const read_only,
                                       struct DismountStatistics *statistics)
{ TCHAR
    volume_name [MAX_PATH + 1];
  bool
    warn_system_drive = false;
  TCHAR
    windd_drive_letter = TEXT('\0');

  assert (statistics != NULL);
  memset (statistics, 0, sizeof(*statistics));
  statistics->continue_op = true;

  /* Determine drive letter of the currently executing process: */
  { TCHAR
      executable_filename [MAX_PATH + 1], *colon;

    if (GetModuleFileName (NULL, executable_filename, MAX_PATH))
      if (IsDriveLetter (executable_filename, false, &colon, NULL))
        windd_drive_letter = *(colon-1);
  }

  /* FSCTL_LOCK_VOLUME can (seemingly) always be used, even for partitions
     with no associated drive letters; ignore affected drives check. */
  /*
  if (dismount_affected_drives () == 0)
    return (true);
  */

  /* Single drive selected && is "%SystemDrive%"? */
  if (drives_partition_selected ())
  {
    if (is_system_drive ())
      warn_system_drive = true;
  }
  else
  /* Physical disk selected && contains "%SystemDrive%"? */
  if (drives_physdisk_selected () && disk_contains_system_drive ())
    warn_system_drive = true;

  { char
      key;

    /* Any open handles on the selected drive? */
    if (not force_operations && not warn_system_drive
        #ifndef EXPERIMENTAL_OPEN_HANDLES_CHECK
     && false
        #endif
       )
    { size_t
        handle_count;

      open_file_handles = false;
      EnumFileHandles (&EnumFileHandlesCallback, &handle_count);

      if (open_file_handles)
      {
        _ftprintf (stdout,
TEXT("%s cannot run because the volume is in use by other processes. %s may run\n")
TEXT("if this volume is dismounted first. ALL OPEN HANDLES TO THIS VOLUME WOULD THEN\n")
TEXT("BE INVALID. Would you like to force a dismount on this volume? (Y/N) "),
          TEXT(PROGRAM_NAME), TEXT(PROGRAM_NAME));

        if (not force_operations)
          key = PromptUserYesNo ();
        else
          key = 'Y';

        if (not IsFileRedirected (stdin))
        {
          _ftprintf (stdout, TEXT("%c\n\n"), key);
          FlushStdout ();
        }
        if (key == 'N')
        {
          statistics->continue_op = false;
          return (false);
        }
      /*// We shouldn't set this flag variable unless the user specified it explicitly!
        else
          force_operations = true;*/
      }
    }

    /* Would this operation involve %SystemDrive%? */
    if (warn_system_drive && not read_only)
    {
      _ftprintf (stdout,
TEXT("The selected disk contains your %%SystemDrive%%. You may proceed, but please\n")
TEXT("be aware that all physical sectors within this volume will only be readable,\n")
TEXT("not writable. Would you like to continue nonetheless? (Y/N) "));

      if (not force_operations)
        key = PromptUserYesNo ();
      else
        key = 'Y';

      if (not IsFileRedirected (stdin))
      {
        _ftprintf (stdout, TEXT("%c\n\n"), key);
        FlushStdout ();
      }
      if (key == 'N')
      {
        statistics->continue_op = false;
        return (false);
      }
    }
  }

  /* Dismount drives: */
  { TCHAR const *
      cur_mount_point = display_name;
    uint32_t
      first_part, last_part,
      step, p, m = (uint32_t)-1;
    bool
      first_iteration = true,
      is_windows_part = true,
      dismount_multipe = false;

    /* First and last selected partition? */
    first_part = drives_partition_selected ()? sel_part : 0;
    if (drives_physdisk_selected ()
        #ifdef READONLY_DONT_DISMOUNT
        && not read_only  /* Don't dismount drives if we only want to read data? */
        #endif
       )  /* Physical drive or logical volume? */
    {
      last_part = 1 + min (sel_part, physical_disk[sel_disk].part_count-1);
      if (sel_part == INVALID_PART || force_wdx_logdrive)
        --first_part;                 /* Lock `vol' as well as selected partition */
      dismount_multipe = true;
    }
    else
  /*if (sel_nt_devname || drives_optdisk_selected ())*/
      last_part = first_part+1;       /* Simply lock/dismount passed `vol'! */

    /* Lock and dismount: */
    step = (last_part != first_part+1)? 2 : 1;  /* Step 2: lock directory mount points
                                                   Step 1: lock drive letters */
LockDriveLetters:
    for (p=first_part; p != last_part; ++p, m=(uint32_t)-1)
    {
      if (sel_disk!=INVALID_DISK && IsExtendedPartitionIndex (sel_disk, p))
        continue;  /* Skip extended partition! */

      do {
        if (first_iteration)
          _tcscpy (volume_name, cur_mount_point);
        else
        {
          ++m;
          if (m >= 1)
            break;  /* No need to lock each partition more than once! */

          /* If available, lock `mount_point'; otherwise lock "\Device\HarddiskVolumeX" */
          cur_mount_point = physical_disk[sel_disk].part[p].volume->mount_point[m];
          is_windows_part = physical_disk[sel_disk].part[p].is_windows_part;

          if (valid_mount_point (cur_mount_point))
          { int
              path_prefix = PathPrefixW (cur_mount_point);

            _tcscpy (volume_name, UNC_DISPARSE_PREFIX);
            _tcscat (volume_name, cur_mount_point+path_prefix);

            if (physical_disk[sel_disk].part[p].locked_dismounted)
              volume_name [0] = TEXT('\0');
            else
            {
              if (step == 2)
              {
                if (not IsDriveLetter (volume_name, true, NULL, NULL)
                 && select_drive_by_mountpoint (volume_name))
                {
                  cur_mount_point = physical_disk[sel_disk].part[p].harddisk_volume;
                  _tcscpy (volume_name, cur_mount_point);
                  NormalizeNTDevicePath (volume_name);
                }
                else
                  volume_name [0] = TEXT('\0');
              }
              else
              {
                if (not IsDriveLetter (volume_name, true, NULL, NULL))
                  volume_name [0] = TEXT('\0');
              }
            }
          }
          else
          if (step==1 && m==0)
          {
            cur_mount_point = physical_disk[sel_disk].part[p].harddisk_volume;

            if ((valid_mount_point (cur_mount_point) || NTDeviceName (cur_mount_point))
                && not physical_disk[sel_disk].part[p].locked_dismounted)
            {
              _tcscpy (volume_name, cur_mount_point);
              NormalizeNTDevicePath (volume_name);
            }
            else
              volume_name [0] = TEXT('\0');
          }
          else
            break;  /* List of mount points is terminated by an empty string L""! */

          RemoveTrailingBackslash (volume_name);
        }

        if (volume_name [0] != TEXT('\0'))
        { TCHAR *
            colon;

          _ftprintf (stdout, TEXT("%-39s ... "), volume_name);
          ++statistics->total_count;

          /* (1) Can't lock the system drive: */
          if (warn_system_drive && p!=INVALID_PART
           && physical_disk[sel_disk].part[p].volume->is_system_drive)  /*is_system_drive ()*/
          {
            _ftprintf (stdout, TEXT("failed  (system drive)\n"));
            statistics->skipped_sysdrive = true;
          }
          /* (2) Can't lock if there is a pagefile: */
          else
          if (IsDriveLetter (volume_name, false, &colon, NULL)
              && IsPagefileDrive (colon-1))
          {
            _ftprintf (stdout, TEXT("failed  (has pagefile.sys)\n"));
            ++statistics->skipped_pagefile;
          }
          /* (3) No need to lock Non-Windows partitions: */
          else
          if (not is_windows_part)
          {
            _ftprintf (stdout, TEXT("skipped (non-Windows partition)\n"));
            ++statistics->skipped_non_windows;
          }
          else
          /* (4) Try to lock and dismount: */
          { bool
              locked;             /* Volume lock successful? */
            bool
              dismounted;         /* Volume also dismounted? */

            if (not first_iteration)
            {
              if ((vol=OpenVolumeRW (-1, volume_name)) == NULL)
                return (false);

              /* Store partition HANDLE; at the end of execution it will be released through `close();' */
              statistics->volume_handles = (HANDLE *)MemRealloc (statistics->volume_handles,
                                                                 statistics->handle_count + 2, sizeof(HANDLE));
              statistics->volume_handles [statistics->handle_count]   = GetVolumeHandle (vol);
              statistics->volume_handles [++statistics->handle_count] = NULL;
            }

            /* volume_name = L"\\\\.\\PHYSICALDRIVE" or L"/dev/wdX"? */
            if (PhysicalDriveOpened (vol))
              _ftprintf (stdout, TEXT("OK\n"));  /* No need to do anything! */
            else
            {
              /* (4) Shouldn't lock the drive from which `windd' is executing: */
              TCHAR const *dismount_drive_letter = _tcsrchr (volume_name, TEXT(':'));

              if (dismount_drive_letter && IsSameDriveLetter (windd_drive_letter, dismount_drive_letter-1))
              {
                textcolor (LIGHTRED);
                _ftprintf (stdout, TEXT("\nWarning:"));
                textcolor (DEFAULT_COLOR);
                _ftprintf (stdout,
TEXT(" you're executing %s from a drive that will now be dismounted;\n")
TEXT("this is not recommended. While the operation you initiated may be able to\n")
TEXT("run to completion, you'll probably receive the following exception as soon\n")
TEXT("as control is returned back to the operating system:\n"),
                  TEXT(PROGRAM_NAME));
                textcolor (WHITE);
                _ftprintf (stdout,
TEXT("Faulting module ntdll.dll: \"In-page I/O error c000009c – code c0000006\"\n"));
                textcolor (DEFAULT_COLOR);
                _ftprintf (stdout,
TEXT("Would you like to continue nonetheless? (Y/N) "));
                { char
                    key = PromptUserYesNo ();
                  if (not IsFileRedirected (stdin))
                  {
                    _ftprintf (stdout, TEXT("%c\n\n"), key);
                    FlushStdout ();
                  }
                  if (key == 'N')
                  {
                    statistics->continue_op = false;
                    return (false);
                  }
                }
                _ftprintf (stdout, TEXT("%-39s ... "), volume_name);
              }

              /* Lock and dismount: */
              DismountVolume (vol, &locked, &dismounted);
              if (not locked)
                ++statistics->failed_to_lock;
              if (not dismounted)
                ++statistics->failed_to_dismount;

              /* Success/failed messages: */
              if (locked && dismounted)
              {
                _ftprintf (stdout, TEXT("locked & dismounted\n"));
                if (sel_disk != INVALID_DISK)
                  physical_disk[sel_disk].part[p].locked_dismounted = true;
              }
              else
              if (locked || dismounted)
              {
                _ftprintf (stdout, TEXT("dismounted only\n"));
                if (sel_disk != INVALID_DISK)
                  physical_disk[sel_disk].part[p].locked_dismounted = true;
              }
              else
              {
                _ftprintf (stdout, TEXT("failed (read only)\n"));
                ++statistics->failed_count;
              }
            }
            FlushStdout ();

            /* Free `VolumeDesc' structure, but keep partition HANDLE: */
            if (not first_iteration)
              CloseVolumeEx (&vol, false);
          }
        }
      } while (not sel_nt_devname && not first_iteration
            && physical_disk[sel_disk].part[p].volume->mount_point[m][0] != TEXT('\0'));

      first_iteration = false;
    }

    if (--step == 1 && dismount_multipe)
    {
      ++first_part;
      goto LockDriveLetters;
    }
  }

  return (true);
}
