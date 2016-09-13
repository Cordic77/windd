#ifndef WINDD_LOGICAL_VOLUMES_H_
#define WINDD_LOGICAL_VOLUMES_H_

extern size_t
  GetMountPointCount (struct VolumeInfo *volume);
extern bool
  SelectMountPointByIndex (size_t const mount_point_index, TCHAR const **mount_point);
extern bool
  SearchDriveLetterMountPoint (TCHAR const **mount_point);
extern bool
  GetMountPoint (struct VolumeInfo const *volume, TCHAR const **mount_point, bool require_drive_letter);
extern bool
  IsDirMountPoint (TCHAR const mount_point []);
extern bool
  IsDriveLetter (TCHAR const mount_point [], bool strict_semantics, TCHAR const **colon, bool *has_unc_prefix);
extern bool
  GetDriveLetter (TCHAR const path [], TCHAR const **drive_letter, bool require_drive_letter);
extern bool
  EnumLogicalVolumes (void);
extern bool
  PartitionsOverlap (off_t const StartingOffset1, off_t PartitionLength1,
                     off_t const StartingOffset2, off_t PartitionLength2);
extern bool
  MaybeDynamicDisk (struct DiskInfo const *disk);
extern bool
  DynamicPartitionMatch (off_t const StartingOffset1, off_t PartitionLength1,
                         off_t const StartingOffset2, off_t PartitionLength2);
extern bool
  DiskPartitionToVolume (uint32_t const disk_index, uint32_t const part_index,
                         uint32_t *vol_index, uint32_t *extent_index);
extern bool
  VolumeToDiskPartition (struct VolumeInfo *volume, uint32_t const extent_index,
                         uint32_t *disk_index, uint32_t *part_index);

/* Logical volume properties: */
extern bool
  drives_partition_selected (void);
extern bool
  drives_volume_selected (void);
extern bool
  valid_mount_point (TCHAR const mount_point []);
extern bool
  get_volume_path (TCHAR volume_path []);
extern bool
  get_nt_harddiskvolume (TCHAR nt_harddiskvolume []);
extern off_t
  get_volume_offset (void);
extern off_t
  get_volume_size (void);
extern bool
  is_system_drive (void);

#endif
