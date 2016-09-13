/* Disk/partition naming scheme: */

/* Provide support for wdx identifiers for up to 26^6 (308.915.776) disks
   and up to 999 partitions per disk:
   wda1 ... wdz999, wdaa1 ... wdzz999, ..., wdaaaaaa1 ... wdzzzzzz999

   Worst (degenerate) case: "wdnxmrlxv4294967294"
   wdnxmrlxv = 13*26^6 + 23*26^5 + 12*26^4 + 17*26^3 + 11*26^2 + 23*26 + 21
*/
extern bool is_wdx_id (wdx_t const wdx_id, enum EDriveType *drive_type, bool *drive_partition)
{
  if (drive_type != NULL)
   *drive_type = DTYPE_INVALID;
  if (drive_partition != NULL)
   *drive_partition = false;

  if (wdx_id != NULL)
  {
    if (memcmp (wdx_id, "/dev/w", 6) == 0)
      wdx_id += 5;

    /* Disk? */
    if (wdx_id [1] == 'd')  /* "wdnxmrlxv4294967294" */
    { size_t
        index;

      /* simple consistency checks: */
      if (not isalpha (wdx_id[2])
       || strlen(wdx_id) > strlen("wdzzzzzz999"))
        return (false);

      /* string of letters */
      index = 3;
      while (isalpha (wdx_id [index]))
        ++index;

      if (wdx_id[index] == '\0')
      {
        if (drive_type != NULL)
         *drive_type = DTYPE_LOC_DISK;
        return (true);
      }

      /* string of digits */
      while (isdigit (wdx_id [index]))
        ++index;

      if (wdx_id[index] == '\0')
      {
        if (drive_type != NULL)
         *drive_type = DTYPE_LOC_DISK;
        if (drive_partition != NULL)
         *drive_partition = true;
        return (true);
      }
    }

    /* CD/DVD/BD drive? */
    else
    if (wdx_id [1] == 'r')  /* "wr308915775" */
    { size_t
        index;

      /* simple consistency checks: */
      if (not isdigit (wdx_id[2])
       || strlen(wdx_id) > strlen("wr308915775"))
        return (false);

      /* string of digits */
      index = 3;
      while (isdigit (wdx_id [index]))
        ++index;

      if (wdx_id[index] == '\0')
      {
        if (drive_type != NULL)
         *drive_type = DTYPE_OPT_DISK;
        return (true);
      }
    }
  }

  return (false);
}


static bool is_wdx_id_wide (wdxw_t const wdxw_id, enum EDriveType *drive_type, bool *drive_partition)
{ wdx_t
   wdx_id;

  wcstombs (wdx_id, wdxw_id, sizeof(wdx_id));

  return (is_wdx_id (wdx_id, drive_type, drive_partition));
}


extern bool diskpart2wdx (uint32_t const disk_index, uint32_t const part_index, wdx_t wdx_id)
{
  /* part_index */
  char part_id [10 + 1];

  if (part_index != INVALID_PART)
    _snprintf (part_id, sizeof(part_id), "%" PRIu32, (uint32_t)(part_index + 1));
  else
    part_id [0] = '\0';

  /* disk_index */
  { size_t
      index = sizeof(wdx_t);
    uint32_t
      disk_rem = disk_index;

    wdx_id [--index] = '\0';
    wdx_id [0] = 'w';
    wdx_id [1] = 'd';

    do {
      wdx_id [--index] = 'a' + (disk_rem % 26);
      disk_rem /= 26;
    } while (disk_rem > 0);

    memmove (wdx_id + 2, wdx_id + index, sizeof(wdx_t)-index);
  }

  strcat (wdx_id, part_id);
  return (true);
}


extern bool disk2wdx (uint32_t const disk_index, wdx_t wdx_id)
{
  return (diskpart2wdx (disk_index, INVALID_PART, wdx_id));
}


extern bool wdx2diskpart (wdx_t const wdx_id, uint32_t *disk_index, uint32_t *part_index)
{ enum EDriveType
    drive_type;

  if (part_index != NULL)
   *part_index = INVALID_PART;
  if (disk_index != NULL)
   *disk_index = INVALID_DISK;
  else
    return (false);

  if (not is_wdx_id (wdx_id, &drive_type, NULL) || not IsLocalDisk (drive_type))
    return (false);

  if (memcmp (wdx_id, "/dev/", 5) == 0)
    wdx_id += 5;

  /* disk_id */
  {
    wdx_id += 2;
   *disk_index = 0;

    do {
     *disk_index = (*disk_index * 26) + (int)(*wdx_id++ - 'a');
    } while (isalpha (*wdx_id));

    if (part_index!=NULL && not isdigit (*wdx_id))
      return (false);
  }

  /* part_id */
  if (part_index != NULL)
  { char *
      endptr;
    int
      conv_err;

    errno = 0;
   *part_index = strtoul (wdx_id, &endptr, 10);
    conv_err = errno;
    if (conv_err!=0 || *endptr!='\0')
      return (false);

    /* partitions are numbered from 1 (not 0): */
    if (*part_index == 0)
      return (false);

   *part_index -= 1;
  }

  return (true);
}


extern bool wdx2disk (wdx_t const wdx_id, uint32_t *disk_index)
{
  return (wdx2diskpart (wdx_id, disk_index, NULL));
}


extern bool valid_part_index (uint32_t const part_index)
{
  return (part_index != INVALID_PART);
}


/* Establish same (somewhat arbitrary) limit of up to 26^6 (308.915.776) disks:
   wr0 ... wr308915775
*/
extern bool cdrom2wdx (uint32_t const disk_index, wdx_t wdx_id)
{
  wdx_id[0] = '\0';
  if (disk_index > 308915775)
    return (false);

  _snprintf (wdx_id, sizeof(wdx_t), "wr%" PRIu32, (uint32_t)disk_index);
  return (true);
}


extern bool wdx2cdrom (wdx_t const wdx_id, uint32_t *disk_index)
{ enum EDriveType
    drive_type;

  if (disk_index != NULL)
   *disk_index = INVALID_DISK;
  else
    return (false);

  if (not is_wdx_id (wdx_id, &drive_type, NULL) || not IsOpticalDisk (drive_type))
    return (false);

  if (memcmp (wdx_id, "/dev/", 5) == 0)
    wdx_id += 5;

  /* disk_id */
  { char *
      endptr;
    int
      conv_err;

    errno = 0;
   *disk_index = strtoul (wdx_id+2, &endptr, 10);
    conv_err = errno;
    if (conv_err!=0 || *endptr!='\0')
      return (false);

    /* CD/DVD/BD drives *are* numbered from 0: */
    return (true);
  }
}
