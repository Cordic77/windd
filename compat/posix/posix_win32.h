#ifndef WINDD_POSIX_H_
#define WINDD_POSIX_H_

//
extern int
  newfd_ex (HANDLE handle, bool from_end);
extern int
  newfd (HANDLE handle);
extern void
  remfd (int fd);
extern int
  handle2fd (HANDLE handle);
extern HANDLE
  fd2handle (int fd);

//
extern bool
  is_std_file (int fd);
extern bool
  is_regular_file (int fd);
extern bool
  is_device_file (int fd);
extern bool
  is_volume_access (int fd);
extern uint32_t
  is_pipe_handle (int fd);

//
extern int
  set_direct_io (int fd, bool direct);
/* http://pubs.opengroup.org/onlinepubs/9699919799/functions/dup.html */
extern int
  dup2 (int fildes, int fildes2);
/* https://fossies.org/dox/coreutils-8.25/fd-reopen_8c.html */
extern int
  fd_reopen (int desired_fd, char const *file, int flags, mode_t mode);
/* http://pubs.opengroup.org/onlinepubs/9699919799/functions/close.html */
extern int
  close (int fd);

/* http://pubs.opengroup.org/onlinepubs/9699919799/functions/lseek.html */
extern off_t
  lseek_win32_setfilepointer (int fd, off_t offset, int whence);
/* pubs.opengroup.org/onlinepubs/9699919799/functions/read.html */
extern ssize_t
  read_win32_readfile (int fd, void *buf, size_t count);
/* http://pubs.opengroup.org/onlinepubs/9699919799/functions/write.html */
extern ssize_t
  write_win32_writefile (int fd, const void *buf, size_t count);

#endif
