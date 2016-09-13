#ifndef WINDD_DEVFILES_H_
#define WINDD_DEVFILES_H_

/* /dev/zero */
extern ssize_t
  read_dev_zero (int fd, void *buf, size_t count);
extern off_t
  lseek_dev_zero (int fd, off_t offset, int whence);

/* /dev/random */
extern ssize_t
  read_dev_random (int fd, void *buf, size_t count);
extern off_t
  lseek_dev_random (int fd, off_t offset, int whence);

#endif
