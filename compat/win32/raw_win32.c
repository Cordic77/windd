/* Displaying Volume Paths
// https://msdn.microsoft.com/en-us/library/windows/desktop/cc542456(v=vs.85).aspx

// Direct Drive Access Under Win32
// https://support.microsoft.com/en-us/kb/100027
// To open a physical hard drive for direct disk access (raw I/O) in a
// Win32-based application, use a device name of the form
// \\.\PhysicalDriveN
// where N is 0, 1, 2, and so forth, representing each of the physical
// drives in the system.
//
// To open a logical drive, direct access is of the form
// \\.\X:
// where X: is a hard-drive partition letter, floppy disk drive, or
// CD-ROM drive.

// MORE INFORMATION
// You can open a physical or logical drive using the CreateFile() application
// programming interface (API) with these device names provided that you have
// the appropriate access rights to the drive (that is, you must be an Administrator).
//
// You must use both the CreateFile() FILE_SHARE_READ and FILE_SHARE_WRITE flags
// to gain access to the drive.
//
// Once the logical or physical drive has been opened, you can then perform
// direct I/O to the data on the entire drive. When performing direct disk I/O,
// you must seek, read, and write IN MULTIPLES OF SECTOR SIZES of the device
// and on sector boundaries.
//
// Call DeviceIoControl() using IOCTL_DISK_GET_DRIVE_GEOMETRY to get the bytes
// per sector, number of sectors, sectors per track, and so forth, so that you
// can compute the size of the buffer that you will need.
//
// Note that a Win32-based application cannot open a file by using internal
// Windows NT object names; for example, attempting to open a CD-ROM drive by
// opening
// \Device\CdRom0
// does not work because this is not a valid Win32 device name. An application
// can use the QueryDosDevice() API to get a list of all valid Win32 device
// names and see the mapping between a particular Win32 device name and an
// internal Windows NT object name.
//
// An application running at a sufficient privilege level can define, redefine,
// or delete Win32 device mappings by calling the DefineDosDevice() API.
*/
#include "win32utf16.h"           /* switch to UTF-16 in this file! */

#include <config.h>

#include "raw_win32.h"
#include "path_win32.h"           /* GetVolumePath() */
#include "objmgr_win32.h"         /* IsPhysicalDrive() */
#include "diskmgmt\physical_disks.h"  /* drives_optdisk_selected() */


/* */
struct VolumeDesc
{
  int                                   posix_fd;
  DWORD                                 dwDesiredAccess;
  DWORD                                 dwFlagsAndAttributes;
  bool                                  is_physical_drive;

  DISK_GEOMETRY                        *disk_geom;
  GET_LENGTH_INFORMATION               *length_info;
  STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR  *align_desc;
};


/* */
static bool GetVolumeInfo (struct VolumeDesc *vol);


/*===  _WIN32 raw disk access  ===*/
extern struct VolumeDesc *OpenVolumeEx (int desired_fd, TCHAR const device_id [], DWORD dwDesiredAccess)
{ struct VolumeDesc *
    vol = (struct VolumeDesc *)MemAlloc (1, sizeof(*vol));

  memset (vol, 0, sizeof(*vol));

  vol->dwFlagsAndAttributes =
                      FILE_FLAG_NO_BUFFERING   /* disable caching, but allow */
                    | FILE_FLAG_RANDOM_ACCESS; /* sectors to be accessed randomly */
  vol->dwDesiredAccess = dwDesiredAccess;


  { HANDLE handle = CreateFile (
                      device_id,               /* file to open */
                      vol->dwDesiredAccess,    /* n.b.: specify only GENERIC_READ, even for write access */
                      FILE_SHARE_READ | FILE_SHARE_WRITE,  /* shared for read and write access */
                      NULL,                    /* default security */
                      OPEN_EXISTING,           /* existing file only */
                      vol->dwFlagsAndAttributes,
                      NULL);                   /* no attr. template */

    if (not IsValidHandle (handle))
    {
      Free (vol);
      return (NULL);
    }

    vol->is_physical_drive = (IsPhysicalDrive (device_id) > 0);

    /* Reserve a file descriptor representing this volume HANDLE: */
    if (desired_fd >= 0)
    {
      vol->posix_fd = newfd (handle);

      if (vol->posix_fd != desired_fd)
      {
        if (dup2 (vol->posix_fd, desired_fd) < 0)
        {
          Free (vol);
          return (NULL);
        }
        remfd (vol->posix_fd);
        vol->posix_fd = desired_fd;
      }
    }
    else
      vol->posix_fd = newfd_ex (handle, true);  /* 65533, 65532, ... */

    /* Volume properties? (Retrieve only, if we'll actually be reading/writing sectors.) */
  /*if (dwDesiredAccess != 0)*/
    {
      (void)GetVolumeInfo (vol);  /* Ignore any errors! */
    }
  }

  return (vol);
}


extern struct VolumeDesc *OpenVolumeRW (int desired_fd, TCHAR const device_id [])
{
  /* CreateFile: direct write operation to raw disk “Access is denied” - Vista, Win7
http://stackoverflow.com/questions/8694713/createfile-direct-write-operation-to-raw-disk-access-is-denied-vista-win7#answer-8808596
     Contrary to the answer provided by wallyk, WriteFile() fails unluess
     both GENERIC_READ and GENERIC_WRITE are requested:
  */
  return (OpenVolumeEx (desired_fd, device_id, GENERIC_READ | GENERIC_WRITE));
}


extern struct VolumeDesc *OpenVolumeRO (int desired_fd, TCHAR const device_id [])
{
  return (OpenVolumeEx (desired_fd, device_id, GENERIC_READ));
}


extern struct VolumeDesc *OpenVolumeInfo (int desired_fd, TCHAR const device_id[])
{
  return (OpenVolumeEx (desired_fd, device_id, 0));
}


extern bool PhysicalDriveOpened (struct VolumeDesc const *vol)
{
  if (vol==NULL)
    ErrnoReturn (EINVAL, false);

  return (vol->is_physical_drive);
}


/* See "How Win32-Based Applications Read CD-ROM Sectors in Windows NT"
   https://support.microsoft.com/en-us/kb/138434
*/
#ifndef IOCTL_CDROM_MEDIA_REMOVAL
#define IOCTL_CDROM_MEDIA_REMOVAL  0x24804
#endif

extern bool LockCdromDriveEx (HANDLE hCdrom, BOOL fLock)
{ PREVENT_MEDIA_REMOVAL
    media_removal = {0};
  DWORD
    dwBytesReturned;

  media_removal.PreventMediaRemoval = fLock;

  if (not DeviceIoControl (hCdrom,              /* cd rom drive */
        IOCTL_CDROM_MEDIA_REMOVAL,              /* operation to perform */
        &media_removal, sizeof(media_removal),  /* input buffer */
        NULL, 0,                                /* no output buffer */
        &dwBytesReturned,                       /* unused */
        (LPOVERLAPPED)NULL))                    /* no overlapped I/O */
    return (false);

  return (true);
}

extern bool LockCdromDrive (struct VolumeDesc *vol)
{ HANDLE
    vol_handle;

  if ((vol_handle=GetVolumeHandle (vol)) != INVALID_HANDLE_VALUE)
    return (LockCdromDriveEx, TRUE);

  return (false);
}

extern bool UnlockCdromDrive (struct VolumeDesc *vol)
{ HANDLE
    vol_handle;

  if ((vol_handle=GetVolumeHandle (vol)) != INVALID_HANDLE_VALUE)
    return (LockCdromDriveEx, TRUE);

  return (false);
}


extern bool CloseVolume (struct VolumeDesc const **vol)
{
  return (CloseVolumeEx (vol, true));
}


extern bool CloseVolumeEx (struct VolumeDesc const **vol, bool close_handle)
{
  if (vol==NULL || *vol==NULL)
    ErrnoReturn (EINVAL, false);

  { int fd = (*vol)->posix_fd;

   *vol = NULL;

    if (not close_handle)
    {
      remfd (fd);
      Free (*vol);
      return (true);
    }

    Free (*vol);
    return (close (fd) == 0);
  }
}


extern bool SetVolumeHandle (struct VolumeDesc *vol, HANDLE handle)
{
  if (vol == NULL)
    ErrnoReturn (EINVAL, false);

  vol->posix_fd = handle2fd (handle);
  return (true);
}

extern HANDLE GetVolumeHandle (struct VolumeDesc const *vol)
{ HANDLE
    vol_handle;

  if (vol == NULL)
    ErrnoReturn (EINVAL, false);

  vol_handle = fd2handle (vol->posix_fd);

  if (not IsValidHandle (vol_handle))
    ErrnoReturn (EBADF, INVALID_HANDLE_VALUE);

  return (vol_handle);
}


extern DWORD GetVolumeDesiredAccess(struct VolumeDesc const *vol)
{
  return (vol->dwDesiredAccess);
}


extern DWORD GetVolumeFlagsAndAttributes (struct VolumeDesc const *vol)
{
  return (vol->dwFlagsAndAttributes);
}


/* Inspired by Ext3FSD's Ext2Mgr:enumDisk.cpp:Ext2DismountVolume() function */
extern bool DismountVolume (struct VolumeDesc const *vol, bool *locked, bool *dismounted)
{
  if (locked != NULL)
   *locked = false;
  if (dismounted != NULL)
   *dismounted = false;

  if (vol->is_physical_drive)
    return (true);  /* Nothing to do */

  { HANDLE
      vol_handle;
    DWORD
      bytes_returned;

    if ((vol_handle=GetVolumeHandle (vol)) == INVALID_HANDLE_VALUE)
      return (false);

    /* The following pages describe the preconditions listed below:
       * FSCTL_LOCK_VOLUME control code
       * FSCTL_DISMOUNT_VOLUME control code
       The hDevice handle passed to DeviceIoControl must be a handle to a volume,
       opened for direct access. To retrieve this handle, call CreateFile with the
       lpFileName parameter set to a string of the following form: \\.\X:
       where X is a hard-drive partition letter, floppy disk drive, or CD-ROM drive.
       The application must also specify the FILE_SHARE_READ and FILE_SHARE_WRITE
       flags in the dwShareMode parameter of CreateFile.
    */

    /* Remarks:
       * If the specified volume is a system volume or contains a page file, the operation fails.
       * If there are any open files on the volume, this operation will fail.
       * Conversely, success of this operation indicates there are no open files.
    */
    if (DeviceIoControl (vol_handle, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytes_returned, NULL))
    {
      if (locked != NULL)
        *locked = true;
    }
    else
    {
      /* Ignore and try to dismount anyway. */
    }

    /* Note that without a successful lock operation, a dismounted volume may be
       remounted by any process at any time. This would not be an ideal state for
       performing a volume backup, for example.*/
    if (DeviceIoControl (vol_handle, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytes_returned, NULL))
    {
      if (dismounted != NULL)
       *dismounted = true;
    }
    else
    {
      SetWinErr ();
      return (false);
    }
  }

  return (true);
}


extern LONGLONG GetFilePointerEx (HANDLE handle)
{ LARGE_INTEGER
    new_off = {0},
    cur_off;

  if (not SetFilePointerEx (handle, new_off, &cur_off, FILE_CURRENT))
    return (-1LL);

  return (cur_off.QuadPart);
}


/* Right now, these two functions are not really used: */
extern ssize_t ReadSector (struct VolumeDesc const *vol, void *buf, off_t start_sector, size_t sector_count)
{
  off_t offset = (off_t)start_sector * LOGICAL_SECTOR_SIZE;
  off_t bytes  = (off_t)sector_count * LOGICAL_SECTOR_SIZE;

  (void)offset; (void)bytes;

  return (read (vol->posix_fd, buf, sector_count));
}


extern ssize_t WriteSector (struct VolumeDesc const *vol, void *buf, off_t start_sector, size_t sector_count)
{
  off_t offset = (off_t)start_sector * LOGICAL_SECTOR_SIZE;
  off_t bytes  = (off_t)sector_count * LOGICAL_SECTOR_SIZE;

  (void)offset; (void)bytes;

  return (write (vol->posix_fd, buf, sector_count));
}


/*===  Volume metadata  ===*/
static bool GetVolumeInfo (struct VolumeDesc *vol)
{ DWORD
    dwBytesReturned;
  HANDLE
    vol_handle;

  if ((vol_handle=GetVolumeHandle (vol)) == INVALID_HANDLE_VALUE)
    return (false);

  /* */
  { DISK_GEOMETRY
      disk_geom;

    if (DeviceIoControl (vol_handle,          /* device to be queried */
          IOCTL_DISK_GET_DRIVE_GEOMETRY,      /* operation to perform */
          NULL, 0,                            /* no input buffer */
          &disk_geom, sizeof(disk_geom),      /* output buffer */
          &dwBytesReturned,                   /* # bytes returned */
          (LPOVERLAPPED)NULL))                /* no overlapped I/O */
    {
      vol->disk_geom = (DISK_GEOMETRY *)MemAlloc (sizeof(disk_geom), 1);
      memcpy (vol->disk_geom, &disk_geom, sizeof(disk_geom));
    }
    else
      vol->disk_geom = NULL;
  }

  /* */
  { GET_LENGTH_INFORMATION
      length_info;

    if (DeviceIoControl (vol_handle,          /* device to be queried */
          IOCTL_DISK_GET_LENGTH_INFO,         /* operation to perform */
          NULL, 0,                            /* no input buffer */
          &length_info, sizeof(length_info),  /* output buffer */
          &dwBytesReturned,                   /* # bytes returned */
          (LPOVERLAPPED)NULL))                /* no overlapped I/O */
    {
      vol->length_info = (GET_LENGTH_INFORMATION *)MemAlloc (sizeof(length_info), 1);
      memcpy (vol->length_info, &length_info, sizeof(length_info));
    }
    else
      vol->length_info = NULL;
  }

  /* */
  { STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR
      align_desc;
    STORAGE_PROPERTY_QUERY
      query;

    ZeroMemory (&query, sizeof(query));
    query.QueryType  = PropertyStandardQuery;
    query.PropertyId = StorageAccessAlignmentProperty;

    if (DeviceIoControl (vol_handle,          /* device to be queried */
          IOCTL_STORAGE_QUERY_PROPERTY,       /* operation to perform */
          &query, sizeof(query),              /* input buffer */
          &align_desc, sizeof(align_desc),    /* output buffer */
          &dwBytesReturned,                   /* # bytes returned */
          (LPOVERLAPPED)NULL))                /* no overlapped I/O */
    {
      vol->align_desc = (STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR *)MemAlloc (sizeof(align_desc), 1);
      memcpy (vol->align_desc, &align_desc, sizeof(align_desc));
    }
    else
      vol->align_desc = NULL;
  }

  if (not vol->disk_geom && not vol->length_info && not vol->align_desc)
    return (false);

  return (true);
}


extern bool GetDiskGeometry (struct VolumeDesc const *vol,
                             DISK_GEOMETRY *disk_geom)
{
  if (vol == NULL || disk_geom == NULL)
    ErrnoReturn (EINVAL, false);

  if (not vol->disk_geom)
    return (false);

  memcpy (disk_geom, vol->disk_geom, sizeof(*vol->disk_geom));
  return (true);
}


extern bool GetVolumeSize (struct VolumeDesc const *vol,
                           GET_LENGTH_INFORMATION *length_info)
{
  if (vol == NULL || length_info == NULL)
    ErrnoReturn (EINVAL, false);

  if (not vol->length_info)
    return (false);

  #if 0
  return ((unsigned long long)
    vol->disk_geom->Cylinders.QuadPart *
    vol->disk_geom->TracksPerCylinder *
    vol->disk_geom->SectorsPerTrack *
    vol->disk_geom->BytesPerSector);
  #endif

  memcpy (length_info, vol->length_info, sizeof(*vol->length_info));
  return (true);
}


extern bool GetVolumeAlignment (struct VolumeDesc const *vol,
                                STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR *align_desc)
{
  if (vol == NULL || align_desc == NULL)
    ErrnoReturn (EINVAL, false);

  if (not vol->align_desc)
    return (false);

  memcpy (align_desc, vol->align_desc, sizeof(*vol->align_desc));
  return (true);
}


extern int GetVolumeExtents (struct VolumeDesc const *vol, VOLUME_DISK_EXTENTS **volume_disk_extents)
{ DWORD
    dwBytesReturned;
  HANDLE
    vol_handle;

  if ((vol_handle=GetVolumeHandle (vol)) == INVALID_HANDLE_VALUE)
    return (0);
  if (volume_disk_extents == NULL)
    ErrnoReturn (EINVAL, 0);

  /* */
  { int
      ret;
    VOLUME_DISK_EXTENTS
     *vde = NULL;
    size_t
      sizeof_vde = sizeof(*vde);

    vde = (VOLUME_DISK_EXTENTS *)MemAlloc (sizeof_vde, 1);
    memset (vde, 0, sizeof_vde);

TryAgain:
    if ((ret=DeviceIoControl (vol_handle,             /* device to be queried */
                IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, /* operation to perform */
                NULL, 0,                              /* no input buffer */
                vde, (DWORD)sizeof_vde,               /* output buffer */
                &dwBytesReturned,                     /* # bytes returned */
                (LPOVERLAPPED)NULL)) == 0)            /* no overlapped I/O */
    {
      if ((GetLastError() != ERROR_MORE_DATA
        && GetLastError() != ERROR_INSUFFICIENT_BUFFER) || sizeof_vde > sizeof(*vde))
        WinErrReturn (0);
      else
      {
        sizeof_vde = offsetof (VOLUME_DISK_EXTENTS, Extents [vde->NumberOfDiskExtents]);
        vde = (VOLUME_DISK_EXTENTS *)MemRealloc (vde, sizeof_vde, 1);
        memset (vde, 0, sizeof_vde);
        goto TryAgain;
      }
    }
    else
     *volume_disk_extents = vde;

    return (ret);
  }
}


extern DWORD PhysicalSectorSize (struct VolumeDesc const *vol)
{ STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR
    align_desc = {0};
  DISK_GEOMETRY
    disk_geom = {0};

  if (not drives_optdisk_selected () && GetVolumeAlignment (vol, &align_desc))
    return (align_desc.BytesPerPhysicalSector);
  else
  if (GetDiskGeometry (vol, &disk_geom))
    return (disk_geom.BytesPerSector);

  return (0);
}


extern DWORD PhysicalSectorSizeEx (TCHAR const path [])
{ static TCHAR             /* = TEXT("\\\\.\\PhysicalDrive0"); // Device path */
    devpath [MAX_PATH + 1];/* = TEXT("\\\\.\\C:");             // Volume device path */

  if (not GetVolumePath (path, devpath, _countof(devpath)))
    return (0);

  { struct VolumeDesc
     *vol;
    DWORD
      phys_sect_size;

    if ((vol=OpenVolumeInfo (-1, devpath)) == NULL)
      return (0);

    phys_sect_size = PhysicalSectorSize (vol);

    CloseVolume (&vol);
    return (phys_sect_size);
  }
}
