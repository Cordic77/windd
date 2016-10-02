#include <io.h>

#ifndef _SSIZE_T_DEFINED
  #include <BaseTsd.h>                  /* SSIZE_T */
  #undef ssize_t
  typedef SSIZE_T ssize_t;
  #define _SSIZE_T_DEFINED
#endif

#ifndef _OFF_T_DEFINED                  /* CRT: prevent `typedef long off_t;' */
  typedef long _off_t;
/*#if     !__STDC__
  // Non-ANSI name for compatibility */
  typedef __int64 off_t;                /* file offset value */
/*#endif*/
  #define OFF_T_MAX       INT64_MAX
  #define _OFF_T_DEFINED
#endif

#include "msvcr_fileops.h"


/* */
struct FileOperations
  if_fop,  /* = {&lseek_if,&read_if,&write_if};  // input file */
  of_fop;  /* = {&lseek_of,&read_of,&write_of};  // output file */


/* */
static off_t crt_lseek (int fd, off_t offset, int whence)
{
  return (_lseeki64 (fd, offset, whence));
}


static ssize_t crt_read (int fd, void *buf, size_t count)
{
  ssize_t read = _read (fd, buf, (unsigned int)count);
  return (read);
}


static ssize_t crt_write (int fd, const void *buf, size_t count)
{
  ssize_t written = _write (fd, buf, (unsigned int)count);
  return (written);
}


/* Initialize file operations structure. */
extern int init_msvcr_fileops (void)
{
  if_fop.lseek_func = &crt_lseek;
  if_fop.read_func  = &crt_read;
  if_fop.write_func = &crt_write;

  of_fop.lseek_func = &crt_lseek;
  of_fop.read_func  = &crt_read;
  of_fop.write_func = &crt_write;

  return (0);
}


extern int crt_open_osfhandle (intptr_t osfhandle, int flags)
{
  return (_open_osfhandle (osfhandle, flags));
}


extern intptr_t crt_get_osfhandle (int fd)
{
  return (_get_osfhandle (fd));
}


extern int crt_isatty (int fd)
{
  return (_isatty (fd));
}


extern int crt_setmode (int fd, int mode)
{
  return (_setmode (fd, mode));
}


extern int crt_commit (int fd)
{
  return (_commit (fd));
}


extern int crt_close (int fd)
{
  return (_close (fd));
}
