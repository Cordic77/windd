#include "win32utf16.h"           /* switch to UTF-16 in this file! */

#include <config.h>

#include "win32\path_win32.h"     /* DEV_NONE, EMULATED_WDX*/
#include "win32\raw_win32.h"      /* OpenVolume(), GetVolumeHandle(), IsValidHandle() */

/* Additional _WIN32 headers: */
#include <shlwapi.h>              /* PathIsUNC() */


/* `dd.c' global variables: */
extern char const
 *input_file,
 *output_file;
extern size_t
  page_size;
extern size_t
  input_blocksize,
  output_blocksize;
extern uintmax_t
  skip_records,
  seek_records;
extern uintmax_t
  max_records;
extern size_t
  max_bytes;
extern int
  conversions_mask;
extern int
  input_flags,
  output_flags;


/* */
static HANDLE
  fdhandle [MAX_OPEN_DESCRIPTORS] = {NULL};

#define HANDLEARR_SIZE        _countof(fdhandle)
#define DEVFILE_HANDLE_VALUE  ((HANDLE)(LONG_PTR)-2)
#define IsValidFdesc(fd)      (fd <= STDERR_FILENO || IsValidHandle(fdhandle [fd]))
#define GetWinHandle(fd)      (IsValidHandle(fdhandle[fd])? fdhandle[fd] : INVALID_HANDLE_VALUE)


/* Additional information about if= and of= */
struct FileDesc
{
  bool                  valid;                  /* Valid `struct FileDesc'? */
  uint32_t              disk_index;             /* Must be <>INVALID_DISK */
  uint32_t              part_index;             /* For HDDs and SSDs, this too must be <>INVALID_PART */
  HANDLE               *partition_handles;      /* Partition lock HANDLEs */

  TCHAR                 file [MAX_UNCPATH + 1]; /* Normalized UNC path */
  DWORD                 dwDesiredAccess;        /* RO, WO or R/W? */
  DWORD                 dwFlagsAndAttributes;   /* Flags passed to CreateFile() */
  TCHAR                 display_name [MAX_UNCPATH + 1]; /* Name to display to the user */
  bool                  disk_access;            /* true: physical disk or logical volume; false: regular file */
  uint32_t              pipe_buffer_size;       /* 0: not connected to a pipe; lpIn/OutBufferSize otherwise */

  enum EDeviceFile      device_file;            /* Linux/MS-DOS device file? */
  DWORD                 phys_sect_size;         /* Physical sector size (may be != 512 bytes) */
  bool                  seekable;               /* Is the device seekable? */
  /* Unbuffered I/O: https://msdn.microsoft.com/en-us/library/windows/desktop/cc644950(v=vs.85).aspx */
  bool                  direct_io;              /* Direct I/O enabled for this device? */
  size_t                unbuf_blocksize;        /* [Unused] equal to `ibs' or `obs', respectively. */

  enum EDriveType       drive_type;             /* See `dskmgmt_win32.c:enum EDriveType'! */
  #if defined ASSIGN_TMP_DRIVE_LETTER
  TCHAR                 tmp_drive_letter [4];   /* CD/DVD/BD drives: temporary assigned drive letter */
  #endif
  bool                  attempt_write_opt;      /* Try to optimize superfluous writes away? */
  char                 *existing_data;          /* WriteFile(): compare `write buffer' to 'existing data' */
  uintmax_t             r_total, r_written;     /* Records in total / records actually written */

  off_t                 first_byte_index;       /* Absolute offset we started at:    0 <or> PartitionEntry.StartingOffset.QuadPart */
  LARGE_INTEGER         absolute_offset;        /* Current absolute volume offset:   0 <or> PartitionEntry.StartingOffset.QuadPart */
  off_t                 logical_offset;         /* Current logical volume offset:    0  (from the point of where we started) */
  off_t                 last_byte_index;        /* Last valid absolute byte offset: -1 <or> DiskInfo.size_bytes <or> PartitionEntry.PartitionLength.QuadPart */
};

static struct FileDesc const
  null_fd;    /* = {TEXT(""),DEV_NONE,0,false,0}; */

struct FileDesc
  if_fd,      /* = {TEXT(""),DEV_NONE,0,false,0};   // input file */
  of_fd;      /* = {TEXT(""),DEV_NONE,0,false,0};   // output file */

static bool
  src_eq_dst = false;             /* if==of? */

/*#define REQUIRE_MULTIPLE_OF_BLOCKSIZES 1  // unnecessary!*/


/*===  WIN32 HANDLE <-> FILE DESCRIPTOR  ===*/
extern int newfd_ex (HANDLE handle, bool from_end)
{ size_t
    i;

  if (not IsValidHandle (handle))
    return (-1);

  if (from_end)
  for (i= HANDLEARR_SIZE-1; i < HANDLEARR_SIZE; --i)
  {
      if (fdhandle[i] == NULL)
        break;
  }
  else
    for (i=0; i < HANDLEARR_SIZE; ++i)
    {
      if (fdhandle [i] == NULL)
        break;
    }

  fdhandle [i] = handle;
  return ((int)i);
}

extern int newfd (HANDLE handle)
{
  return (newfd_ex (handle, false));
}


extern int handle2fd (HANDLE handle)
{ size_t
    i = HANDLEARR_SIZE;

  if (IsValidHandle (handle))
  {
    for (i = 0; i < HANDLEARR_SIZE; ++i)
    {
      if (fdhandle [i] == handle)
        break;
    }
  }

  if (i >= HANDLEARR_SIZE)
  {
    fprintf (stderr, "\n[%s] handle2fd(): invalid handle.\n",
                         PROGRAM_NAME);
    exit (250);
  }

  return ((int)i);
}


extern HANDLE fd2handle (int fd)
{
  HANDLE handle = fdhandle [fd];

  if (not IsValidHandle (handle))
  {
    fprintf (stderr, "\n[%s] fd2handle(): invalid file descriptor.\n",
                         PROGRAM_NAME);
    exit (250);
  }

  return (handle);
}


extern void remfd (int fd)
{
  if (not IsValidHandle (fdhandle [fd]))
  {
    fprintf (stderr, "\n[%s] remfd(): invalid file descriptor.\n",
                         PROGRAM_NAME);
    exit (250);
  }

  fdhandle [fd] = NULL;
}


/*===  struct FileDesc Helpers  ===*/
static inline struct FileDesc *get_file_desc (int fd)
{
  if (fd == STDIN_FILENO)
    return (&if_fd);
  else
  if (fd == STDOUT_FILENO)
    return (&of_fd);
  else
  {
    fprintf (stderr, "\n[%s] get_file_desc(): invalid file descriptor %" PRId32 "\n",
                         PROGRAM_NAME, fd);
    exit (249);
  }
}


static inline bool is_std_file_ex (struct FileDesc const *file_desc)
{
  /* If `file_desc' is NULL, always assume stdin/stdout: */
  if (file_desc == NULL)
    return (true);

  /* stdin/stdout? */
  if (memcmp (file_desc, &null_fd, sizeof(null_fd)) == 0)
    return (true);

  /* CONIN$/CONOUT$ (which is mapped to stdin/stdout)? */
  if (_tcscmp (file_desc->file, L"CONIN$") == 0
   || _tcscmp (file_desc->file, L"CONOUT$") == 0)
    return (true);

  return (false);
}

extern bool is_std_file (int fd)
{ struct FileDesc const *
    file_desc = get_file_desc (fd);

  return (is_std_file_ex (file_desc));
}


static inline bool is_regular_file_ex (struct FileDesc const *file_desc)
{
  /* If not a disk volume, last_byte_index is always < zero: */
  return (file_desc->last_byte_index < 0ll && file_desc->device_file == DEV_NONE);
}

extern bool is_regular_file (int fd)
{ struct FileDesc const *
    file_desc = get_file_desc (fd);

  if (is_std_file_ex (file_desc))
    return (false);

  return (is_regular_file_ex (file_desc));
}


static inline bool is_device_file_ex (struct FileDesc const *file_desc)
{
  /* If not a disk volume, last_byte_index is always < zero: */
  return (file_desc->last_byte_index < 0ll && file_desc->device_file > DEV_NONE);
}

extern bool is_device_file (int fd)
{ struct FileDesc const *
    file_desc = get_file_desc (fd);

  if (is_std_file_ex (file_desc))
    return (false);

  return (is_device_file_ex (file_desc));
}


static inline bool is_volume_access_ex (struct FileDesc const *file_desc)
{
  /* In case of disk volumes, last_byte_index is always >= zero: */
  return (file_desc->device_file == EMULATED_WDX); /* && file_desc->last_byte_index >= 0ll); */
}

extern bool is_volume_access (int fd)
{ struct FileDesc const *
    file_desc = get_file_desc (fd);

  if (is_std_file_ex (file_desc))
    return (false);

  return (is_volume_access_ex (file_desc));
}


extern uint32_t get_pipe_buffsize (int fd)
{
  if (GetFileType (fdhandle [fd]) == FILE_TYPE_PIPE)
  { DWORD
      in_buffer_size,
      out_buffer_size;

    if (GetNamedPipeInfo (fdhandle [fd], NULL, &out_buffer_size, &in_buffer_size, NULL))
    {
      if (fd == STDIN_FILENO)
      {
        if (input_blocksize > in_buffer_size)
        {
          fprintf (stderr, "\n[%s] Pipe uses an input buffer size of %" PRIu32 ". Please\n"
                           "adjust ibs= accordingly ... aborting!\n",
                           PROGRAM_NAME, (uint32_t)in_buffer_size);
          exit (240);
        }

        return (in_buffer_size);
      }
      else
      if (fd == STDOUT_FILENO)
      {
        if (output_blocksize > out_buffer_size)
        {
          fprintf (stderr, "\n[%s] Pipe uses an output buffer size of %" PRIu32 ". Please\n"
                           "adjust obs= accordingly ... aborting!\n",
                           PROGRAM_NAME, (uint32_t)out_buffer_size);
          exit (240);
        }

        return (out_buffer_size);
      }
    }
    else
    {
      fprintf (stderr, "\n[%s] GetNamedPipeInfo() failed for anonymous pipe ... aborting!\n",
               PROGRAM_NAME);
      exit (240);
    }

    return (65536);  /* By default, anonymous pipes have a buffer size of 64 KiB! */
  }

  return (0);
}


/*===  LINUX (POSIX) FUNCTIONS  ===*/
extern int set_direct_io (int fd, bool direct)
{ struct FileDesc *
    fildes = NULL;

  int real_fd = (fd & CHGFLAGS_FILDES_MASK);

  fildes = get_file_desc (real_fd);
  fildes->direct_io = direct;

  /* In current `windd' versions, Windows HANDLEs are used for everything. */
  /*
  if (real_fd <= STDERR_FILENO)  // fd != real_fd? Only change flags!
  { FILE *stream;

    switch (real_fd)
    {
    case STDIN_FILENO:  stream = stdin; break;
    case STDOUT_FILENO: stream = stdout; break;
    case STDERR_FILENO: stream = stderr; break;
    }

    if (direct)
      return (setvbuf (stream, NULL, _IONBF, BUFSIZ));
    else
      return (setvbuf (stream, NULL, _IOFBF, BUFSIZ));
  }
  */

  /* Do we have a valid file descriptor? */
  if (fd <= STDERR_FILENO)  /* fd != real_fd? Only change flags! */
  {
    if (not IsValidHandle (fdhandle [real_fd]))
      switch (real_fd)
      {
      case STDIN_FILENO:  fdhandle [real_fd] = GetStdHandle (STD_INPUT_HANDLE); break;
      case STDOUT_FILENO: fdhandle [real_fd] = GetStdHandle (STD_OUTPUT_HANDLE); break;
    /*case STDERR_FILENO: fdhandle [real_fd] = GetStdHandle (STD_ERROR_HANDLE); break;*/
      }

    if (not IsValidHandle (fdhandle [real_fd]))
    {
      fprintf (stderr, "\n[%s] set_direct_io(): invalid handle.\n",
                           PROGRAM_NAME);
      exit (248);
    }

    fildes->pipe_buffer_size = get_pipe_buffsize (real_fd);
  }

  /* Disable caching: */
  if (fildes->disk_access)  /* Volume access? */
  {
    /* Do nothing: writes <> sector size are handled in `write_win32_writefile()'! */
  }
  else
  {
    if (fd == STDIN_FILENO)  /* fd != real_fd? Only change flags! */
    {
      /* r_partial records can't be read with FILE_FLAG_NO_BUFFERING enabled: */
      if (fildes->valid && direct != ((fildes->dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING)!=0))
      {
        if (direct)
          fildes->dwFlagsAndAttributes |= FILE_FLAG_NO_BUFFERING;
        else
          fildes->dwFlagsAndAttributes &= ~FILE_FLAG_NO_BUFFERING;

        #if defined(_DEBUG)
        _ftprintf (stdout, TEXT("DEBUG: reopening %s for input: %s\n"), fildes->file,
                           ((fildes->dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING)?
                            TEXT("O_DIRECT == 1") : TEXT("O_DIRECT == 0")));
        FlushStdout ();
        #endif

        FlushFileBuffers (fdhandle [real_fd]);
        #if 0  /* Use `ReOpenFile()' instead? */
        CloseHandle (fdhandle [real_fd]);
        fdhandle [real_fd] = CreateFile (fildes->file,  /* file to open */
                                fildes->dwDesiredAccess,
                                FILE_SHARE_READ,        /* share for reading */
                                NULL,                   /* default security */
                                OPEN_EXISTING,          /* existing file only */
                                fildes->dwFlagsAndAttributes,
                                NULL);                  /* no attr. template */
        #else
        fdhandle [real_fd] = ReOpenFile (fdhandle[real_fd],
                                fildes->dwDesiredAccess,
                                FILE_SHARE_READ,        /* share for reading */
                                fildes->dwFlagsAndAttributes);
        if (not IsValidHandle (fdhandle[real_fd]))
          WinErrReturn (-1);
        #endif
      }
    }
    else
    if (fd == STDOUT_FILENO)  /* fd != real_fd? Only change flags! */
    {
      /* r_partial records can't be written with FILE_FLAG_NO_BUFFERING enabled: */
      if (fildes->valid && direct != ((fildes->dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING)!=0))
      {
        if (direct)
          fildes->dwFlagsAndAttributes |= FILE_FLAG_NO_BUFFERING;
        else
          fildes->dwFlagsAndAttributes &= ~FILE_FLAG_NO_BUFFERING;

        #if defined(_DEBUG)
        _ftprintf (stdout, TEXT("DEBUG: reopening %s for output: %s\n"), fildes->file,
                           ((fildes->dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING)?
                            TEXT("O_DIRECT == 1") : TEXT("O_DIRECT == 0")));
        FlushStdout ();
        #endif

        FlushFileBuffers (fdhandle [real_fd]);
        #if 1  /* Use `ReOpenFile()' instead? */
        CloseHandle (fdhandle [real_fd]);
        fdhandle [real_fd] = CreateFile (fildes->file,  /* file to open */
                                fildes->dwDesiredAccess,
                                0,                      /* do not share */
                                NULL,                   /* default security */
                                OPEN_EXISTING,          /* existing file only */
                                fildes->dwFlagsAndAttributes,
                                NULL);                  /* no attr. template */
        #else
        /* ERROR_SHARING_VIOLATION : The process cannot access the file because it is being used by another process. (?) */
        fdhandle [real_fd] = ReOpenFile (fdhandle[real_fd],
                                fildes->dwDesiredAccess,
                                0,                      /* do not share */
                                fildes->dwFlagsAndAttributes);
        #endif
        if (not IsValidHandle (fdhandle[real_fd]))
          WinErrReturn (-1);
      }
    }
  }

  return (0);
}


extern int dup2 (int fildes, int fildes2)
{
  if (fildes == fildes2)
    return (fildes);

  /* `fildes' valid? */
  if (fdhandle [fildes] == NULL)
    ErrnoReturn (EBADF, -1);

  /* `fildes2' in use? -- Not POSIX conform, but closer
     to the behaviour our emulation requires: */
  if (fdhandle [fildes2] != NULL)
    ErrnoReturn (EBADF, -1);

  fdhandle [fildes2] = fdhandle [fildes];
/*fdhandle [fildes]  = NULL;  // Require explicit remfd() call!*/
  fildes = fildes2;

  return (fildes);
}


extern int
  fd_reopen (int desired_fd, char const *file, int flags, mode_t mode);
#include "fdreopen_win32.c"


extern int close (int fd)
{ HANDLE
    handle = fdhandle [fd];

  if (IsValidHandle (handle))
  {
    if (fd <= STDOUT_FILENO)
    { struct FileDesc *
        close_fd = get_file_desc (fd);

      /* Write optimization enabled? */
      if (close_fd->attempt_write_opt)
      { uintmax_t
          saved_bytes = (close_fd->r_total - close_fd->r_written) * output_blocksize;

        fprintf (stdout, "%" PRIuMAX " record(s) different, saved %" PRIuMAX " MiB",
                         close_fd->r_written, (saved_bytes + 524288) >> (uintmax_t)20);

        if (close_fd->r_total > 0)
          fprintf (stdout, " = %5.2lf%%\n", ((double)saved_bytes / (close_fd->r_total * output_blocksize)) * 100.0);
        else
          fprintf (stdout, "\n");

        VirtualFree (close_fd->existing_data, 0, MEM_RELEASE);
        close_fd->existing_data = NULL;

        close_fd->valid = false;
      }

      /* Unlock CD/DVD/BD drive? */
      if (IsOpticalDisk (close_fd->drive_type))
      {
        LockCdromDriveEx (handle, FALSE);
      }

      /* Delete (temporary) drive letter? */
      #if defined ASSIGN_TMP_DRIVE_LETTER
      if (close_fd->tmp_drive_letter[0] != TEXT('\0'))
      {
        DeleteVolumeMountPoint (close_fd->tmp_drive_letter);
        close_fd->tmp_drive_letter[0] = TEXT('\0');
      }
      #endif

      /* Close Partition lock HANDLEs: */
      if (close_fd->partition_handles != NULL)
      { size_t
          i;

        for (i=0; close_fd->partition_handles [i] != NULL; ++i)
          CloseHandle (close_fd->partition_handles [i]);

        Free (close_fd->partition_handles);
      }
    }

    /* Close Windows HANDLE: */
    { HRESULT
        hr;

      remfd (fd);

      if (not IsValidHandle (handle))
        ErrnoReturn (EBADF, -1);

      if (handle != DEVFILE_HANDLE_VALUE)
      {
        if (fd==STDOUT_FILENO && src_eq_dst)
          ;  /* if==of? Close HANDLE only once! */
        else
        {
          hr = CloseHandle (handle);
          if (FAILED (hr))
            ComErrReturn (NULL, hr, -1);
        }
      }
    }
  }

  /* Flush stdio buffers as soon as the first file is closed: */
  if (fd == STDIN_FILENO)
  {
    FlushStdout ();
    FlushStderr ();
  }

  return (0);
}


/*#define lseek(fd,offset,whence) _lseeki64(fd,(__int64)offset, whence)*/
extern off_t lseek_win32_setfilepointer (int fd, off_t offset, int whence)
{ DWORD
    dwMoveMethod;
  struct FileDesc *
    file_desc = get_file_desc (fd);

  { HANDLE
      handle = GetWinHandle (fd);

    /* Adjust file pointer: */
    switch (whence)
    {
    case SEEK_SET:
      dwMoveMethod = FILE_BEGIN;
      file_desc->absolute_offset.QuadPart = file_desc->first_byte_index;
      file_desc->logical_offset           = (off_t)0;
      break;

    case SEEK_CUR:
      dwMoveMethod = FILE_CURRENT;
      file_desc->absolute_offset.QuadPart = (file_desc->absolute_offset.QuadPart + offset);
      file_desc->logical_offset           = (file_desc->logical_offset + offset);
      break;

    case SEEK_END: dwMoveMethod = FILE_END;
      if (is_volume_access (fd))
      {
        file_desc->absolute_offset.QuadPart = file_desc->last_byte_index;
        file_desc->logical_offset           = (file_desc->last_byte_index - file_desc->first_byte_index);
      }
      else /*if (is_regular_file (fd))*/
      { LARGE_INTEGER
          file_size;
        if (not GetFileSizeEx (handle, &file_size))
          WinErrReturn ((off_t)-1);
        file_desc->absolute_offset.QuadPart = (file_size.QuadPart - (LONGLONG)1);
        file_desc->logical_offset           = (file_desc->absolute_offset.QuadPart - file_desc->first_byte_index);
      }
      break;

    default:
      ErrnoReturn (EINVAL, (off_t)-1);
    }

    if (not SetFilePointerEx (handle, file_desc->absolute_offset, NULL, dwMoveMethod))
      WinErrReturn ((off_t)-1);

    return (file_desc->logical_offset);
  }
}


extern ssize_t read_win32_readfile (int fd, void *buf, size_t count)
{ struct FileDesc *
    read_fd = get_file_desc (fd);

  if (is_volume_access_ex (read_fd) && read_fd->last_byte_index > 0)
  { off_t remaining = (read_fd->last_byte_index
                       - read_fd->absolute_offset.QuadPart + (off_t)1);

    if (remaining < (off_t)count)
    {
      if (remaining == (off_t)0)
        return ((ssize_t)0);

      count = (size_t)remaining;
    }

  /*#ifdef _DEBUG*/  /* Always included for now! */
    if (read_fd->absolute_offset.QuadPart < read_fd->first_byte_index
     || read_fd->absolute_offset.QuadPart > read_fd->last_byte_index)
    {
      fprintf (stderr, "\n[%s] win32_readfile(): absolute offset %" PRIdMAX " not with-\n"
                       "in expected partition bounds [%25" PRIdMAX ", %20" PRIdMAX "]\n"
                       "... aborting!\n",
                       PROGRAM_NAME, (intmax_t)read_fd->absolute_offset.QuadPart,
                       (intmax_t)read_fd->first_byte_index, (intmax_t)read_fd->last_byte_index);
      exit (249);
    }
  /*#endif*/
  }

  if (count > 0)
  { HANDLE
      handle = GetWinHandle (fd);
    size_t
      left = count;
    DWORD
      read;

    /*lseek {*/
    if (not SetFilePointerEx (handle, read_fd->absolute_offset, NULL, FILE_BEGIN))
      WinErrReturn (-1);
    /*lseek }*/

    do {
      DWORD bytes = (DWORD)left;

      /* Multiple of 2**32? */
      if (bytes == 0)
        bytes = (DWORD)page_size;

      /* Pipes have a limited input buffer size: */
      if (read_fd->pipe_buffer_size)
        bytes = min (bytes, read_fd->pipe_buffer_size);
      else
      /* Unbuffered I/O - case 1: can't be turned off for volumes */
      if (read_fd->disk_access)
      {
        /* When reading, rounding up to the next sector size doesn't really
           make any sense. The ReadFile() call will just fail in this case.

           This approach is also consistent with direct volume access, as
           every volume will have to have a size that is a multiple of the
           physical sector size (of the corresponding mass storage device).
        */
      }

      /* Read next block: */
      if (not ReadFile (handle, buf, bytes, &read, NULL))
      {
        if (GetLastError () != ERROR_BROKEN_PIPE)
        {
          /* Unbuffered I/O - case 2: turn off DIRECT_IO for r_partial records */
          if (GetLastError()==ERROR_INVALID_PARAMETER && read_fd->direct_io)
          {
            if (not read_fd->disk_access)
            {
              int old_flags = fcntl (fd, F_GETFL);
              if (fcntl (fd, F_SETFL, old_flags & ~O_DIRECT) == 0)
              {
                read_fd->direct_io = false;
                /* Prevent write_win32_writefile() callback because of `attempt_write_opt': */
                read_fd->valid = false;
                return (read (fd, buf, count));
              }
            }
          }

          WinErrReturn (-1);
        }
      }

      /* End-of-file? */
      if (read == 0)
      {
        count -= left;
        break;
      }

      buf = (char *)buf + read;
      left = (read <= left)? (left - read) : 0;
    } while (left > 0);

    if (left == 0)
      ++read_fd->r_total;  /* Increase number of records read */

    /*lseek {*/
    read_fd->absolute_offset.QuadPart = read_fd->absolute_offset.QuadPart + count;
    read_fd->logical_offset           = read_fd->logical_offset           + count;
    #ifdef _DEBUG
    if (read_fd->device_file <= DEV_NONE)
    { LONGLONG
        cur_off;

      if ((cur_off=GetFilePointerEx (handle)) < 0LL)
        WinErrReturn (-1);

      assert (read_fd->absolute_offset.QuadPart == cur_off);
    }
    #endif
    /*lseek }*/

    return ((ssize_t)count);
  }
  else
    ErrnoReturn (EINVAL, -1);
}


extern ssize_t write_win32_writefile (int fd, const void *buf, size_t count)
{ struct FileDesc *
    write_fd = get_file_desc (fd);

  if (is_volume_access_ex (write_fd) && write_fd->last_byte_index > 0)
  { off_t remaining = (write_fd->last_byte_index
                       - write_fd->absolute_offset.QuadPart + (off_t)1);

    if (remaining < (off_t)count)
    {
      if (remaining == (off_t)0)
        return ((ssize_t)0);

      count = (size_t)remaining;
    }

  /*#ifdef _DEBUG*/  /* Always included for now! */
    if (write_fd->absolute_offset.QuadPart < write_fd->first_byte_index
     || write_fd->absolute_offset.QuadPart > write_fd->last_byte_index)
    {
      fprintf (stderr, "\n[%s] win32_writefile(): absolute offset %" PRIdMAX " not with-\n"
                       "in expected partition bounds [%25" PRIdMAX ", %20" PRIdMAX "]\n"
                       "... aborting!\n",
                       PROGRAM_NAME, (intmax_t)write_fd->absolute_offset.QuadPart,
                       (intmax_t)write_fd->first_byte_index, (intmax_t)write_fd->last_byte_index);
      exit (249);
    }
  /*#endif*/
  }

  if (count > 0)
  { HANDLE
      handle = GetWinHandle (fd);
    size_t
      reqested = count,
      left = count;
    DWORD
      written;

    /*write optimization {*/
    if (write_fd->attempt_write_opt)
    {
      /* Remember current offset: */
      LARGE_INTEGER write_absolute_offset = write_fd->absolute_offset;
      off_t         write_logical_offset  = write_fd->logical_offset;
      uintmax_t     write_r_total         = write_fd->r_total;

      /* Perform read: */
      ssize_t read = read_win32_readfile (fd, write_fd->existing_data, count);

      /* Restore previous offset: */
      write_fd->absolute_offset = write_absolute_offset;
      write_fd->logical_offset  = write_logical_offset;
      write_fd->r_total         = write_r_total;

      /* Ignore any read errors! */
      if (read < 0)
        SetLastError (ERROR_SUCCESS);

      /* Compare data (if read() completed successfully): */
      if (read == count)
        if (memcmp (write_fd->existing_data, buf, count) == 0)
        {
          ++write_fd->r_total;
          goto write_win32_lseek;
        }
    }
    /*write optimization }*/

    /*lseek {*/
    if (not SetFilePointerEx (handle, write_fd->absolute_offset, NULL, FILE_BEGIN))
      WinErrReturn (-1);
    /*lseek }*/

    do {
      DWORD bytes = (DWORD)left;

      /* Multiple of 2**32? */
      if (bytes == 0)
        bytes = (DWORD)page_size;

      /* Pipes have a limited output buffer size: */
      if (write_fd->pipe_buffer_size)
        bytes = min (bytes, write_fd->pipe_buffer_size);
      else
      /* Unbuffered I/O case 1 - can't be turned off for volumes: */
      if (write_fd->disk_access)
      {
        if ((write_fd->dwFlagsAndAttributes & FILE_FLAG_NO_BUFFERING) != 0 && (bytes % write_fd->phys_sect_size) != 0)
        {
          if (IsPower2 (write_fd->phys_sect_size))
            bytes = (bytes + write_fd->phys_sect_size-1) & ~(write_fd->phys_sect_size-1);
          else
            bytes = ((bytes + write_fd->phys_sect_size-1) / write_fd->phys_sect_size) * write_fd->phys_sect_size;

          /* Remaining bytes will contain whatever data is in `buf': */
          count = count + (bytes - reqested);
        /*memset ((char *)buf + req_bytes, 0, bytes - req_bytes);*/
        }
      }

      /* Write next block:*/
      if (not WriteFile (handle, buf, bytes, &written, NULL)
          || (written == 0 && GetLastError() != ERROR_SUCCESS))
      {
        if (GetLastError () != ERROR_BROKEN_PIPE)
        {
          /* Unbuffered I/O case 2 - turn off DIRECT_IO for r_partial records: */
          if (GetLastError()==ERROR_INVALID_PARAMETER && write_fd->direct_io)
          {
            if (not write_fd->disk_access)
            {
              int old_flags = fcntl (fd, F_GETFL);
              if (fcntl (fd, F_SETFL, old_flags & ~O_DIRECT) == 0)
              {
                write_fd->direct_io = false;
              /*write_fd->valid = false;*/    /* see `close (STDOUT_FILENO)'! */
                return (write (fd, buf, count));
              }
            }
          }

          WinErrReturn (-1);
        }
      }

      /* End-of-Partition? */
      if (written == 0)
      {
        count -= left;
        break;
      }

      buf = (char *)buf + written;
      left = (written <= left)? (left - written) : 0;
    } while (left > 0);

    if (left == 0)
    {
      ++write_fd->r_total;    /* Increase total and written number of records */
      ++write_fd->r_written;
    }

    /*lseek {*/
write_win32_lseek:;
    write_fd->absolute_offset.QuadPart = write_fd->absolute_offset.QuadPart + count;
    write_fd->logical_offset           = write_fd->logical_offset           + count;
    #ifdef _DEBUG
    if (write_fd->device_file <= DEV_NONE)
    { LONGLONG
        cur_off;

      if ((cur_off=GetFilePointerEx (handle)) < 0LL)
        WinErrReturn (-1);

      assert (write_fd->absolute_offset.QuadPart == cur_off);
    }
    #endif
    /*lseek }*/

    return ((ssize_t)reqested);  /*return ((ssize_t)count);*/
  }
  else
    ErrnoReturn (EINVAL, -1);
}


/* get memory page size
  http://man7.org/linux/man-pages/man2/getpagesize.2.html
*/
extern int getpagesize (void)
{ SYSTEM_INFO
    system_info;

  GetSystemInfo (&system_info);

  return ((int)system_info.dwPageSize);
}


/* truncate a file to a specified length
   http://pubs.opengroup.org/onlinepubs/9699919799/functions/ftruncate.html
*/
extern int ftruncate (int fd, off_t size)
{ HANDLE
    hfile;
  LARGE_INTEGER
    dontmove = {0},
    ori_pos,
    new_pos;

  if (fd < 0)
    ErrnoReturn (EBADF, -1);

  if (of_fd.device_file == DEV_NONE)  /* special device file? => do nothing! */
  {
    if (is_std_file (fd))
      hfile = (HANDLE)crt_get_osfhandle (fd);
    else
    if (is_regular_file (fd))
      hfile = GetWinHandle (fd);
    else
      ErrnoReturn (EBADF, -1);

    /* BOOL WINAPI SetFilePointerEx (HANDLE hFile, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod); */
    new_pos.QuadPart = (LONGLONG)size;

    if (not SetFilePointerEx (hfile, dontmove, &ori_pos, FILE_CURRENT) ||
        not SetFilePointerEx (hfile, new_pos, NULL, FILE_BEGIN) ||
        not SetEndOfFile (hfile) ||
        not SetFilePointerEx (hfile, ori_pos, NULL, FILE_BEGIN))
    {
      switch (GetLastError ())
      {
      case ERROR_INVALID_HANDLE:
        errno = EBADF;
        break;

      default:
        errno = EIO;
        break;
      }

      return (-1);
    }
  }

  return (0);
}


/* synchronize a file's in-core state with storage device
   http://pubs.opengroup.org/onlinepubs/9699919799/functions/fsync.html
*/
extern int fsync (int fd)
{
  if (of_fd.device_file == DEV_NONE)  /* special device file? => do nothing! */
  {
    if (is_std_file (fd))
    {
      return (crt_commit (fd));
    }
    else
    if (is_regular_file (fd))
    {
      HANDLE hfile = GetWinHandle (fd);
      if (not FlushFileBuffers (hfile))
        WinErrReturn (-1);
    }
    else
      ErrnoReturn (EBADF, -1);
  }

  return (0);
}
