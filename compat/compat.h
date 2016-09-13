/* Compatibility headers: */
#define ENABLE_EXPERIMENTAL_FEATURES

#if defined ENABLE_EXPERIMENTAL_FEATURES
#define DETECT_OPEN_HANDLES               /* Try to detect if there are open file handles? */
#endif

/* Declare any necessary compatibility scaffolding right at the beginning; If
   this leads to any warnings/errors, we know we've screwed up!
*/
#if PRE_COMPAT_SETTINGS == 1
  #define _MSVC_NOTHROW_H                 /* error.c: prevent `#include "msvc-nothrow.h"' */
  #define _INC_IO                         /* msvc-nothrow.h: prevent `# include <io.h>' */

  #define C_CTYPE_H                       /* c-strcaseeq.h: prevent `#include "c-ctype.h"' */
  #define TIMESPEC_H                      /* system.h, gethrxtime.c: prevent `#include "timespec.h"' */

  /* C++ needs to know that types and declarations are C, not C++.
     /usr/include/x86_64-linux-gnu/sys/cdefs.h: required by several
     files in `compat\usr_include' */
  #ifdef  __cplusplus
  # define __BEGIN_DECLS  extern "C" {
  # define __END_DECLS  }
  #else
  # define __BEGIN_DECLS
  # define __END_DECLS
  #endif

  /*===  POSIX types  ===*/
  typedef int uid_t;                      /* system.h, openat.h */
  typedef int gid_t;                      /* system.h, openat.h */
  typedef int mode_t;                     /* openat.h, fd-reopen.h */

  #define __USE_XOPEN2K8                  /* dirent.h: declare `extern DIR *fdopendir (int);' */
  typedef int __ino_t;
  typedef __int64 __off_t;
  #define __nonnull(arg_index, ...)

  /*===  POSIX 64-bit types  ===*/
  #undef _USE_32BIT_TIME_T                /* CRT: force 64-bit types! */

  #ifndef _OFF_T_DEFINED                  /* CRT: prevent `typedef long off_t;' */
  typedef long _off_t;
/*#if     !__STDC__
  // Non-ANSI name for compatibility */
  typedef __int64 off_t;                  /* file offset value */
/*#endif*/
  #define OFF_T_MAX       INT64_MAX
  #define _OFF_T_DEFINED
  #endif
#endif

/* Default include files: */
#if PRE_COMPAT_SETTINGS == 1
  /*===  Win32 API  ===*/
  #include "win32\windows_api.h"          /* also includes <tchar.h> */

  /*===  C89 headers  ===*/
  #include <stdio.h>                      /* printf(), snprintf() */
  #if _MSC_VER < 1900  /* snprintf (introduced with VS2015) */
  #define snprintf _snprintf
  #endif

  #include <stdlib.h>                     /* _CRTRESTRICT, _CRT_STRINGIZE, _exit(), EXIT_SUCCESS, EXIT_FAILURE */

  #include <stdarg.h>                     /* va_list, va_start(), va_arg(), va_end() */
  #ifndef va_copy                         /* va_copy (introduced with VS2013) */
  #define va_copy(destination, source) ((destination) = (source))
  #endif

  #include <locale.h>                     /* N.B.: also included in `system.h' */
  #if ! HAVE_LC_MESSAGES
  #define LC_MESSAGES LC_ALL              /* system.h: `emit_ancillary_info (char const *program)' */
  #endif

  #include <string.h>                     /* c-strcaseeq.h, path_win32.c: _stricmp(), _memicmp() */
  extern int c_strcasecmp (const char *s1, const char *s2);
  #define c_strcasecmp(s1,s2) _stricmp(s1,s2)
  #define C_STRCASE_H                     /* c-strcaseeq.h: prevent `#include "c-strcase.h"' */

  #include <errno.h>
  #include <math.h>                       /* OVERFLOW */
  #ifndef EOVERFLOW
  #define EOVERFLOW  OVERFLOW             /* dd.c: `lseek_errno = EOVERFLOW;' */
  #endif

  /*===  C99 headers  ===*/
  #include <iso646.h>                     /* not, and, or, xor */
/*#if _MSC_VER >= 1600  // VS2010 */
  #include <stdint.h>                     /* uintmax_t (introduced with VS2010) */
/*#endif*/
/*#if _MSC_VER >= 1800  // VS2013 */
  #include <inttypes.h>                   /* PRIuMAX */
  #include <stdbool.h>                    /* bool (introduced with VS2013) */
//#endif

  /*=== Windows types ===*/
  #ifndef _SSIZE_T_DEFINED
    #include <BaseTsd.h>                  /* SSIZE_T */
    #undef ssize_t
    typedef SSIZE_T ssize_t;
    #define _SSIZE_T_DEFINED
  #endif

  #define inline __inline                 /* redefine to VC++ __inline keyword */

  /*=== Text console ===*/
  #include "win32\msvcr_fileops.h"
  #include "win32\console_win32.h"

  /*=== Global externs ===*/
  #define PACKVERSION(major,minor)    MAKELONG(minor,major)
  #define MAJOR_VERSION(packvers)     HIWORD(packvers)
  #define MINOR_VERSION(packvers)     LOWORD(packvers)

  extern LONG os_version;
  extern bool elevated_process;
#endif

/* POSIX compatibility: */
#if POST_COMPAT_SETTINGS == 1
  #include "usr_include/ctype_compat.h"   /* system.h: isblank() */
  #include "usr_include/fcntl_compat.h"   /* openat.h: AT_SYMLINK_NOFOLLOW */
  #include "usr_include/stat_compat.h"    /* dd.c: S_IR*, S_IW* */
  #include "usr_include/time_compat.h"    /* gethrxtime.c, clock_gettime.c: struct timespec, tv_nsec */

  #include "safe_malloc.h"                /* safe memory allocation functions */
  #include "unistr.h"                     /* string helper functions and UTF8/UTF16 conversion */

  typedef char utf8_t;
  #define UTF8_T_DEFINED

  /*===  print to allocated string  ===*/
  #include "msvc/asprintf.h"             /* xvasprintf.c: vasprintf() */

  /*===  clock and timer functions  ===*/
  #include "msvc/clock_gettime.h"
  void microuptime (struct timeval *tv);
  #define microuptime(tv) (tv)->tv_sec = 0, (tv)->tv_usec = 0  /*_WIN32: unused!*/

/*TODO*/

  /*===  fake support for signals  ===*/
  #include <signal.h>

  #ifndef _SIG_ATOMIC_T_DEFINED
  typedef int sig_atomic_t;
  #define _SIG_ATOMIC_T_DEFINED
  #endif

  #ifndef SIGUSR1
  /*#define SIGUSR1 30  // user defined signal 1 */
  #define SIGUSR1   SIGBREAK              /* Ctrl-Break causes `dd' to print I/O statistics to standard error */

  #  define SIG_BLOCK   0  /* blocked_set = blocked_set | *set; */
  #  define SIG_SETMASK 1  /* blocked_set = *set; */
  #  define SIG_UNBLOCK 2  /* blocked_set = blocked_set & ~*set; */
  #endif

  /* siginterrupt - allow signals to interrupt system calls
     http://pubs.opengroup.org/onlinepubs/9699919799/functions/siginterrupt.html
  */
  int siginterrupt (int sig, int flag);
  #define siginterrupt(sig,flag) do { (void)sig; (void)flag; } while (false)

  /* Simulate signals by installing a Control handler function: */
  extern void install_signal_handlers_win32 (void);
  extern void cleanup (void);

  /*===  warning: 'function' undefined; assuming extern returning int  ===*/
  extern int openat (int dirfd, const char *pathname, int flags);
  extern int fchownat (int dirfd, const char *pathname, uid_t owner, gid_t group, int flags);
  extern int fchmod (int fd, mode_t mode);
  extern int fchmodat (int dirfd, const char *pathname, mode_t mode, int flags);
  extern int fstatat (int dirfd, const char *pathname, struct /*stat*/_stat64 *buf, int flags);

  /*===  error.c:is_open(), compat.c:ftruncate()  ===*/
/*#include <io.h>*/     /* "warning C4985: 'close': attributes not present on previous declaration." (see `raw_win32.h') */
/*
  _CRTIMP intptr_t __cdecl _get_osfhandle (_In_ int _FileHandle);
  _Check_return_opt_ _CRTIMP int __cdecl _commit (_In_ int _FileHandle);
*/
  /* These CRT routines are made accessible via wrappers in `fileop_win32.c'! */

  /*===  compat.c:  ===*/
  extern int getpagesize (void);

  extern int ftruncate (int fd, off_t size);
  extern int fsync (int fd);

  extern int fdatasync (int fd);
  #define fdatasync(fd) fsync(fd)

  /*===  stat(), fstat()  ===*/
  #include <sys/stat.h>
  #include <sys/types.h>

/*int stat (int fd, struct __stat64 *buffer);*/
  #define fstat(fd,buf) _fstat64(fd,buf)
  #define stat __stat64

  #ifndef S_ISREG
  #define S_ISREG(mode)  (((mode) & S_IFMT) == S_IFREG)
  #endif
  #ifndef S_ISDIR
  #define S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
  #endif
  #ifndef S_ISLNK
  #define S_ISLNK(mode)     0  /* usable_st_size() is only called once; let's just resolve any symlinks before that! */
  #endif
  #ifndef S_TYPEISSHM
  #define S_TYPEISSHM(stat) 0
  #endif
  #ifndef S_TYPEISTMO
  #define S_TYPEISTMO(stat) 0
  #endif

  /* VC++ lacks these macros: */
  #ifndef STDIN_FILENO
  # define STDIN_FILENO 0
  #endif
  #ifndef STDOUT_FILENO
  # define STDOUT_FILENO 1
  #endif
  #ifndef STDERR_FILENO
  # define STDERR_FILENO 2
  #endif

  #define CHGFLAGS_ONLY_FILENO  0x10000   /* Only change file flags, don't open any files. */
  #define CHGFLAGS_FILDES_MASK  0x0FFFF

  // http://pubs.opengroup.org/onlinepubs/9699919799/functions/open.html
  #include <fcntl.h>                      /* O_ constants (+ dummy defines for missing values) */

  #ifndef O_DIRECT
  #define O_DIRECT       0x00400000ul     /* Try to minimize cache effects of I/O */
  #endif
  #ifndef O_DIRECTORY
  #define O_DIRECTORY    0x00800000ul     /* If pathname is not a directory, cause the open to fail */
  #endif
  #ifndef O_DSYNC
  #define O_DSYNC        0x01000000ul     /* Synchronized I/O data integrity completion requirement */
  #endif
  #ifndef O_NOATIME
  #define O_NOATIME      0x02000000ul     /* Do not update the file last access time */
  #endif
  #ifndef O_NOCTTY
  #define O_NOCTTY       0x04000000ul     /* Terminal device will not become the process's controlling terminal even if the process does not have one. */
  #endif
  #ifndef O_NOFOLLOW
  #define O_NOFOLLOW     0x08000000ul     /* If pathname is a symbolic link, then the open fails */
  #endif
  #ifndef O_NOLINKS
  #define O_NOLINKS      0x10000000ul     /* If  the link count of the named file is greater than 1, open() fails */
  #endif
  #ifndef O_NONBLOCK
  #define O_NONBLOCK     0x20000000ul     /* When possible, the file is opened in nonblocking mode */
  #endif
  #ifndef O_SYNC
  #define O_SYNC         0x40000000ul     /* Synchronized I/O data (and metadata) integrity completion requirement */
  #endif
  #ifndef O_RSYNC
  #define O_RSYNC        0x80000000ul     /* HPUX only */
  #endif

  #include "msvc\fcntl_win32.h"

  /* _WIN32 wrappers for various POSIX functions: */
  #include "win32\devfiles_win32.h"
  #include "posix\posix_win32.h"

  off_t lseek (int fd, off_t offset, int whence);
  #define lseek(fd, offset, whence) ((fd==STDIN_FILENO)? (if_fop.lseek_func (fd, offset, whence)) : \
                                                         (of_fop.lseek_func (fd, offset, whence)))
  ssize_t read (int fd, void *buf, size_t count);
  #define read(fd, buf, count) (if_fop.read_func (fd, buf, count))

  ssize_t write (int fd, const void *buf, size_t count);
  #define write(fd, buf, count) (of_fop.write_func (fd, buf, count))
#endif

/* Win32 helper functions: */
#if POST_COMPAT_SETTINGS == 1
  #include "win32\error_win32.h"          /* simplify error handling */
  #include "win32\com_supp.h"             /* C/C++ compatiblity macros for COM */
  #include "win32\handles_win32.h"        /* Enumerate open file HANDLE's */
  #include "win32\objmgr_win32.h"         /* Device objects of the Windows Object Manager */
  #include "win32\suppart_win32.h"        /* Partition types supported by Windows */
  #include "win32\diskmgmt\diskmgmt_win32.h"/* crude compatiblity layer for libata */
  #include "win32\path_win32.h"           /* path normalization functions */
  #include "win32\raw_win32.h"            /* raw disk access under Win32 */
#endif

/* Coreutils header files: */
#if POST_COMPAT_SETTINGS == 1
  # define _GL_ATTRIBUTE_FORMAT_PRINTF(a, b)  /* see `error.c' */
  # define _GL_ARG_NONNULL(a)

  #include <system.h>                     /* _() macro */
  #include <verify.h>                     /* verify macro() */
  #include <close-stream.h>               /* close_stream() */
  #include <closeout.h>                   /* close_stdout() */

  #define PROGRAM_NAME "windd"

  #ifdef HELP_OPTION_DESCRIPTION
  #undef HELP_OPTION_DESCRIPTION
  #endif
  #define HELP_OPTION_DESCRIPTION \
    _("  -h, --help     display this help and exit\n")
  #ifdef VERSION_OPTION_DESCRIPTION
  #undef VERSION_OPTION_DESCRIPTION
  #endif
  #define VERSION_OPTION_DESCRIPTION \
    _("  -v, --version  output version information and exit\n")
  #define LIST_OPTION_DESCRIPTION \
    _("  -l, --list     list all physical and logical drives\n")
  #define NOOPT_OPTION_DESCRIPTION \
    _("  -n, --nopt     disables all (programmatically chosen) optimizations\n")
  #define FORCE_OPTION_DESCRIPTION \
    _("  -f, --force    force operation (ignore open handles etc.)\n")
  #define CONFIRM_OPTION_DESCRIPTION \
    _("  -c, --confirm  always ask back, before doing anything critical\n")

  /* Call dd's main() from our (Windows) main() function: */
  extern bool
    disable_optimizations;

  #define IO_STATISTICS_HIDDEN_MESSAGE_ONLY_WINDOW 1

  extern int linux_main (int argc, char **argv);
  #  define initialize_main(ac, av)
  #define main linux_main

  /* Input or output stdin or stdout, respectively? (if=con: <or> of=con:) */
  extern void set_fd_flags (int fd, int add_flags, char const *name);
#endif
