#ifndef WINDD_MSVCR_FILEOPS_H_
#define WINDD_FILEOP_H_

/* */
struct FileOperations
{
  off_t   (*lseek_func) (int fd, off_t offset, int whence);
  ssize_t (*read_func)  (int fd, void *buf, size_t count);
  ssize_t (*write_func) (int fd, const void *buf, size_t count);
};

/* */
extern struct FileOperations
  if_fop,
  of_fop;

/* */
extern int
  init_msvcr_fileops (void);

/* int _open_osfhandle (intptr_t osfhandle, int flags); */
int
  crt_open_osfhandle (intptr_t osfhandle, int flags);
/* intptr_t _get_osfhandle (int fd); */
extern intptr_t
  crt_get_osfhandle (int fd);
/* int _isatty (int fd); */
extern int
  crt_isatty (int fd);
/* int _setmode (int fd, int mode); */
extern int
  crt_setmode (int fd, int mode);
/* int _commit (int fd); */
extern int
  crt_commit (int fd);
/* int _close (int fd); */
extern int
  crt_close (int fd);

#endif
