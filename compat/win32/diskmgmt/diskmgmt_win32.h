#ifndef WINDD_DISKMGMT_H_
#define WINDD_DISKMGMT_H_

/* See MSDN or `WinBase.h' */
enum EDriveType
{
  DTYPE_INVALID     = -1,
  DTYPE_UNKNOWN     = DRIVE_UNKNOWN,      /* DRIVE_UNKNOWN: The drive type cannot be determined. */
  DTYPE_NOT_MOUNTED = DRIVE_NO_ROOT_DIR,  /* The root path is invalid; for example, there is no volume mounted at the specified path. */
  DTYPE_REMOVABLE   = DRIVE_REMOVABLE,    /* The drive has removable media; for example, a floppy drive, thumb drive, or flash card reader. */
  DTYPE_FIXED       = DRIVE_FIXED,        /* The drive has fixed media; for example, a hard disk drive or flash drive. */
  DTYPE_REMOTE      = DRIVE_REMOTE,       /* The drive is a remote (network) drive. */
  DTYPE_CDROM       = DRIVE_CDROM,        /* The drive is a CD/DVD/BD drive. */
  DTYPE_RAMDISK     = DRIVE_RAMDISK,      /* The drive is a RAM disk. */
  /*windd - added*/
  DTYPE_FIXED_SSD   = 15,
  DTYPE_BIT_MASK    = 15,
  DTYPE_LOC_DISK    = 16,
  DTYPE_OPT_DISK    = 32
};

#define IsLocalDisk(drive_type)       (((drive_type)!=DTYPE_INVALID)? (((drive_type) & DTYPE_LOC_DISK) == DTYPE_LOC_DISK) : (exit(244), false))
#define IsSolidStateDrive(drive_type) (((drive_type)!=DTYPE_INVALID)? (((drive_type) & DTYPE_BIT_MASK) == DTYPE_FIXED_SSD) : (exit(244), false))
#define IsOpticalDisk(drive_type)     (((drive_type)!=DTYPE_INVALID)? (((drive_type) & DTYPE_OPT_DISK) == DTYPE_OPT_DISK) : (exit(244), false))

/* */
#define INT64_UNKNOWN     ((intmax_t)-1)

/* */
#define DEVFILE_ZERO      "/dev/zero"
#define DEVFILE_URANDOM   "/dev/urandom"
#define DEVFILE_RANDOM    "/dev/random"
#define DEVFILE_NULL      "/dev/null"

/* */
struct DismountStatistics
{
  size_t  total_count;            /* Total number of volumes we tried to lock */
  size_t  failed_count;           /* DismountVolume() was unsuccessful: volume is only readable */
  size_t  failed_to_lock;         /* Volume is only dismounted (but not locked) */
  size_t  failed_to_dismount;     /* Volume is locked but not dismounted (this should never happen) */

  bool    skipped_sysdrive;       /* Can't lock "%SystemDrive%", the Operation systems drive */
  size_t  skipped_pagefile;       /* Can't lock drives where a `pagefile.sys' is stored */
  size_t  skipped_non_windows;    /* No need to lock/dismount Non-Windows partitions */

  HANDLE *volume_handles;         /* HANDLEs of all successfully locked volumes */
  size_t  handle_count;           /* Length of the array `volume_handles' */
  bool    continue_op;            /* Did the user decide to abort `dismount_selected_volumes()'? */
};

/* */
typedef char    wdx_t  [19 + 1];
typedef wchar_t wdxw_t [19 + 1];

/* */
extern bool
  force_operations,
  always_confirm;

/* */
extern bool
  dskmgmt_initialize (void);
extern void
  dskmgmt_free (void);
extern bool
  WMIEnumDrives (void);

/* Detected drives: */
extern uint32_t
  number_of_local_disks (void);
extern uint32_t
  index_nth_local_disk (uint32_t disk_index);
extern uint32_t
  number_of_disk_partitions (uint32_t const disk_index);
extern uint32_t
  number_of_optical_disks (void);
extern uint32_t
  index_nth_optical_disk (uint32_t disk_index);
extern void
  DumpDrivesAndPartitions (void);

/* Drive selection and paths: */
extern void
  drives_reset_selection (void);
extern bool
  select_drive_by_wdxid (wdx_t const wdx_id, TCHAR const **mount_point);
extern bool
  select_drive_by_devicename (TCHAR const nt_device_name [], TCHAR const **mount_point);
extern bool
  select_drive_by_mountpoint (TCHAR const *mount_point);
extern bool
  select_drive_by_letter (TCHAR const *drive_letter);
extern void
  get_selected_diskpartition (uint32_t *disk_index, uint32_t *part_index);
extern bool
  get_selected_wdxid (wdx_t wdx_id);
extern enum EDriveType
  get_drive_type (void);

/* Sub-Header files: */
#include "physical_disks.h"
#include "logical_volumes.h"
#include "wdx_identifer.h"

/* Dismount drives: */
extern bool
  dismount_selected_volumes (struct VolumeDesc const *vol, TCHAR const display_name [],
                             bool force_wdx_logdrive, bool const read_only,
                             struct DismountStatistics *statistics);

#endif
