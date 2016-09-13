#ifndef WINDD_FCNTL_H_
#define WINDD_FCNTL_H_

/* Maximum number of sockets that can be created on Windows NT platforms:
   HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\Tcpip\Parameters */
#define MAX_OPEN_DESCRIPTORS  65534

#define F_GETFL		  3     /* get file->f_flags */
#define F_SETFL		  4     /* set file->f_flags */

/* http://pubs.opengroup.org/onlinepubs/9699919799/functions/fcntl.html */
extern int
  fcntl (int fd, int cmd, ...);

#endif
