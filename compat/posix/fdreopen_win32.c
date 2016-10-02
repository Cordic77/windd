#include "win32\devfiles_win32.h" /* read_dev_xxx(), lseek_dev_xxx() */
#include "win32\objmgr_win32.h"   /* NormalizeNTDevicePath() */
#include "win32\diskmgmt\diskmgmt_win32.h"  /* DTYPE_XXX, drives_reset_selection(), select_drive_by_letter() */
                                            /*"logical_volumes.h" Get/IsDriveLetter(), SearchDriveLetterMountPoint() */

/* */
enum EReopenHandler
{
  FDREOPEN_NONE           = -2,
  FDREOPEN_EINVAL         = -1,   /* Invalid argument (EINVAL)                  */
  FDREOPEN_FILE_INVALID   =  0,   /* No valid if= or of= file                   */

  FDREOPEN_LINUX_DEV      =  1,   /* /dev/random                                */
  FDREOPEN_MSDOS_DEV      =  2,   /* CON:, NUL:                                 */
  FDREOPEN_WDX_PHYCDR     =  3,   /* /dev/wr0       # \Device\CdRom0            */
  FDREOPEN_WDX_PHYDSK     =  4,   /* /dev/wda       # \\.\PHYSICALDRIVE0        */
  FDREOPEN_WDX_LOGDRV     =  5,   /* /dev/wda1                                  */

  FDREOPEN_WINNT_DEVFILE  =  6,   /* \\?\GLOBALROOT\Device\Harddisk0\Partition6 */
                                  /* \\?\GLOBALROOT\Device\Cdrom0               */
  FDREOPEN_DRIVE_LETTER   =  7,   /* drive-letter:  # \\.\C:                    */
  FDREOPEN_MOUNT_POINT    =  8,   /* d:\windd_drive -> f:\                      */
  FDREOPEN_NETWORK_PATH   =  9,   /* \\server\path                              */
  FDREOPEN_LOCAL_FILE     = 10    /* D:\Image.dat                               */
};


/* */
static enum EReopenHandler
  DetermineReopenHandler (char const *fileA, TCHAR fileW [], int const flags,
                          TCHAR const **mount_point, enum EDeviceFile *device_file);
static void
  ThrowClassificationException (TCHAR const reopen_file [],
                                enum EReopenHandler const reopen_handler);
static bool UserConfirmOperation (void);


/* */
static int
  reopen_recursion_depth; /* = 0; */

/* Either definition is working, but the second one is a bit
   simpler (albeit not as robust) */
#define INPUT_FILE_FLAGS  ((O_RDONLY!=0 && (flags & O_RDONLY) == O_RDONLY) || (O_RDONLY==0 && (flags & (O_WRONLY | O_RDWR))==0))
#define INPUT_FILE_FD     (desired_fd == STDIN_FILENO)

extern int fd_reopen (int desired_fd, char const *file, int flags, mode_t mode)
{ struct FileDesc *
    reopen_fd = NULL;

  /* (Volume) path: */
  HANDLE
    volume_handle = NULL;         /* FDREOPEN_WDX_*, FDREOPEN_DRIVE_LETTER: volume handle */
  off_t
    volume_offset = (off_t)0,     /* FDREOPEN_WDX_*, FDREOPEN_DRIVE_LETTER: 0 <or> StartingOffset */
    volume_length = (off_t)-1;    /* FDREOPEN_WDX_*, FDREOPEN_DRIVE_LETTER: DiskInfo::size_bytes <or> PartitionLength */

  /* Reopen handler: */
  enum EDeviceFile
    device_file = DEV_NONE;       /* FDREOPEN_LINUX_DEV, FDREOPEN_WDX_*, FDREOPEN_MSDOS_DEV */
  enum EReopenHandler
    reopen_handler= FDREOPEN_NONE;/* Type of handler to use for the specified file */

  /* Properties: */
  TCHAR const *
    mount_point = NULL;           /* FDREOPEN_DRIVE_LETTER: specified drive letter <or> mount point */
  bool
    network_path = false;         /* FDREOPEN_NETWORK_PATH */
  bool
    os_device_file = false;       /* Device file (like /dev/null, CON: or \Device\Cdrom)? */
  bool
    winnt_devfile = false;        /* Specifically: Windows NT Device Namespace reference? */
  bool
    unbuf_io_supp = false;        /* O_DIRECT: unbuffered file input/output (I/O) supported? */

  reopen_fd = get_file_desc (desired_fd);
  reopen_fd->dwFlagsAndAttributes = FILE_FLAG_SEQUENTIAL_SCAN
                               /* Not all hard disk hardware has write-through capability! */
                               /* | FILE_FLAG_WRITE_THROUGH */
                                      ;

  drives_reset_selection ();

  /* (1) UTF-16 conversion and PATH normalization: */
  #pragma region PathNormalization
  os_device_file = IsDeviceFile (file, &winnt_devfile);

  { TCHAR *
      unipath = NULL;
    DWORD
      copied;
    int
      path_prefix;

    /* Retrieve full path name: */
    unipath = utf8_to_utf16_ex (file, 1);
    copied = (DWORD)_tcslen (unipath);
  /*TrimA (unipath);*/  /* If the user wants spaces at the end, he gets them! */

    /* Do we have a network path? */
    if (os_device_file || IsDriveLetter (unipath, true, NULL, NULL))
      network_path = false;
    else
      network_path = IsNetworkPath (unipath);

    /* If possible, store `file' as a UNC path: */
    path_prefix = PathPrefixW (unipath);

    if (os_device_file)
      _tcscpy (reopen_fd->file, unipath);
    else
    if (network_path)
    {
      _tcscpy (reopen_fd->file, UNC_NETSHARE_PREFIX);
      _tcscat (reopen_fd->file, unipath+path_prefix);
      copied = (DWORD)_tcslen (reopen_fd->file);
    }
    else
    {
      if (unipath [copied-1] == TEXT(':'))
      {
        unipath [copied]   = TEXT('\\');
        unipath [copied+1] = TEXT('\0');
      }

      _tcscpy (reopen_fd->file, UNC_DISPARSE_PREFIX);
      copied = GetFullPathName (unipath+path_prefix, _countof(reopen_fd->file)-UNC_DISPARSE_LENGTH,
                                reopen_fd->file+UNC_DISPARSE_LENGTH, NULL);
      if (copied == 0)
        WinErrReturn (-1);
      copied = copied + UNC_DISPARSE_LENGTH;
    }

    /* Remove trailing directory separator, if any: */
    --copied;
    if (reopen_fd->file[copied] == L'\\') /*|| reopen_fd->file[copied] == L'/') // not necessary*/
      reopen_fd->file[copied] = L'\0';

    /* Volume path? */
    if (reopen_fd->file[copied-1] == L':' && PathPrefix(reopen_fd->file) > 0)
      memcpy (reopen_fd->file, UNC_PHYS_DEVICE, UNC_PHYS_LENGTH*sizeof(*reopen_fd->file));

    Free (unipath);
  }
  #pragma endregion PathNormalization


  /* (2) Type of file to (re-)open? */
DetermineReopenHandler:
  #pragma region DetermineReopenHandler
  reopen_handler = DetermineReopenHandler (file, reopen_fd->file, flags,
                                           &mount_point, &device_file);
  _tcscpy (reopen_fd->display_name, reopen_fd->file);

  switch (reopen_handler)
  {
  case FDREOPEN_EINVAL:
    ErrnoReturn (EINVAL, -1);

  case FDREOPEN_FILE_INVALID:
    return (-1);
  }

  /* Will be invalid for FDREOPEN_NETWORK_PATH and FDREOPEN_LOCAL_FILE: */
  get_selected_diskpartition (&reopen_fd->disk_index, &reopen_fd->part_index);
  reopen_fd->drive_type = get_drive_type ();
  #pragma endregion DetermineReopenHandler


  /* (3) FDREOPEN_DRIVE_LETTER or FDREOPEN_LOCAL_FILE (includes volume access: \\.\D:) */
  #pragma region DriveLetterOrLocalFile
  if (not network_path && device_file == DEV_NONE)
  {
    int is_directory = IsDirectoryPath (reopen_fd->file);

    if (GetDriveLetter (reopen_fd->file, &mount_point, false))
    { TCHAR
        volpath [MAX_PATH + 1];
      enum EVolumePath
        classification;

      if (not GetVolumePathName (reopen_fd->file, volpath, _countof(volpath)))
      #if defined(_DEBUG)
        if (reopen_fd->file[_tcslen(reopen_fd->file)-2] == 'A')
          _tcscpy (volpath, reopen_fd->file);
        else
      #endif
        WinErrReturn (-1);

      /* Note: updates `sel_disk' and `sel_part', if successful: */
      if ((classification=IsVolumePath (reopen_fd->file, volpath)) >= VOL_DRIVE_LETTER)
      {
        get_selected_diskpartition (&reopen_fd->disk_index, &reopen_fd->part_index);

        /* Disk not found? - Try volume GUID! */
        if (reopen_fd->disk_index == (uint32_t)-1)
        {
          /* Force FDREOPEN_WINNT_DEVFILE by setting a (non-existent)
         Windows device file name; the actual value doesn't matter
       as `select_drive_by_devicename()' relies on `reopen_fd->file',
       which is set correctly: */
          file = "\\Device\\Xxx";
          if (get_volume_path (reopen_fd->file))
            goto DetermineReopenHandler;
          else
            ErrnoReturn (EACCES, -1);
        }

        reopen_fd->drive_type = get_drive_type ();
      }

      /* Use mountpoint to select the correct disk and partition: */
      if (classification == VOL_MOUNT_POINT)
      { TCHAR const *
          drive_mount_point;

        if (SearchDriveLetterMountPoint (&drive_mount_point))
        {
          mount_point    = drive_mount_point;
          reopen_handler = FDREOPEN_DRIVE_LETTER;
        }
        else
        {
        /*reopen_handler = FDREOPEN_WDX_LOGDRV;*/
          if (IsOpticalDisk (reopen_fd->drive_type))
          {
            /* Will result in `get_physdisk_path()' later on: */
            mount_point = NULL;
            reopen_handler = FDREOPEN_WDX_PHYCDR;
          }
          else
          {
            get_nt_harddiskvolume (reopen_fd->file);
            reopen_handler = FDREOPEN_WINNT_DEVFILE;
          }
        }
      }
      else
      {
        if (is_directory == 1)  /* Ignore `is_directory==-1'! */
        {
          fprintf (stderr, "\n[%s] The following path is a directory, this is not supported:\n%s\n",
                              PROGRAM_NAME, file);
          ErrnoReturn (EINVAL, -1);
        }

        if (classification == VOL_DRIVE_LETTER)
        {
          if (IsOpticalDisk (reopen_fd->drive_type))
          {
            /* Will result in `get_physdisk_path()' later on: */
            reopen_handler = FDREOPEN_WDX_PHYCDR;
          }
          else
          {
            if (reopen_handler < FDREOPEN_DRIVE_LETTER || _tcslen (reopen_fd->file) <= UNC_PHYS_LENGTH+2)
              reopen_handler = FDREOPEN_DRIVE_LETTER;
            else
              ThrowClassificationException (reopen_fd->file, reopen_handler);
          }
        }
        /* Hack, to update `sel_disk' even for files: */
        else
        { TCHAR
            file_drive_letter [3];

          _sntprintf (file_drive_letter, _countof(file_drive_letter), TEXT("%c:"), *mount_point);

          if (select_drive_by_letter (file_drive_letter))
          {
            get_selected_diskpartition (&reopen_fd->disk_index, &reopen_fd->part_index);
            reopen_fd->drive_type = get_drive_type ();
          }
          else
          if (classification == VOL_DRIVE_LETTER)
            return (-1);
        }
      }
    }
  }
  #pragma endregion DriveLetterOrLocalFile


  /* (4) Execute chosen fdreopen handler: */
  #pragma region ExecuteFdreopenHandler
  unbuf_io_supp = false;

  reopen_fd->device_file               = device_file;
  reopen_fd->phys_sect_size            =  0;
/*reopen_fd->first_byte_index          =  0;*/
/*reopen_fd->absolute_offset.QuadPart  =  0;*/
/*reopen_fd->logical_offset            =  0;*/  /* initial value anyway */
  reopen_fd->last_byte_index           = -1;    /* no limitation at all */
  reopen_fd->seekable                  = (reopen_handler > (FDREOPEN_WDX_PHYCDR  /* CD/DVD/BD are seekable when read, but */
                                                          - INPUT_FILE_FD));     /* not if we're trying to burn a new CD */

  switch (reopen_handler)
  {
  case FDREOPEN_LINUX_DEV:        /* /dev/null, /dev/(u)random */
    {
      /*hfile = DEVFILE_HANDLE_VALUE;*/  /*See: else of `(6) Open _WIN32 HANDLE' */
      reopen_fd->drive_type = DTYPE_UNKNOWN;

      /* Data source? */
      if (INPUT_FILE_FD)
      {
        switch (if_fd.device_file)
        {
        case DEV_ZERO:    if_fop.read_func  = &read_dev_zero;
                          if_fop.lseek_func = &lseek_dev_zero;
                          break;
        case DEV_RANDOM:
        case DEV_URANDOM: if_fop.read_func  = &read_dev_random;
                          if_fop.lseek_func = &lseek_dev_random;
                          break;

        case DEV_NULL: /*Will never be entered as /dev/null is handled via (MS-DOS) NUL:*/
        default:
          fprintf (stderr, "\n[%s] The Linux device file '%s' can only be written to!\n",
                              PROGRAM_NAME, file);
          ErrnoReturn (EINVAL, -1);
        }
      }
      /* Data sink: */
      else
      {
      /*hfile = DEVFILE_HANDLE_VALUE;*/  /*See: `if (of_fd.device_file > DEV_WINNATIVE)' */

        switch (of_fd.device_file)
        {
      /*// Commented out: we're now handling /dev/null via L"\\\\.\\NUL" */
        case MSDOS_NUL: /*of_fop.write_func = &write_dev_null;
                          of_fop.lseek_func = &lseek_dev_null;*/
                          break;

        case DEV_ZERO:
        case DEV_RANDOM:
        case DEV_URANDOM:
        default:
          fprintf (stderr, "\n[%s] The Linux device file '%s' can only be read from!\n",
                              PROGRAM_NAME, file);
          ErrnoReturn (EINVAL, -1);
        }
      }
    }
    break;

  case FDREOPEN_MSDOS_DEV:        /* CON:, NUL: */
    { wchar_t const *
        dev_file = MSDOSDeviceName (file, &device_file);

      _tcscpy (reopen_fd->file, dev_file);
      reopen_fd->drive_type = DTYPE_UNKNOWN;

      if (INPUT_FILE_FD)
      {
        /* Read from stdin instead, as CONIN$ is used to read user input (see `console_win32.c') */
        if (device_file == MSDOS_CON)
        {
          _tcscat (reopen_fd->file, L"IN$");
          set_fd_flags (STDIN_FILENO, input_flags, file/*input_file*/);  /* eventually calls back into `set_direct_io()' */
          assert (desired_fd == STDIN_FILENO);
          reopen_fd->valid = true;
          if (not INPUT_FILE_FD && always_confirm)
          {
            if (not UserConfirmOperation ())
              return (-1);
          }
          return (desired_fd);
        }
      }
      else
      {
        /* Write from stdin instead, as CONOUT$ is used to print screen output (see `console_win32.c') */
        if (device_file == MSDOS_CON)
        {
          _tcscat (reopen_fd->file, L"OUT$");
          set_fd_flags (STDOUT_FILENO, output_flags, file/*output_file*/);  /* eventually calls back into `set_direct_io()' */
          assert (desired_fd == STDOUT_FILENO);
          reopen_fd->valid = true;
          if (not INPUT_FILE_FD && always_confirm)
          {
            if (not UserConfirmOperation ())
              return (-1);
          }
          return (desired_fd);
        }
      }
    }
    break;

  case FDREOPEN_WINNT_DEVFILE:    /* \\?\GLOBALROOT\Device\Cdrom0 */
  case FDREOPEN_WDX_PHYCDR:       /* /dev/wr0  # \Device\CdRom0 */
    if (reopen_handler == FDREOPEN_WDX_PHYCDR)
    {
      if (not INPUT_FILE_FD)
      {
        fprintf (stderr, "\n[windd] Right now, this application does not offer any CD recording\n"
                         "capabilities (and maybe never will) ... aborting.\n",
                         PROGRAM_NAME);
        ErrnoReturn (EINVAL, -1);
      }
      else
      {
        if (mount_point == NULL || not IsDriveLetter (mount_point, true, NULL, NULL))
        {
          #if defined ASSIGN_TMP_DRIVE_LETTER
          /* Although working, this code path isn't used anymore, as CD/DVD/BD drives
          without a drive letter can much easier be accessed through the NT device
          namespace: \\?\GLOBALROOT\Device\Cdrom0
          */
          TCHAR
            device_path [MAX_PATH + 1];

          get_optdisk_path (device_path);

          if (AssignTempDriveLetter (device_path, &drive_letter) == 0)
          {
            reopen_fd->tmp_drive_letter[0] = *drive_letter;
            reopen_fd->tmp_drive_letter[1] = TEXT(':');
            reopen_fd->tmp_drive_letter[2] = TEXT('\0');
          }
          else
            return (-1);
          #else
          get_optdisk_path (reopen_fd->file);
          reopen_handler = FDREOPEN_WINNT_DEVFILE;
          #endif
        }
        /* fall through! */
      }
    }

  case FDREOPEN_WDX_PHYDSK:       /* /dev/wda       # \\.\PHYSICALDRIVE0 */
  case FDREOPEN_WDX_LOGDRV:       /* /dev/wda1 */
  case FDREOPEN_DRIVE_LETTER:     /* drive-letter:  # \\.\C: */
  case FDREOPEN_MOUNT_POINT:      /* d:\windd_drive -> f:\ */
    { struct VolumeDesc const *
        vol;

      if (reopen_handler == FDREOPEN_WDX_PHYDSK
          /* Only used if forced with a leading '!': */
       || reopen_handler == FDREOPEN_WDX_LOGDRV)
      {
        get_selected_diskpartition (&reopen_fd->disk_index, &reopen_fd->part_index);
        get_physdisk_path (reopen_fd->file);   /* "\\.\PHYSICALDRIVE0" */
        reopen_fd->drive_type = get_drive_type ();

        if (drives_partition_selected ())  /* /dev/wdX\d+ */
        {
          /* StartingOffset and PartitionLength */
          volume_offset = get_volume_offset ();
          volume_length = get_volume_size ();
        }
        else
        {
          /* Offsett 0 and DiskInfo::size_bytes */
          volume_offset = (off_t)0;
          volume_length = get_physdisk_size ();
        }
      }
      else
      {
        /* ... otherwise, the logical (Read|Write)File offset is 0: */
        volume_offset = (off_t)0;
        volume_length = get_volume_size ();

        /* "\\?\GLOBALROOT\Device\Harddisk0\Partition6" */
        if (reopen_handler == FDREOPEN_WINNT_DEVFILE)
        {
          if (drives_optdisk_selected ())
            get_optdisk_path (reopen_fd->file);
          else
          if (drives_partition_selected ())
            get_nt_harddiskvolume (reopen_fd->file);

          NormalizeNTDevicePath (reopen_fd->file);

          /* Don't display adjusted NT device name: */
          _tcscpy (reopen_fd->display_name, reopen_fd->file);
        }
        else
        /* \\.\C: */
        if (reopen_handler == FDREOPEN_DRIVE_LETTER || reopen_handler == FDREOPEN_WDX_PHYCDR)
        {
          if (_tcslen (mount_point) <= UNC_PHYS_LENGTH+2)
            _sntprintf (reopen_fd->file, 7, TEXT("\\\\.\\%c:"), *mount_point);
          else
            ThrowClassificationException (reopen_fd->file, reopen_handler);
        }
      }

      /* Did anything change, update display name? */
      if (_tcsicmp (reopen_fd->file, reopen_fd->display_name) != 0)
      {
        _tcscat (reopen_fd->display_name, TEXT(" => "));
        _tcscat (reopen_fd->display_name, reopen_fd->file);
      }

      /* Source equal to destination (if==of)? */
      reopen_fd->disk_access = true;

      if (not INPUT_FILE_FD && if_fd.disk_access
          && (reopen_fd->disk_index == if_fd.disk_index
           && reopen_fd->part_index == if_fd.part_index))
      {
        volume_handle             = GetWinHandle (STDIN_FILENO);
        reopen_fd->phys_sect_size = if_fd.phys_sect_size;
        src_eq_dst                = true;  /* Use `if_fd.handle' for of_fd as well! */
      }
      else
      {
        if ((vol=OpenVolumeRW (desired_fd, reopen_fd->file)) == NULL)
          return (-1);

        /* Dismount affected drives: */
        { struct DismountStatistics
            statistics;
          char
            key;

          bool successful = dismount_selected_volumes (vol,
                                reopen_fd->display_name,
                                reopen_handler == FDREOPEN_WDX_LOGDRV,  /* force_wdx_logdrive? */
                                INPUT_FILE_FD,                          /* read_only? */
                                &statistics
                              );
          reopen_fd->partition_handles = statistics.volume_handles;
          /* Failed or user decided not to continue? */
          if (not successful || not statistics.continue_op)
            ErrnoReturn (EACCES, -1);

          if (statistics.failed_to_lock || statistics.skipped_sysdrive || statistics.skipped_pagefile)
            if (not force_operations)
            {
              _ftprintf (stdout,
TEXT("\nSome of the attempted volume locks failed. Would you like to continue\n")
TEXT("nonetheless? (Y/N) "));
              key = PromptUserYesNo ();
              if (not IsFileRedirected (stdin))
              {
                _ftprintf (stdout, TEXT("%c\n\n"), key);
                FlushStdout ();
              }
              if (key == 'N')
                ErrnoReturn (EACCES, -1);
            }
        }

        if ((volume_handle=GetVolumeHandle (vol)) == INVALID_HANDLE_VALUE)
        {
          CloseVolume (&vol);
          SetLastError (ERROR_INVALID_HANDLE);
          WinErrReturn (-1);
        }

        /*O_DIRECT {*/
        /*if ((flags & O_DIRECT) == O_DIRECT)*/
        reopen_fd->dwDesiredAccess = GetVolumeDesiredAccess (vol);
        reopen_fd->dwFlagsAndAttributes = GetVolumeFlagsAndAttributes (vol);
        reopen_fd->phys_sect_size = PhysicalSectorSize (vol);
        /*O_DIRECT }*/

        CloseVolumeEx (&vol, false);
      }
    }

    /* We prefer to use O_DIRECT. However, as we have a locked volume
       which - in order to disable FILE_FLAG_NO_BUFFERING - can't (easily)
       be re-opened later on, we do so only if we know this will never
       result in a call to `set_direct_io(false)'. */
    unbuf_io_supp = (reopen_fd->phys_sect_size != 0);

    if (not INPUT_FILE_FD)
    { LARGE_INTEGER
        file_size;

      if (not if_fd.valid || not GetFileSizeEx (fdhandle [STDIN_FILENO], &file_size)
       || (file_size.QuadPart % (LONGLONG)output_blocksize) != 0)
      {
        unbuf_io_supp = false;
      }
    }
    break;

  case FDREOPEN_NETWORK_PATH:     /* \\server\path */
    /*
    unbuf_io_supp = false;
    // ... probably not a good idea in this case:
    // See "Problem with unbuffered IO over network",
    // http://www.devsuperpage.com/search/Articles.aspx?G=10&ArtID=197065
    */
    reopen_fd->drive_type = DTYPE_REMOTE;
    /* fall through! */

  case FDREOPEN_LOCAL_FILE:       /* \\.\D:\Image.dat */
    if ((reopen_fd->phys_sect_size=PhysicalSectorSizeEx (reopen_fd->file)) != 0)
    {
      unbuf_io_supp = true;
    }
    break;
  }
  #pragma endregion ExecuteFdreopenHandler


  /* If `conv=noerror,sync' is specified, `ibs' and `obs' should not be larger
     than the sector sizes of the involved devices. This ensures minimal data
     loss in case of actual read or write errors: */
  if ((conversions_mask & C_NOERROR) != 0 && not INPUT_FILE_FD)
  { TCHAR
      opt_bs_msg [368] = TEXT("[%s] When enabling the 'conv=noerror' option, please also make sure that\n")
                         TEXT("the block sizes are identical to the devices physical sector sizes. For the\n")
                         TEXT("current operation, ");
    int
      parts = 0;
    size_t
      length;

    if (if_fd.phys_sect_size != 0 && input_blocksize != if_fd.phys_sect_size)
    {
      length = _tcslen (opt_bs_msg);
      _sntprintf (opt_bs_msg+length, _countof(opt_bs_msg)-length,
                  TEXT("'ibs' should be set to %") TEXT(PRIu32) TEXT(" (instead of %") TEXT(PRIuMAX) TEXT(")"),
                  (uint32_t)if_fd.phys_sect_size, (uintmax_t)input_blocksize);
      ++parts;
    }

    if (of_fd.phys_sect_size != 0 && output_blocksize != of_fd.phys_sect_size)
    {
      if (parts)
        _tcscat (opt_bs_msg, TEXT(" and\n"));
      length = _tcslen (opt_bs_msg);
      _sntprintf (opt_bs_msg+length, _countof(opt_bs_msg)-length,
                  TEXT("'obs' should be set to %") TEXT(PRIu32) TEXT(" (instead of %") TEXT(PRIuMAX) TEXT(")"),
                  (uint32_t)of_fd.phys_sect_size, (uintmax_t)output_blocksize);
      ++parts;
    }

    if (parts)
    { char
        key;

      _tcscat (opt_bs_msg, TEXT(".\nWould you like to continue nonetheless? (Y/N) "));
      _ftprintf (stderr, opt_bs_msg, TEXT(PROGRAM_NAME));
      key = PromptUserYesNo ();
      if (not IsFileRedirected (stdin))
      {
        _ftprintf (stdout, TEXT("%c\n\n"), key);
        FlushStdout ();
      }
      if (key == 'N')
        ErrnoReturn (EINVAL, -1);
    }
  }


  /* (5) Unbuffered I/O requested or possible?
  //     https://msdn.microsoft.com/en-us/library/windows/desktop/cc644950(v=vs.85).aspx
  // As (disk) volumes are always opened with FILE_FLAG_NO_BUFFERING, the
  // following block of code is only really relevant for (regular) files!
  //
  // Preconditions:
  // i.  File access sizes, including the optional file offset in the
  //     OVERLAPPED structure, if specified, must be for a number of
  //     bytes that is an integer multiple of the volume sector size.
  //
  //     => This requirement is always satisifed as `ibuf' and `obuf'
  //        are aligned by `ptr_align (page_size);'
  //
  // ii. File access buffer addresses for read and write operations should
  //     be physical sector-aligned, which means aligned on addresses in
  //     memory that are integer multiples of the volume's physical sector
  //     size.
  //
  //     => Satisified, as long as sector size <= page size holds, see i.
  */
  #pragma region UnbufferedIOPrerequisites
  { bool
      direct_io = ((flags & O_DIRECT) == O_DIRECT);

    if (unbuf_io_supp && (direct_io || not disable_optimizations))
    {
      /* Data source? */
      if (INPUT_FILE_FD)
      {
        if_fd.direct_io = true;

        if (if_fd.phys_sect_size > 0 && input_blocksize >= if_fd.phys_sect_size && (input_blocksize % if_fd.phys_sect_size) == 0)
        {
          if_fd.unbuf_blocksize = input_blocksize;
          reopen_fd->dwFlagsAndAttributes |= FILE_FLAG_NO_BUFFERING;
          /* Try to use direct I/O, even if not specifically selected (i.e. direct_io==false) */
          set_fd_flags (STDIN_FILENO|CHGFLAGS_ONLY_FILENO, input_flags | O_DIRECT, file/*input_file*/);
          input_flags |= O_DIRECT;
        }
        else if (direct_io || volume_handle != NULL)  /* Only an error, if the user actually requested
                                                         direct I/O, or we're accessing a volume! */
        {
          fprintf (stderr, "\n[%s] input_blocksize(%" PRIuMAX ") %% phys_sect_size(%" PRIu32 ") != 0 ... aborting!\n",
                              PROGRAM_NAME, (uintmax_t)input_blocksize, (uint32_t)if_fd.phys_sect_size);
          ErrnoReturn (EINVAL, -1);
        }
      }
      /* Data sink: */
      else
      {
        if (of_fd.phys_sect_size > 0 && output_blocksize >= of_fd.phys_sect_size && (output_blocksize % of_fd.phys_sect_size) == 0)
        {
          /* Are unbuffered blocksizes a multiple of each other? */
          #ifdef REQUIRE_MULTIPLE_OF_BLOCKSIZES
          if (if_fd.unbuf_blocksize > 0)
          {
            DWORD max_blocksize = (DWORD)max (input_blocksize, output_blocksize),
                  min_blocksize = (DWORD)min (input_blocksize, output_blocksize);

            if ((max_blocksize % min_blocksize) == 0)
            {
              of_fd.unbuf_blocksize = output_blocksize;
              reopen_fd->dwFlagsAndAttributes |= FILE_FLAG_NO_BUFFERING;
              /* Try to use direct I/O, even if not specifically selected (i.e. direct_io==false) */
              set_fd_flags (STDOUT_FILENO|CHGFLAGS_ONLY_FILENO, output_flags | O_DIRECT, file/*output_file*/);
              output_flags |= O_DIRECT;
            }
            else if (unbuf_io_supp)  /* Only an error, if the user actually requested direct I/O! */
            {
              fprintf (stderr, "\n[%s] lcm(input_blocksize=%" PRIuMAX ",output_blocksize=%" PRIuMAX ")\n"
                                     " != max(input_blocksize%" PRIuMAX ",output_blocksize%" PRIuMAX ") ... aborting!\n",
                                  PROGRAM_NAME, (uintmax_t)input_blocksize, (uintmax_t)output_blocksize,
                                                (uintmax_t)input_blocksize, (uintmax_t)output_blocksize);
              ErrnoReturn (EINVAL, -1);
            }
          }
          #else
          of_fd.unbuf_blocksize = output_blocksize;
          reopen_fd->dwFlagsAndAttributes |= FILE_FLAG_NO_BUFFERING;
          /* Try to use direct I/O, even if not specifically selected (i.e. direct_io==false) */
          set_fd_flags (STDOUT_FILENO|CHGFLAGS_ONLY_FILENO, output_flags | O_DIRECT, file/*output_file*/);
          output_flags |= O_DIRECT;
          #endif
        }
        else if (direct_io || volume_handle != NULL)  /* Only an error, if the user actually requested
                                                         direct I/O, or we're accessing a volume! */
        {
          fprintf (stderr, "\n[%s] output_blocksize(%" PRIuMAX ") %% phys_sect_size(%" PRIu32 ") != 0 ... aborting!\n",
                              PROGRAM_NAME, (uintmax_t)output_blocksize, (uint32_t)of_fd.phys_sect_size);
          ErrnoReturn (EINVAL, -1);
        }
      }
    }
    else
    /* Unbuffered I/O is not supported: */
    if (direct_io)
    {
      fprintf (stderr, "\n[%s] Direct I/O only works for local volumes ... aborting:\n%s\n",
                          PROGRAM_NAME, file);
      ErrnoReturn (EINVAL, -1);
    }
  }
  #pragma endregion UnbufferedIOPrerequisites


  /* File descriptor / _WIN32 HANDLE: */
  /*
  // if ((flags & O_BINARY) == O_BINARY) can be ignored, as all I/O is binary by default!
  */
  { HANDLE
      hfile = INVALID_HANDLE_VALUE;
    int
      fd;

    /* (6) Open _WIN32 HANDLE: */
    #pragma region OpenWin32Handle
    if (device_file <= DEV_WINNATIVE)
    {
      /* Data source? */
      if (INPUT_FILE_FD)
      {
        /* Use opened volume, or */
        if (volume_handle != NULL)
        {
          hfile                          = volume_handle;
          if_fd.first_byte_index         = volume_offset;
          if_fd.absolute_offset.QuadPart = volume_offset;
          if_fd.last_byte_index          = (volume_offset + volume_length - (off_t)1);
        }
        else
        /* open regular file handle? */
        {
          reopen_fd->dwDesiredAccess = GENERIC_READ;        /* open for reading */

          /* See also `posix_win32.c:set_direct_io ()'! */
          hfile = CreateFile (reopen_fd->file,              /* file to open */
                              reopen_fd->dwDesiredAccess,
                              FILE_SHARE_READ,              /* share for reading */
                              NULL,                         /* default security */
                              OPEN_EXISTING,                /* existing file only */
                              reopen_fd->dwFlagsAndAttributes,
                              NULL);                        /* no attr. template */
        }

        if_fop.read_func  = &read_win32_readfile;
        if_fop.lseek_func = &lseek_win32_setfilepointer;
      }
      /* Data sink: */
      else
      {
        /* File and drive access on same volume? */
        if (not INPUT_FILE_FD && if_fd.disk_access != reopen_fd->disk_access)
          if (if_fd.disk_index == reopen_fd->disk_index
           && if_fd.part_index == reopen_fd->part_index)
          {
            _ftprintf (stderr, TEXT("\n[%s] The selected source and destination\n")
                               TEXT("%s\n")
                               TEXT("%s\n")
                               TEXT("translate to the same volume; this is not supported ... aborting!\n"),
                               TEXT(PROGRAM_NAME), if_fd.file, reopen_fd->file);
            exit (242);
          }

        /* Allocate a 64KiB aligned buffer if write optimization is enabled: */
        if (not disable_optimizations
            && (IsSolidStateDrive (reopen_fd->drive_type) && reopen_fd->seekable))
        {
          if (volume_handle != NULL || PathFileExists (reopen_fd->file))
          { SIZE_T
              max_blocksize, mem_pages;

            max_blocksize = max (input_blocksize, output_blocksize);
            mem_pages = (max_blocksize + page_size - 1) / page_size;

            reopen_fd->existing_data = (char *)VirtualAlloc (NULL, mem_pages * page_size,
                                                             MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (reopen_fd->existing_data == NULL)
            {
              fprintf (stderr, "\n[%s] %s:%" PRIu32 " VirtualAlloc (%" PRIuMAX " pages) failed ... aborting!\n",
                                   PROGRAM_NAME, __FILE__, (uint32_t)__LINE__,
                                   (uintmax_t)mem_pages);
              exit (253);
            }

            fprintf (stdout, "Write optimization enabled.\n");
            reopen_fd->attempt_write_opt = true;
          }
        }

        /* STDOUT_FILENO */
      /*of_fd.first_byte_index          =  0;*/
      /*of_fd.absolute_offset.QuadPart  =  0;*/
      /*of_fd.logical_offset            =  0;*/  /* initial value anyway */
        of_fd.last_byte_index           = -1;    /* no limitation at all */

        /* Use opened volume, or */
        if (volume_handle != NULL)
        {
          hfile                          = volume_handle;
          of_fd.first_byte_index         = volume_offset;
          of_fd.absolute_offset.QuadPart = volume_offset;
          of_fd.last_byte_index          = (volume_offset + volume_length - (off_t)1);
        }
        else
        /* open regular file handle? */
        {
          /* Source equal to destination (if==of)? */
          if (if_fd.file[0]!=TEXT('\0') && not if_fd.disk_access
              && (if_fd.disk_index == reopen_fd->disk_index
               && if_fd.part_index == reopen_fd->part_index)
              && PathEquals (if_fd.file, of_fd.file))
          {
            close (STDIN_FILENO);
            flags = (flags & ~O_WRONLY) | O_RDWR;
            src_eq_dst = true;
          }

          if ((flags & O_WRONLY) == O_WRONLY && not reopen_fd->attempt_write_opt)
          {
            reopen_fd->dwDesiredAccess = GENERIC_WRITE;     /* open for writing */
            reopen_fd->dwFlagsAndAttributes |= FILE_ATTRIBUTE_NORMAL;

            /* See also `posix_win32.c:set_direct_io ()'! */
            hfile = CreateFile (reopen_fd->file,            /* file to open */
                                reopen_fd->dwDesiredAccess,
                                0,                          /* do not share */
                                NULL,                       /* default security */
                                CREATE_ALWAYS,              /* create always (truncate existing) */
                                reopen_fd->dwFlagsAndAttributes,
                                NULL);                      /* no attr. template */
          }
          else

          if ((flags & O_RDWR) == O_RDWR || reopen_fd->attempt_write_opt)
          {
            reopen_fd->dwDesiredAccess = GENERIC_READ
                                       | GENERIC_WRITE;     /* open for R/W */

            /* See also `posix_win32.c:set_direct_io ()'! */
            hfile = CreateFile (reopen_fd->file,            /* file to open */
                                reopen_fd->dwDesiredAccess,
                                0,                          /* do not share */
                                NULL,                       /* default security */
                                OPEN_EXISTING,              /* existing file only */
                                reopen_fd->dwFlagsAndAttributes,
                                NULL);                      /* no attr. template */
          }
          else
            ErrnoReturn (EINVAL, -1);
        }

        of_fop.write_func = &write_win32_writefile;
        of_fop.lseek_func = &lseek_win32_setfilepointer;
      }

      /* Successful? */
      if (hfile == INVALID_HANDLE_VALUE)
        WinErrReturn (-1);
    }
    else
    {
      /* /dev/zero, /dev/random, /dev/urandom */
      hfile = DEVFILE_HANDLE_VALUE;
    }
    #pragma endregion OpenWin32Handle


    /* (7) Assign a (dummy) file descriptor: */
    fd = newfd (hfile);

    if (desired_fd >= 0 && fd != desired_fd)
    {
      if (dup2 (fd, desired_fd) < 0)
        return (-1);
      remfd (fd);
      fd = desired_fd;
    }

    reopen_fd->pipe_buffer_size = get_pipe_buffsize (fd);

    /* Source equal to destination (if==of)? */
    if (src_eq_dst)
      if (fdhandle [STDIN_FILENO] == NULL)
        fdhandle [STDIN_FILENO] = fdhandle [fd];

    if (not INPUT_FILE_FD && always_confirm)
    {
      if (not UserConfirmOperation ())
        return (-1);
    }

    reopen_fd->valid = true;
    return (fd);
  }
}


/* Windows formats NTFS partitions in such a way, that the last cluster
   isn't used by the filesystem. Instead, this cluster holds a copy of
   the filesystems boot sector.

   As a result of this, we won't be able to read/write the last cluster
   of a partition, if the partition contains a valid NTFS filesystem and
   the volume handle was opened using a drive letter or a corresponding
   NT device name.

   If the user specifies i. a /dev/wdx identifier, or ii. a Windows
   NT device name, we could, of course, just open \\.\PHYSICALDRIVE and
   access the referenced partition directly.

   We currently don't do so, however. (Unless the users forces us by
   prepending the /dev/wdx identifier with a !)
*/
static enum EReopenHandler DetermineReopenHandler (char const *fileA, TCHAR fileW [], int const flags,
                                                   TCHAR const **mount_point, enum EDeviceFile *device_file)
{ enum EReopenHandler
    reopen_handler = FDREOPEN_FILE_INVALID;
  bool
    force_wdx_logdrive = false;

 *mount_point = NULL;

  /* Special case: forced `/dev/wdx' access? (E.g. "!/dev/wda1") */
  if (_memicmp (fileA, "!/dev/wd", 8) == 0)
  {
    force_wdx_logdrive = true;
    ++fileA;
  }

  /* (1) Linux device file? */
  if ((*device_file=LinuxDeviceName (fileA)) != DEV_NONE)
  {
    if (*device_file == DEV_INVALID)
    {
      fprintf (stderr, "\n[%s] Unrecognized Linux device file '%s' ... aborting!\n",
                          PROGRAM_NAME, fileA);
      return (FDREOPEN_EINVAL);  /*ErrnoReturn (EINVAL, -1);*/
    }

    /* >= (4) /dev/wdx device file? */
    if (*device_file == EMULATED_WDX)
    {
      if (not select_drive_by_wdxid (fileA, mount_point))
        return (FDREOPEN_FILE_INVALID);  /*return (-1);*/

      /* Ignore ! in front if disk identifiers, e.g. "!/dev/wda": */
      if (force_wdx_logdrive && drives_partition_selected ())
        return (FDREOPEN_WDX_LOGDRV);

      /* Rule set:
      1. /dev/wdx[0-9] is mapped to a drive letter?
         a. Harddisk:  FDREOPEN_DRIVE_LETTER (A: to Z:)
         b. CD-ROM:    FDREOPEN_WDX_PHYCDR (semantically identical to `FDREOPEN_DRIVE_LETTER', if there is a drive letter)

      2. /dev/wdx[0-9] has no associated drive letter: FDREOPEN_WINNT_DEVFILE
         a. Harddisk:  "\\?\GLOBALROOT\Device\HarddiskVolume1"
         b. CD-ROM:    "\\?\GLOBALROOT\Device\Cdrom0"

      3. Access to all sectors of a physical harddisk? FDREOPEN_WDX_PHYDSK
         Harddisk:  "\\.\PHYSICALDRIVE0"
      */

      /* Ad 1.b. CD/DVD/BD drive letter? */
      if (drives_optdisk_selected ())
      {
        /* Open question with regard to the function comment. Wouldn't
           it be better to just return FDREOPEN_WDX_LOGDRV instead? */
    /* *mount_point = NULL; */  /* Disable mapping to drive letter. */
        return (FDREOPEN_WDX_PHYCDR);
      }

      if (drives_partition_selected ())
      {
        /* Ad 2.a/b. No associated drive letter? */
        if (*mount_point==NULL || (*mount_point)[0]==TEXT('\0'))
        {
          if (get_nt_harddiskvolume (NULL))
            return (FDREOPEN_WINNT_DEVFILE);
          else
            return (FDREOPEN_WDX_LOGDRV);
        }

        /* Ad 1.a. Windows partition: */
        if (IsDriveLetter (*mount_point, true, NULL, NULL))
        {
          /* Open question with regard to the function comment. Wouldn't
             it be better to just return FDREOPEN_WDX_LOGDRV instead? */
      /* *mount_point = NULL; */  /* Disable mapping to drive letter. */
          return (FDREOPEN_DRIVE_LETTER);  /* Alternatively: return (FDREOPEN_WINNT_DEVFILE); */
        }
        else
        {
          if (select_drive_by_mountpoint (*mount_point))
          {
            _tcscpy (fileW, *mount_point);
           *device_file = DEV_NONE;
            return (FDREOPEN_MOUNT_POINT);
          }
          else
            return (FDREOPEN_WINNT_DEVFILE);
        }
      }

      /* Ad 3. Physical drive: */
      if (drives_physdisk_selected ())
        return (FDREOPEN_WDX_PHYDSK);
      /*
      else  // Can't (and shouldn't) happen!
        ;
      */
    }
    else
      return (FDREOPEN_LINUX_DEV);
  }
  else
  {
    /* (2) MS-DOS device file? */
    (void) MSDOSDeviceName (fileA, device_file);

    if (*device_file != DEV_NONE)
    {
      if (INPUT_FILE_FLAGS)
      {
        if (*device_file >= DEV_WO)
        {
          fprintf (stderr, "\n[%s] The %sdevice file '%s' can only be written to!\n",
                              PROGRAM_NAME, (strchr(fileA,'/')!=NULL?"Linux ":"MS-DOS "), fileA);
          return (FDREOPEN_EINVAL);  /*ErrnoReturn (EINVAL, -1);*/
        }
      }
      else
      {
        if (*device_file <= DEV_RO)
        {
          fprintf (stderr, "\n[%s] The MS-DOS device file '%s' can only be read from!\n",
                              PROGRAM_NAME, fileA);
          return (FDREOPEN_EINVAL);  /*ErrnoReturn (EINVAL, -1);*/
        }
      }

      return (FDREOPEN_MSDOS_DEV);
    }
    /* 3. NT device namespace? */
    else
    if (NTDeviceNameA (fileA))
    {
     *device_file = NT_DEVICE;

      if (select_drive_by_devicename (fileW, mount_point))
      { enum EDriveType
          drive_type = get_drive_type ();

        /* Don't use a drive letter, if the user wants the access
           to occur via a named device object. */
       *mount_point = NULL;

        if (IsOpticalDisk (drive_type))
        {
        /*SelectMountPointByIndex (0, mount_point);*/
          return (FDREOPEN_WDX_PHYCDR);
        }
        else
        {
        /*return (FDREOPEN_WDX_PHYDSK);*/  /* Fall through! */
        }
      }

      /* Ignore error; pass `NT device name' through to CreateFile()! */
      return (FDREOPEN_WINNT_DEVFILE);
    }
    /* Local or network path? */
    else
    {
      if (not IsDriveLetter (fileW, true, NULL, NULL) && IsNetworkPath (fileW))
        return (FDREOPEN_NETWORK_PATH);
    }
  }

  return (FDREOPEN_LOCAL_FILE);
}


static void ThrowClassificationException (TCHAR const reopen_file [], enum EReopenHandler const reopen_handler)
{
  /* This is to guard against classification errors and should never happen! */
  _ftprintf (stderr, TEXT("\n[%s] The user specified the following filename:\n")
                     TEXT("%s\n")
                     TEXT("Cowardly refusing to change the fd_reopen() handler from\n")
                     TEXT("%s to FDREOPEN_DRIVE_LETTER ... aborting!\n"),
                     TEXT(PROGRAM_NAME), reopen_file,
                     ((reopen_handler == FDREOPEN_LOCAL_FILE)? TEXT("FDREOPEN_LOCAL_FILE")
                                                             : TEXT("FDREOPEN_NETWORK_PATH")));
  exit (243);
}


static bool UserConfirmOperation (void)
{ TCHAR
    records [32] = TEXT(""),
    range1 [64] = TEXT(""),
    range2 [64] = TEXT("");
  char
    key;

  if (max_records != (uintmax_t)-1)
    _sntprintf (records, _countof(records), TEXT("%") TEXT(PRIuMAX) TEXT(" "), max_records);
  if (if_fd.last_byte_index > 0)
    _sntprintf (range1, _countof(range1), TEXT(" (%18") TEXT(PRIuMAX) TEXT(",%18") TEXT(PRIuMAX) TEXT(")"),
                                          (uintmax_t)if_fd.absolute_offset.QuadPart,
                                          (uintmax_t)(if_fd.last_byte_index-if_fd.first_byte_index+1));
  if (of_fd.last_byte_index > 0)
    _sntprintf (range2, _countof(range2), TEXT(" (%18") TEXT(PRIuMAX) TEXT(",%18") TEXT(PRIuMAX) TEXT(")"),
                                          (uintmax_t)of_fd.absolute_offset.QuadPart,
                                          (uintmax_t)(of_fd.last_byte_index-of_fd.first_byte_index+1));

  /* Ask the user, if he's happy with our choices! */
  textcolor (LIGHTGREEN);
  fprintf (stdout, "[Confirmation] ");
  textcolor (DEFAULT_COLOR);
  _ftprintf (stdout, TEXT("The operation will copy %srecord(s) from\n"), records);
  if (range1[0] != TEXT('\0'))
    _ftprintf (stdout, TEXT("%-39.39s%s\nto\n"), if_fd.file, range1);
  else
    _ftprintf (stdout, TEXT("%s\nto\n"), if_fd.file);
  if (range2[0] != TEXT('\0'))
    _ftprintf (stdout, TEXT("%-39.39s%s\n"), of_fd.file, range2);
  else
    _ftprintf (stdout, TEXT("%s\n"), of_fd.file);
  _ftprintf (stdout, TEXT("Are you sure you want to continue? (Y/N) "));
  key = PromptUserYesNo ();

  if (not IsFileRedirected (stdin))
  {
    _ftprintf (stdout, TEXT("%c\n\n"), key);
    FlushStdout ();
  }
  if (key == 'N')
    ErrnoReturn (EPERM, false);

  return (true);
}
