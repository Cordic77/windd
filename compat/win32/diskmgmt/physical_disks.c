static bool NTDeviceNamespaceVolumeName (struct DiskInfo const *disk, struct PartInfo *diskpart)
{ static TCHAR
    target_path [MAX_PATH + 1];
  TCHAR
   *harddisk, *separator;

  /* Which drive? Discussion in 'Microsoft Windows 2000', Apr 14, 2004
  http://www.pcreview.co.uk/threads/which-drive.1512379/#post-4551820
  I have seen these paths end with three things after Harddisk#\
  a. DR#          is simply an abbreviation for Drive and should match the
                  Harddisk# entry in the path before it.
  b. Partition#   indicates the partion on that drive.
  c. FT#          is an abbreviation for Fault Tolerance set, indicates the
                  Fault Talerance set the drive belongs to.
  Since yours is simply "\Device\Harddisk1\DR1" and "\Device\Harddisk2\DR2"
  both the Harddisk# and DR# represent the same as the Disk# you see in Disk
  Management.
  */

  /* L"PhysicalDrive0"  =>  L"\Device\Harddisk0\DR0" */
  harddisk = disk->device_path;
  if (memcmp (harddisk, UNC_PHYS_DEVICE, UNC_PHYS_LENGTH*sizeof(TCHAR)) == 0)
    harddisk += UNC_DISPARSE_LENGTH;

  if (QueryDosDevice (harddisk, target_path, _countof(target_path)-1) == 0)
    return (false);

  /* L"\Device\Harddisk0\DR0"  =>  L"Harddisk0" */
  if ((harddisk=_tcsistr (target_path, TEXT("Harddisk"))) == NULL)
    return (false);

  separator = harddisk + _tcslen(TEXT("Harddisk"));
  if (not _istdigit (*separator))
    return (false);

  for (++separator; _istdigit (*separator); ++separator)
    ;
 *separator = TEXT('\0');

  /* L"Harddisk0Partition1"  =>  L"\Device\HarddiskVolume1" */
  _sntprintf (separator, _countof(target_path)-(separator-target_path)-1,
              TEXT("Partition%") TEXT(PRId32), diskpart->PartitionEntry.PartitionNumber);
  if (QueryDosDevice (harddisk, target_path, _countof(target_path)-1) == 0)
    return (false);
  diskpart->harddisk_volume = (TCHAR *)MemAlloc (_tcslen(target_path) + 1, sizeof(*diskpart->harddisk_volume));
  _tcscpy (diskpart->harddisk_volume, target_path);

  /* L"\\Device\\HarddiskVolume1"  =>  L"\\\\?\\Volume{916307dc-0000-0000-0000-600000000000}\\" */
  { uint32_t
      v;

    TCHAR *harddisk_volume_guid = NTDeviceNameToVolumeGuid (target_path);

    /* Search corresponding `struct VolumeInfo': */
    if (harddisk_volume_guid != NULL)
    {
      for (v=0; logical_volume[v].valid; ++v)
        if (EqualLogicalVolumeGuid (logical_volume[v].volume_guid, harddisk_volume_guid))
          break;

      if (logical_volume[v].valid)
        Free (harddisk_volume_guid);
      else
      {
        _ftprintf (stderr, TEXT("\n[%s] The volume GUID '%s'\n")
                           TEXT("has no logical volume entry; this is likely a bug ... aborting!\n"),
                           TEXT(PROGRAM_NAME), harddisk_volume_guid);
        exit (247);
      }

      diskpart->volume = logical_volume + v;
    }
    else
      return (false);
  }

  return (diskpart->volume != NULL);
}


/* Optical disk properties: */
extern bool drives_optdisk_selected (void)
{
  return (sel_disk!=INVALID_DISK && IsOpticalDisk(physical_disk[sel_disk].type));
}

extern bool get_optdisk_path (TCHAR optdisk_path [])
{
  if (not drives_optdisk_selected ())
    return (false);

  _tcscpy (optdisk_path, physical_disk[sel_disk].objmgr_path);
  return (true);
}


/* Physical disk properties: */
extern bool drives_physdisk_selected (void)
{
  return (sel_disk!=INVALID_DISK && IsLocalDisk(physical_disk[sel_disk].type));
}

extern bool get_physdisk_path (TCHAR physdisk_path [])
{
  if (not drives_physdisk_selected ())
    return (false);

  _tcscpy (physdisk_path, physical_disk[sel_disk].device_path);
  return (true);
}

extern off_t get_physdisk_size (void)
{
  if (not drives_physdisk_selected ())
    return ((off_t)0);

  return (physical_disk[sel_disk].size_bytes);
}

extern bool disk_contains_system_drive (void)
{
  if (not drives_physdisk_selected ())
    return (false);

  return (physical_disk[sel_disk].contains_system);
}

extern bool disk_is_ssd (void)
{
  if (not drives_physdisk_selected ())
    return (false);

  return (IsSolidStateDrive (physical_disk[sel_disk].type));
}
