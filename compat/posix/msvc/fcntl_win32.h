#ifndef WINDD_FCNTL_H_
#define WINDD_FCNTL_H_

/* 3 standard descriptors plus a maximum of 2 additional OpenVolume() calls: */
#define MAX_OPEN_DESCRIPTORS  5

#define F_GETFL     3     /* get file->f_flags */
#define F_SETFL     4     /* set file->f_flags */

/* http://pubs.opengroup.org/onlinepubs/9699919799/functions/fcntl.html */
extern int
  fcntl (int fd, int cmd, ...);

#endif
