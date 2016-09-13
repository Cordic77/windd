#ifndef WINDD_PHYSICAL_DISKS_H_
#define WINDD_PHYSICAL_DISKS_H_

/* Optical disk properties: */
extern bool
  drives_optdisk_selected (void);
extern bool
  get_optdisk_path (TCHAR optdisk_path []);

/* Physical disk properties: */
extern bool
  drives_physdisk_selected (void);
extern bool
  get_physdisk_path (TCHAR physdisk_path []);
extern long long
  get_physdisk_size (void);
extern bool
  disk_contains_system_drive (void);
extern bool
  disk_is_ssd (void);

#endif
