#ifndef WINDD_WDX_IDENTIFIER_H_
#define WINDD_WDX_IDENTIFIER_H_

/* Disk/partition naming scheme: */
extern bool
  is_wdx_id (wdx_t const wdx_id, enum EDriveType *drive_type, bool *drive_partition);
extern bool
  diskpart2wdx (uint32_t const disk_index, uint32_t const part_index, wdx_t wdx_id);
extern bool
  disk2wdx (uint32_t const disk_index, wdx_t wdx_id);
extern bool
  wdx2diskpart (wdx_t const wdx_id, uint32_t *disk_index, uint32_t *part_index);
extern bool
  wdx2disk (wdx_t const wdx_id, uint32_t *disk_index);
extern bool
  valid_part_index (uint32_t const part_index);

/* CD/DVD/BD naming scheme: */
extern bool
  cdrom2wdx (uint32_t const disk_index, wdx_t wdx_id);
extern bool
  wdx2cdrom (wdx_t const wdx_id, uint32_t *disk_index);

#endif
