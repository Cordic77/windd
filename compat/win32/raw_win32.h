#ifndef WINDD_RAW_H_
#define WINDD_RAW_H_

/* NOTE:
   Currently, this module is only used for local storage devices, in order
   to gain (read-only) access to the MBR and GPT on-disk structures.

   All read() and write() calls as performed by `dd.c' are actually
   implemented in `posix_win32.c'!
*/

#include <winioctl.h>             /* STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR */

/* */
#define LOGICAL_SECTOR_SIZE  512

/* */
enum
  {
    C_ASCII = 01,

    C_EBCDIC = 02,
    C_IBM = 04,
    C_BLOCK = 010,
    C_UNBLOCK = 020,
    C_LCASE = 040,
    C_UCASE = 0100,
    C_SWAB = 0200,
    C_NOERROR = 0400,
    C_NOTRUNC = 01000,
    C_SYNC = 02000,

    /* Use separate input and output buffers, and combine partial
       input blocks. */
    C_TWOBUFS = 04000,

    C_NOCREAT = 010000,
    C_EXCL = 020000,
    C_FDATASYNC = 040000,
    C_FSYNC = 0100000,

    C_SPARSE = 0200000
  };

/* */
struct VolumeDesc;

/* */
#define IsValidHandle(handle) (handle!=NULL && handle!=INVALID_HANDLE_VALUE)

/* */
extern struct VolumeDesc *
  OpenVolumeEx (int desired_fd, TCHAR const device_id [], DWORD dwDesiredAccess);
extern struct VolumeDesc *
  OpenVolumeRW (int desired_fd, TCHAR const device_id []);
extern struct VolumeDesc *
  OpenVolumeRO (int desired_fd, TCHAR const device_id []);
extern struct VolumeDesc *
  OpenVolumeInfo (int desired_fd, TCHAR const device_id[]);
extern bool
  PhysicalDriveOpened (struct VolumeDesc const *vol);
extern bool
  LockCdromDriveEx (HANDLE hCdrom, BOOL fLock);
extern bool
  LockCdromDrive (struct VolumeDesc *vol);
extern bool
  UnlockCdromDrive (struct VolumeDesc *vol);
extern bool
  CloseVolume (struct VolumeDesc const **vol);
extern bool
  CloseVolumeEx (struct VolumeDesc const **vol, bool close_handle);
extern HANDLE
  GetVolumeHandle (struct VolumeDesc const *vol);
extern DWORD
  GetVolumeDesiredAccess (struct VolumeDesc const *vol);
extern DWORD
  GetVolumeFlagsAndAttributes (struct VolumeDesc const *vol);
extern LONGLONG
  GetFilePointerEx (HANDLE handle);
extern bool
  DismountVolume (struct VolumeDesc const *vol, bool *locked, bool *dismounted);
extern ssize_t
  ReadSector (struct VolumeDesc const *vol, void *buf, off_t start_sector, size_t sector_count);
extern ssize_t
  WriteSector (struct VolumeDesc const *vol, void *buf, off_t start_sector, size_t sector_count);

/* */
extern bool
  GetDiskGeometry (struct VolumeDesc const *vol,
                   DISK_GEOMETRY *disk_geom);
extern bool
  GetVolumeSize (struct VolumeDesc const *vol,
                 GET_LENGTH_INFORMATION *length_info);
extern bool
  GetVolumeAlignment (struct VolumeDesc const *vol,
                      STORAGE_ACCESS_ALIGNMENT_DESCRIPTOR *align_desc);
extern int
  GetVolumeExtents (struct VolumeDesc const *vol, VOLUME_DISK_EXTENTS **volume_disk_extents);

extern DWORD
  PhysicalSectorSize (struct VolumeDesc const *vol);
extern DWORD
  PhysicalSectorSizeEx (TCHAR const path []);

#endif
