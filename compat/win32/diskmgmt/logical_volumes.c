/* Inspired by GitHub:c4l-utils/DriveInfo.c # coderforlife/c4l-utils.

   See also "How to query the list of disks/partitions for a certain volume (and viceversa)"
   https://blogs.msdn.microsoft.com/adioltean/2005/05/04/how-to-query-the-list-of-diskspartitions-for-a-certain-volume-and-viceversa/

   as well as "NTFS Curiosities (part 2): Volumes, volume names and mount points"
   https://blogs.msdn.microsoft.com/adioltean/2005/04/17/ntfs-curiosities-part-2-volumes-volume-names-and-mount-points/
*/
static TCHAR *NTDeviceNameToVolumeGuid (TCHAR const nt_device_name []) /* "\Device\HarddiskVolume1" */
{ HANDLE                                                            /* or "\Device\Cdrom0" */
    handle;
  static TCHAR
    cur_volume_guid [MAX_PATH + 1],
    target_path [MAX_PATH + 1];
  TCHAR
   *fnd_volume_guid = NULL;

  /* Get HANDLE to iterate over all known volumes: */
  if ((handle=FindFirstVolume (cur_volume_guid, _countof(cur_volume_guid))) == INVALID_HANDLE_VALUE)
    WinErrReturn (NULL);

  /* Iterate volume list: */
  do {
    size_t length = _tcslen (cur_volume_guid);

    #if defined(_DEBUG)  /* These tests should "never" fail. */
    if (length < UNC_DISPARSE_LENGTH + 1
        || memcmp (cur_volume_guid, UNC_DISPARSE_PREFIX, UNC_DISPARSE_LENGTH*sizeof(TCHAR)) != 0)
      exit (246);  /*continue;*/

    /* Skip entries not ending in a '\' */
    if (cur_volume_guid [length-1] != TEXT('\\'))
      exit (246);  /*continue;*/
    #endif

    /* However, QueryDosDevice() only works, if any trailing '\' are removed */
    if (cur_volume_guid [length-1] == TEXT('\\'))
      cur_volume_guid [--length] = TEXT('\0');

    if (QueryDosDevice (
            cur_volume_guid+UNC_DISPARSE_LENGTH,  /* "Volume{916307dc-0000-0000-0000-600000000000}" */
            target_path,                          /* => "\Device\HarddiskVolume1" */
            _countof(target_path)) == 0
          )
      continue;

    if (_tcscmp (target_path, nt_device_name) == 0)
    {
      /* Restore trailing '\'? */
      if (cur_volume_guid [length-1] != TEXT('\\'))
      {
        cur_volume_guid [length++] = TEXT('\\');
        cur_volume_guid [length] = TEXT('\0');
      }

      fnd_volume_guid = (TCHAR *)MemAlloc (length+1, sizeof(*fnd_volume_guid));
      _tcscpy (fnd_volume_guid, cur_volume_guid);
      break;
    }
  } while (FindNextVolume (handle, cur_volume_guid, _countof(cur_volume_guid)));

  FindVolumeClose (handle);
  return (fnd_volume_guid);
}


extern bool SelectMountPointByIndex (size_t const mount_point_index, TCHAR const **mount_point)
{ size_t
    index = (size_t)-1;

  if (mount_point != NULL)
   *mount_point = NULL;
  if (sel_volume == INVALID_VOLUME)
    return (false);

  while (logical_volume[sel_volume].mount_point[++index][0] != TEXT('\0'))
    if (index == mount_point_index)
    {
      sel_mount_point = logical_volume[sel_volume].mount_point[index];
      if (mount_point != NULL)
       *mount_point = sel_mount_point;
      return (true);
    }

  return (false);
}


extern bool SearchDriveLetterMountPoint (TCHAR const **mount_point)
{ size_t
    index = (size_t)-1;

  if (mount_point != NULL)
   *mount_point = NULL;
  if (sel_volume == INVALID_VOLUME)
    return (false);

  while (logical_volume[sel_volume].mount_point[++index][0] != TEXT('\0'))
    if (IsDriveLetter (logical_volume[sel_volume].mount_point[index], true, NULL, NULL))
    {
      sel_mount_point = logical_volume[sel_volume].mount_point[index];
      if (mount_point != NULL)
       *mount_point = sel_mount_point;
      return (true);
    }

  return (false);
}


extern bool GetMountPoint (struct VolumeInfo const *volume, TCHAR const **mount_point, bool require_drive_letter)
{
  if (mount_point != NULL)
  { TCHAR const
     *colon;
    size_t
      i = 0;

   *mount_point = NULL;
    if (volume == NULL)
      return (false);

    while (valid_mount_point (volume->mount_point [i]))
    {
      if (IsDriveLetter (volume->mount_point [i], require_drive_letter, &colon, NULL))
      {
       *mount_point = colon-1;
        return (true);
      }
      else if (not require_drive_letter)
      {
       *mount_point = volume->mount_point [i];
        return (true);
      }

      ++i;
    }
  }

  return (false);
}


extern bool IsDirMountPoint (TCHAR const mount_point [])
{
  return (not IsDriveLetter (mount_point, true, NULL, NULL));
}


extern bool IsDriveLetter (TCHAR const mount_point [], bool strict_semantics, TCHAR const **colon, bool *has_unc_prefix)
{
  if (not valid_mount_point (mount_point))
  {
    if (has_unc_prefix != NULL)
     *has_unc_prefix = FALSE;

    if (colon != NULL)
     *colon = NULL;

    return (false);
  }

  { TCHAR const
     *tmp_colon;

    if (has_unc_prefix != NULL)
     *has_unc_prefix = (_memicmp (mount_point, UNC_PHYS_DEVICE, UNC_PHYS_LENGTH*sizeof(TCHAR)) == 0);

    if (colon == NULL)
      colon = &tmp_colon;

    if ((*colon=_tcschr (mount_point, TEXT(':'))) == NULL)
      return (false);
    else
      if (*colon > mount_point && _istalpha (*(*colon-1)))
      {
        if (*colon > mount_point+1 && *(*colon-2)!=TEXT('\\'))  /* \\.\C: <or> \\?\C: */
          return (false);

        if (strict_semantics)
        {
          if (*(*colon+1)==TEXT('\0') || (*(*colon+1)==TEXT('\\') && *(*colon+1)==TEXT('\0')))
            return (true);

          return (false);
        }
      }
  }

  return (true);
}


extern bool GetDriveLetter (TCHAR const path [], TCHAR const **drive_letter, bool require_drive_letter)
{ TCHAR const *
    dummy_mount [2] = {path, TEXT("")};
  struct VolumeInfo
    dummy_vol = {0};

  dummy_vol.mount_point = (TCHAR **)dummy_mount;

  return (GetMountPoint (&dummy_vol, drive_letter, require_drive_letter));
}


#if 0  /*FIXME! */
extern int AssignTempDriveLetter (TCHAR const device_path [], TCHAR **drive_letter)
{ int
    free_drive_index;

  if (not drives_partition_selected () && not drives_optdisk_selected ())
    ErrnoReturn (EINVAL, -1);

  /* Enumerating all available drive letters in Windows:
     stackoverflow.com/questions/3811052/how-to-get-drives-letters-which-are-available-not-in-use-in-mfc#answer-3811150
  */
  { DWORD
      drive_bit,
      avail_drives = GetLogicalDrives ();

    for (free_drive_index=25, drive_bit=(DWORD)1<<25; free_drive_index >= 0; --free_drive_index, drive_bit>>=1)
    {
      if ((avail_drives & drive_bit) == 0)
        break;
    }

    if (free_drive_index >= 0)
    { TCHAR
        tmp_drive_letter[4];
      BOOL
        succ;

      /* See: "Editing Drive Letter Assignments"
              https://msdn.microsoft.com/en-us/library/windows/desktop/aa364014(v=vs.85).aspx
      */
      tmp_drive_letter[0] = TEXT('A') + free_drive_index;  /* "Z:" */
      tmp_drive_letter[1] = TEXT(':');
      tmp_drive_letter[2] = TEXT('\0');

      if (drives_optdisk_selected ())
      {
        /* Works for `\Device\CdromX' paths only. DeleteVolumeMountPoint() fails afterwards (?) */
        succ = DefineDosDevice (DDD_RAW_TARGET_PATH, tmp_drive_letter, device_path);
      }
      else
      {
        /*TODO: always returns GetLastError() != ERROR_SUCCESS */
        succ = SetVolumeMountPoint (tmp_drive_letter, device_path);
      }

      if (succ)
      {
      /*tmp_drive_letter[2] = TEXT('\\');                  // "Z:\"
        tmp_drive_letter[3] = TEXT('\0');*/

        { struct PartInfo *
            diskpart;
          size_t
            index;

          if (drives_optdisk_selected ())
            diskpart = &local_storage [sel_disk].part [0];
          else
            diskpart = &local_storage [sel_disk].part [sel_part];

          /* Count entries in `mount_point' array: */
          for (index=0; valid_mount_point (diskpart->mount_point [index]); ++index)
            ;

          /* Store drive letter in `mount_point' array: */
          diskpart->mount_point = (TCHAR **)MemRealloc (diskpart->mount_point, index + 2, sizeof(*diskpart->mount_point));
          diskpart->mount_point[index] = (TCHAR *)MemAlloc (3, sizeof(**diskpart->mount_point));
          diskpart->mount_point[index][0] = _totlower (tmp_drive_letter[0]);
          diskpart->mount_point[index][1] = TEXT(':');
          diskpart->mount_point[index][2] = TEXT('\0');

         *drive_letter = diskpart->mount_point [index];
          ++index;

          /* Set empty string as last element: */
          diskpart->mount_point[index] = (TCHAR *)MemAlloc(1, sizeof(**diskpart->mount_point));
          diskpart->mount_point[index][0] = TEXT('\0');
        }

        return (0);
      }
      else
        WinErrReturn (-1);
    }
    else
    {
      _ftprintf (stderr, TEXT("[%s] To be able to continue, a (temporary) drive letter needs to be\n")
                         TEXT("assigned to '%s'. Regrettably, no free drive letters seem\n")
                         TEXT("to be available ... aborting.\n"),
                         TEXT(PROGRAM_NAME), device_path);
      ErrnoReturn (EINVAL, -1);
    }
  }

  return (0);
}
#endif


static bool IsSameDrive (TCHAR const mount_point2 [], TCHAR const mount_point1 [])
{
  if (not valid_mount_point (mount_point2)
   || not valid_mount_point (mount_point1))
    return (false);

  /* mount_point2 */
  { TCHAR const
     *colon2;

    if (not IsDriveLetter (mount_point2, false, &colon2, NULL))
      return (false);

    { TCHAR const
       *colon1;

      /* mount_point1 */
      if (not IsDriveLetter (mount_point1, false, &colon1, NULL))
        return (false);

      /* Same drive? */
      return (_totlower (*(colon2-1)) == _totlower (*(colon1-1)));
    }
  }
}


static bool IsSameDriveLetter (TCHAR const drive_letter, TCHAR const *mount_point)
{
  TCHAR mount_point2 [3];

  mount_point2 [0] = drive_letter;
  mount_point2 [1] = TEXT(':');
  mount_point2 [2] = TEXT('\0');

  return (IsSameDrive (mount_point2, mount_point));
}


/* See "Volume to physical drive"
   http://stackoverflow.com/questions/3824221/volume-to-physical-drive#answer-3824398

   The way you map volumes to the PhysicalDrive names in Win32 is to
   first open the volume and then send IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS.
   This will return a structure that has one DISK_EXTENT entry for every
   partition that the volume spans:

   typedef struct _VOLUME_DISK_EXTENTS {
     DWORD       NumberOfDiskExtents;
     DISK_EXTENT Extents[ANYSIZE_ARRAY];
   } VOLUME_DISK_EXTENTS, *PVOLUME_DISK_EXTENTS;

   The extents have a disk number in them:

   typedef struct _DISK_EXTENT {
     DWORD         DiskNumber;
     LARGE_INTEGER StartingOffset;
     LARGE_INTEGER ExtentLength;
   } DISK_EXTENT, *PDISK_EXTENT;

   The DiskNumber is what goes into the PhsyicalDriveX link, so one can just
   sprintf that number with "\\.\PhysicalDrive%d"
*/
static bool RetrieveMountPoints (struct VolumeInfo *volume)
{ TCHAR *
    path_name = NULL;
  DWORD
    length = 0,
    returned;

  volume->is_system_drive = false;

  /* Always set `mount_point' to at least one (empty) string: */
  volume->mount_point = (TCHAR **)MemAlloc (1, sizeof(*volume->mount_point));
  volume->mount_point[0] = (TCHAR *)MemAlloc (1, sizeof(**volume->mount_point));
  volume->mount_point[0][0] = TEXT('\0');

  /* Note: only works if `lpszVolumeName' is terminated by a trailing '\' */
  GetVolumePathNamesForVolumeName (volume->volume_guid, path_name, length, &returned);
  if (GetLastError() != ERROR_MORE_DATA)  /* Any (real) errors? */
    return (false);

  length = returned;
  path_name = (TCHAR *)MemRealloc (path_name, length, sizeof(*path_name));

  /* Retrieve a list of drive letters and mounted folder paths for the specified volume: */
  GetVolumePathNamesForVolumeName (volume->volume_guid, path_name, length, &returned);
  if (GetLastError() != ERROR_MORE_DATA)  /* Any (real) errors? */
  {
    Free (path_name);
    return (false);
  }

  /* Store mount points: */
  { TCHAR const *
      mount_point;
    size_t
      index = 0;

    for (mount_point=path_name; valid_mount_point (mount_point); mount_point += length+1)
    {
      if (mount_point == path_name)
        Free (volume->mount_point [0]);

      length = (DWORD)_tcslen (mount_point);

      /* Drive letter/directory mount point: adding 1 to (length+1) allows us
         to later (temporariliy) add a trailing backslash, when required: */
      volume->mount_point = (TCHAR **)MemRealloc (volume->mount_point, index + 2, sizeof(*volume->mount_point));
      volume->mount_point [index] = (TCHAR *)MemAlloc (length + 1 + 1, sizeof(**volume->mount_point));

      _tcscpy (volume->mount_point [index], mount_point);

      /* Remove any trailing backslashes, for now: */
      RemoveTrailingBackslash (volume->mount_point [index]);

      /* Even in a multiboot configuration, the partition we booted from will
         always have `C:' as a drive letter: */
      if (IsDriveLetter (volume->mount_point [index], true, NULL, NULL))
        if (IsSameDriveLetter (TEXT('C'), mount_point))
          volume->is_system_drive = true;

      ++index;
    }

    /* Set empty string as last element: */
    volume->mount_point [index] = (TCHAR *)MemAlloc (1, sizeof(**volume->mount_point));
    volume->mount_point [index][0] = TEXT('\0');
  }

  Free (path_name);
  return (true);
}


extern size_t GetMountPointCount (struct VolumeInfo *volume)
{ size_t
    index = 0;

  if (volume != NULL)
  {
    while (volume->mount_point[index][0] != TEXT('\0'))
      ++index;
  }

  return (index);
}


static int SortByDiskIdVolumeExtent (void const *e1, void const *e2)
{ struct VolumeInfo const *v1 = (struct VolumeInfo const *)e1;
  struct VolumeInfo const *v2 = (struct VolumeInfo const *)e2;

  if (v1->extent_count > 0 && v2->extent_count > 0)
  {
    if (v1->extent->info.DiskNumber == v2->extent->info.DiskNumber)
    {
      return ((v1->extent->info.StartingOffset.QuadPart > v2->extent->info.StartingOffset.QuadPart)
            - (v2->extent->info.StartingOffset.QuadPart > v1->extent->info.StartingOffset.QuadPart));
    }
    else
    {
      return ((v1->extent->info.DiskNumber > v2->extent->info.DiskNumber)
            - (v2->extent->info.DiskNumber > v1->extent->info.DiskNumber));
    }
  }
  else
  if (v1->extent_count > 0)
    return (-1);
  else /*if (v2->extent_count > 0)*/
    return (1);
}

extern bool EnumLogicalVolumes (void)
{ HANDLE
    handle;
  static TCHAR
    nt_volume_guid [MAX_PATH + 1];
  uint32_t
    v = INVALID_VOLUME;

  /* Get HANDLE to iterate over all known volumes: */
  if ((handle=FindFirstVolume (nt_volume_guid, _countof(nt_volume_guid))) == INVALID_HANDLE_VALUE)
    WinErrReturn (false);

  /* Iterate volume list: */
  do {
    struct VolumeDesc *
      vol;
    VOLUME_DISK_EXTENTS *
      vde;

    size_t length = _tcslen (nt_volume_guid);

    #if defined(_DEBUG)  /* These tests should "never" fail. */
    if (length < UNC_DISPARSE_LENGTH + 1
        || memcmp (nt_volume_guid, UNC_DISPARSE_PREFIX, UNC_DISPARSE_LENGTH*sizeof(TCHAR)) != 0)
      exit (246);  /*continue;*/

    /* Skip entries not ending in a '\' */
    if (nt_volume_guid [length-1] != TEXT('\\'))
      exit (246);  /*continue;*/
    #endif

    /* However, CreateFile() only works, if any trailing '\' are removed */
    if (nt_volume_guid [length-1] == TEXT('\\'))
      nt_volume_guid [--length] = TEXT('\0');

    if ((vol=OpenVolumeInfo (-1, nt_volume_guid)) != NULL)
    { bool
        is_cdrom = false;  /* CD/DVD/BD drives, naturally, have no volume extents */

      if (not GetVolumeExtents (vol, &vde)
          /* Ignore errors for `\Device\CdromX': */
          && not (is_cdrom=(GetLastError () == ERROR_INVALID_FUNCTION)))
      {
        CloseVolume (&vol);
        return (-1);
      }
      /* Search corresponding `struct PartInfo': */
      else
      { uint32_t
          e;

        /* Restore trailing '\'? */
        if (nt_volume_guid [length-1] != TEXT('\\'))
        {
          nt_volume_guid [length++] = TEXT('\\');
          nt_volume_guid [length] = TEXT('\0');
        }

        /* Realloc `logical_volume' array: */
        ++v;
        logical_volume = (struct VolumeInfo *)MemRealloc (logical_volume, v+2, sizeof(*logical_volume));
        memset (logical_volume + v, 0, 2*sizeof(*logical_volume));

        /* Store volume GUID: */
        logical_volume[v].volume_guid = (TCHAR *)MemAlloc (length+1, sizeof(*nt_volume_guid));
        _tcscpy (logical_volume[v].volume_guid, nt_volume_guid);

        /* Ignore errors; drive letters aren't (strictly) necessary! */
        (void) RetrieveMountPoints (logical_volume + v);

        /* Store volume extents: */
        if (not is_cdrom)  /* implies vde != NULL */
        {
          logical_volume[v].extent_count = (uint32_t)vde->NumberOfDiskExtents;
          logical_volume[v].extent = (struct ExtentsInfo *)MemAlloc (logical_volume[v].extent_count,
                                                                      sizeof(*logical_volume[v].extent));
          for (e=0; e < logical_volume[v].extent_count; ++e)
          {
            memcpy (&logical_volume[v].extent[e].info, vde->Extents + e, sizeof(*vde->Extents));

            /* Set `part' reference to NULL for now; this is corrected later on,
                as soon as we have all the necessary partitioning information: */
            logical_volume[v].extent[e].part = NULL;  /* see end of `WMIEnumDrives()'! */
          }

          Free (vde);
        }

        logical_volume[v].valid = true;
      }

      CloseVolume (&vol);
    }
  /*else
      WinErrReturn (false);*/
  } while (FindNextVolume (handle, nt_volume_guid, _countof(nt_volume_guid)));

  FindVolumeClose (handle);

  /* Add dummy drive A: for testing purposes: */
  #if defined(_DEBUG)
  ++v;
  logical_volume = (struct VolumeInfo *)MemRealloc (logical_volume, v+2, sizeof(*logical_volume));
  memset (logical_volume + v, 0, 2*sizeof(*logical_volume));

  logical_volume[v].volume_guid = (TCHAR *)MemAlloc (49+1, sizeof(*nt_volume_guid));
  _tcscpy (logical_volume[v].volume_guid, TEXT("\\\\?\\Volume{1258b72b-c2df-11e3-be66-806e6f6e6963}\\"));
  logical_volume[v].valid = true;

  logical_volume[v].mount_point = (TCHAR **)MemAlloc (2, sizeof(*logical_volume[v].mount_point));
  logical_volume[v].mount_point[0] = (TCHAR *)MemAlloc (3, sizeof(**logical_volume[v].mount_point));
  _tcscpy (logical_volume[v].mount_point[0], TEXT("A:"));

  logical_volume[v].mount_point[1] = (TCHAR *)MemAlloc (1, sizeof(**logical_volume[v].mount_point));
  logical_volume[v].mount_point[1][0] = TEXT('\0');
  #endif

  /* Sort by disk id of the first extent (as well as its StartingOffset): */
  qsort (logical_volume, v+1, sizeof(*logical_volume), SortByDiskIdVolumeExtent);
  return (true);
}


/*
// Case 1: Overlap at the beginning?
// ---------------------------------
//  off2     off2+len2
//  v        v
//  ddddddddd
//          ssssssssss
//          ^         ^
//          off1      off1+len1
// ---------------------------------
//
// Case 2: Overlap at the end?
// ---------------------------------
//            off2     off2+len2
//            v        v
//            ddddddddd
//   ssssssssss
//   ^         ^
//  off1       off1+len1
// ---------------------------------
*/
extern bool PartitionsOverlap (off_t const StartingOffset1, off_t PartitionLength1,
                               off_t const StartingOffset2, off_t PartitionLength2)
{
  return (
      StartingOffset1 < StartingOffset2 + PartitionLength2  /* Case 1 */
   && StartingOffset2 < StartingOffset1 + PartitionLength1  /* Case 2 */
    );
}


/*
   Support for `Dynamic disks': see "Dynamic vs Basic Partitions"
   https://bytes.com/topic/net/answers/537860-dynamic-vs-basic-partitions

   Dyanamic disks are partitioned using Logical Disk Manager (LDM)
   partitioning. The LDM subsystem in Windows, which consists of user-mode
   and device driver components, oversees dynamic disks. LDM maintains one
   unified database that stores partitioning information for all the dynamic
   disks on a system -- including multipartition-volume configuration.

   The LDM database resides in a 1-MB reserved space at the end of each
   dynamic disk. Unfortunately this database's format is not officially
   documented.
*/
extern bool MaybeDynamicDisk (struct DiskInfo const *disk)
{
  if (disk->PartitionStyle == PARTITION_STYLE_MBR)
  {
/*  /dev/wdb   MBR          VMware, VMware Virtual S
      \\.\PHYSICALDRIVE1    1069286400 bytes, FW rev: 1.0
        /dev/wdb1                                  32,256             1,072,660,992 */
    return (disk->part_count == 1);
  }
  else
  if (disk->PartitionStyle == PARTITION_STYLE_GPT)
  {
/*  /dev/wdb   GPT          VMware, VMware Virtual S
      \\.\PHYSICALDRIVE1    1069286400 bytes, FW rev: 1.0
        /dev/wdb1                                  17,408                 1,048,576
        /dev/wdb2                               1,065,984                32,505,856
        \Device\HarddiskVolume6
        /dev/wdb3                              33,571,840             1,040,153,088 */
    return (disk->part_count == 3);
  }

  return (false);
}


extern bool DynamicPartitionMatch (off_t const StartingOffset1, off_t PartitionLength1,
                                   off_t const StartingOffset2, off_t PartitionLength2)
{ off_t
    offset64k = StartingOffset2,
    length1mib = PartitionLength2;

  offset64k = (offset64k + 65535) & ~65535;
  length1mib = (length1mib - 1048576) & ~1048575;

  return (StartingOffset1 == offset64k && PartitionLength1 == length1mib);
}


extern bool DiskPartitionToVolume (uint32_t const disk_index, uint32_t const part_index, uint32_t *vol_index, uint32_t *extent_index)
{ uint32_t
    p = part_index;
  uint32_t
    v;

 *vol_index = INVALID_VOLUME;
  if (extent_index != NULL)
   *extent_index = INVALID_EXTENT;

  /* CD/DVD/BD drive? */
  if (IsOpticalDisk (physical_disk[disk_index].type))
  {
    if (p == INVALID_PART)
      p = 0;
  }
  else
  {
    if (p == INVALID_PART || IsExtendedPartitionIndex (disk_index, p))
      return (true);
  }

  /* Non-Windows partitions reference a (non-valid) "dummy" volume descriptor: */
  if (physical_disk[disk_index].part[p].volume->valid)
  {
    for (v=0; logical_volume[v].valid; ++v)
      if (logical_volume + v == physical_disk[disk_index].part[p].volume)
      {
       *vol_index = v;

        if (extent_index != NULL && part_index != INVALID_PART)
        { uint32_t
            e;

          for (e=0; e < logical_volume[v].extent_count; ++e)
            if (logical_volume[v].extent[e].part == physical_disk[disk_index].part + p)
            {
             *extent_index = e;
              break;
            }
        }

        return (true);
      }

    fprintf (stderr, "\n[%s] The partition (%" PRIu32 ",%" PRIu32 ") has no matching volume entry ... aborting!\n",
                        PROGRAM_NAME, disk_index, p);
    exit (245);
  }

  return (false);
}


extern bool VolumeToDiskPartition (struct VolumeInfo *volume, uint32_t const extent_index, uint32_t *disk_index, uint32_t *part_index)
{ DISK_EXTENT const
   *disk_ext = NULL;
  uint32_t
    d, p;

  for (d=0; physical_disk[d].type != DTYPE_INVALID; ++d)
    /* Prefer to search by `extent id'; otherwise, fall back on `volume guid': */
    if (extent_index != INVALID_EXTENT)
    {
      disk_ext = &volume->extent[extent_index].info;

      if (IsLocalDisk (physical_disk[d].type)
       && physical_disk[d].DiskNumber == disk_ext->DiskNumber) /* DiskNumber MUST match! */
      {
        for (p=0; p < physical_disk[d].part_count; ++p)
        { PARTITION_INFORMATION_EX const *
            CurrentPartition = &physical_disk[d].part[p].PartitionEntry;
          bool
            dynamic_match = false,
            exact_match = (disk_ext->StartingOffset.QuadPart == CurrentPartition->StartingOffset.QuadPart
                        && disk_ext->ExtentLength.QuadPart == CurrentPartition->PartitionLength.QuadPart);

          if (MaybeDynamicDisk (physical_disk + d))
          {
            dynamic_match = DynamicPartitionMatch (
                                disk_ext->StartingOffset.QuadPart,
                                  disk_ext->ExtentLength.QuadPart,
                                CurrentPartition->StartingOffset.QuadPart,
                                  CurrentPartition->PartitionLength.QuadPart);
          }

          if (exact_match || dynamic_match)
          {
           *disk_index = d;
           *part_index = p;
            return (true);
          }
        }

        _ftprintf (stderr, TEXT("\n[%s] The volume '%s'\n")
           TEXT("has no matching partition table entry ... aborting!\n"),
           TEXT(PROGRAM_NAME), volume->volume_guid);
        exit (245);
      }
    }
    else
    {
      /* If we not only search `.part[0]', but all `part_count' partitions,
         we might find the right one, even if we don't have an extent id: */
      for (p=0; p < physical_disk[d].part_count; ++p)
      {
        if (physical_disk[d].part[p].volume->volume_guid != NULL)
        {
          if (EqualLogicalVolumeGuid (physical_disk[d].part[p].volume->volume_guid, volume->volume_guid))
          {
           *disk_index = d;
            if (IsOpticalDisk (physical_disk[d].type))
             *part_index = INVALID_PART;
            else
             *part_index = p;
            return (true);
          }
        }
      }
    }

  /* This happens for floppy disks (and other removable drives),
     and isn't necessarily an indicator of a bug in the program: */
  /*
  _ftprintf (stderr, TEXT("\n[%s] The volume '%s'\n")
             TEXT("has no matching partition table entry ... aborting!\n"),
             TEXT(PROGRAM_NAME), volume->volume_guid);
  exit (245);
  */
  return (false);
}


/* Logical volume properties: */
extern bool drives_partition_selected (void)
{
  return (sel_disk != INVALID_DISK && sel_part != INVALID_PART);
}

extern bool drives_volume_selected (void)
{
  return (sel_volume != INVALID_VOLUME);
}

extern bool valid_mount_point (TCHAR const mount_point [])
{
  if (mount_point == NULL)
    return (false);

  /* Don't regard "\Device\Xxx" and "\\?\GLOBALROOT\Device\Xxx" as valid mount points: */
  if (NTDeviceName (mount_point))
    return (false);

  return (mount_point[0] != TEXT('\0'));
}

extern bool get_volume_path (TCHAR volume_path [])
{
  if (drives_partition_selected ())
  {
    _tcscpy (volume_path, physical_disk[sel_disk].part[sel_part].volume->volume_guid);
    return (true);
  }
  else
  if (drives_volume_selected ())
  {
    _tcscpy (volume_path, logical_volume[sel_volume].volume_guid);
    return (true);
  }

  return (false);
}

extern bool get_nt_harddiskvolume (TCHAR nt_harddiskvolume [])
{
  if (not drives_partition_selected ())
    return (false);

  { TCHAR const *
      harddisk_volume = physical_disk[sel_disk].part[sel_part].harddisk_volume;
    if (harddisk_volume == NULL)
      return (false);
    if (nt_harddiskvolume != NULL)
      _tcscpy (nt_harddiskvolume, physical_disk[sel_disk].part[sel_part].harddisk_volume);
  }

  return (true);
}

extern off_t get_volume_offset (void)
{
  if (not drives_partition_selected ())
    return ((off_t)-1);

  return ((off_t)physical_disk[sel_disk].part[sel_part].PartitionEntry.StartingOffset.QuadPart);
}

extern off_t get_volume_size (void)
{
  if (not drives_partition_selected ())
    return ((off_t)0);

  return ((off_t)physical_disk[sel_disk].part[sel_part].PartitionEntry.PartitionLength.QuadPart);
}

extern struct VolumeInfo const *get_system_volume (void)
{ uint32_t
    v;

  if (number_of_local_disks() == 0)
  {
    for (v = 0; logical_volume[v].valid; ++v)
      if (logical_volume[v].is_system_drive)
        return (logical_volume + v);
  }

  return (NULL);
}

extern bool is_system_drive (void)
{
  if (drives_partition_selected ())
    return (physical_disk[sel_disk].part[sel_part].volume->is_system_drive);
  else
  if (drives_volume_selected ())
    return (logical_volume[sel_volume].is_system_drive);

  return (false);
}
