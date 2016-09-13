#include <config.h>

#include <errno.h>          /* EMFILE */


/* */
static int
  fdopts [MAX_OPEN_DESCRIPTORS] = {0};


/* fcntl() performs one of the operations described below on the open file
// descriptor "fd". The operation is determined by "cmd".
//
// fcntl() can take an optional third argument. Whether or not this argument
// is required is determined by "cmd". The required argument type is indicated
// in parentheses after each cmd name (in most cases, the required type is long,
// and we identify the argument using the name arg), or void is specified if
// the argument is not required.
//
// F_GETFL (void)
//   Get the file access mode and the file status flags; arg is ignored.
// F_SETFL (long)
//   Set the file status flags to the value specified by arg. On Linux this
//   command can change only the O_APPEND, O_ASYNC, O_DIRECT, O_NOATIME, and
//   O_NONBLOCK flags.
//
// Return Value
//    If successful, all SET operations return zero. Otherwise, -1 is returned,
//    and errno is set appropriately.
*/
extern int fcntl (int fd, int cmd, ...)
{ int
    real_fd, arg;

  real_fd = (fd & CHGFLAGS_FILDES_MASK);

  if (fd < 0 || (HANDLE)crt_get_osfhandle (real_fd) == INVALID_HANDLE_VALUE)
    ErrnoReturn (EBADF, -1);

  if (real_fd >= MAX_OPEN_DESCRIPTORS)
    ErrnoReturn (EMFILE, -1);

  switch (cmd)
  {
  case F_GETFL:
    return (fdopts [real_fd]);

  case F_SETFL:
    { va_list
        argptr;

      va_start (argptr, cmd);
      arg = va_arg (argptr, int);
      va_end (argptr);

      /* Set O_BINARY only for those streams we'll actually be accessing: */
      if ((arg & O_BINARY) == O_BINARY)
      {
        if (IsDescriptorRedirected (real_fd))
          if (crt_setmode (real_fd, O_BINARY) < 0)
          {
            RecordPosixErr (errno);
            return (-1);
          }
      }

      /*O_DIRECT {*/
      if ((arg & O_DIRECT) != 0)
      {
        /* disable buffering: */
        if (set_direct_io (fd, true) < 0)
          return (-1);
      }
      else
      {
        /* enable buffering: */
        if (set_direct_io (fd, false) < 0)
          return (-1);
      }
      /*O_DIRECT }*/

      fdopts [real_fd] = arg;
      return (0);
    }
    break;
  }

  return (-1);
}
