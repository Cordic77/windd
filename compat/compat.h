/* Compatibility headers: */
#if defined _DEBUG && defined _WIN64
#define ENABLE_EXPERIMENTAL_FEATURES
#endif

#if defined ENABLE_EXPERIMENTAL_FEATURES
#define DETECT_OPEN_HANDLES               /* Try to detect if there are open file handles? */
#endif

/* Declare any necessary compatibility scaffolding right at the beginning; If
   this leads to any warnings/errors, we know we've screwed up!
*/
#if PRE_COMPAT_SETTINGS == 1
  #define _INC_IO                         /* msvc-nothrow.h: prevent `# include <io.h>' */

  #define C_CTYPE_H                       /* c-strcaseeq.h: prevent `#include "c-ctype.h"' */
  #define TIMESPEC_H                      /* system.h, gethrxtime.c: prevent `#include "timespec.h"' */

  /* C++ compiler? Export declarationss using `extern "C"'!
     /usr/include/x86_64-linux-gnu/sys/cdefs.h:
     required by several files in `compat\usr_include' */
  #ifdef  __cplusplus
  # define __BEGIN_DECLS  extern "C" {
  # define __END_DECLS  }
  #else
  # define __BEGIN_DECLS
  # define __END_DECLS
  #endif

  /*===  C99 keywords  ===*/
  #ifndef __cplusplus
    #if __STDC_VERSION__ == 199901L /* C99 */
    #elif _MSC_VER >= 1500 /* MSVC 9 or newer */
      #define inline __inline
    #elif __GNUC__ >= 3 /* GCC 3 or newer */
      #define inline __inline
    #else /* Unknown or ancient */
      #define inline
    #endif
  #endif

  /* restrict is standard in C99, but not in all C++ compilers. */
  #if __STDC_VERSION__ == 199901L /* C99 */
    #define RESTRICT restrict
  #elif _MSC_VER >= 1500 /* MSVC 9 or newer */
    #ifdef _CRTRESTRICT
    #define RESTRICT _CRTRESTRICT
    #else
    #define RESTRICT __declspec(restrict)
    #endif
  #elif __GNUC__ >= 3 /* GCC 3 or newer */
    #define RESTRICT __restrict
  #else /* Unknown or ancient */
    #define RESTRICT
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
  #if _MSC_VER < 1900  /* snprintf() [introduced with VS2015] */
  #define snprintf _snprintf
  #endif

  #include <stdlib.h>                     /* _CRTRESTRICT, _CRT_STRINGIZE, _exit(), EXIT_SUCCESS, EXIT_FAILURE */

  #include <stdarg.h>                     /* Variadic functions support: va_list, va_start(), va_arg(), va_end() */
  #ifndef va_copy                         /* va_copy() [introduced with VS2013] */
  #define va_copy gl_va_copy
  #endif

  #include <locale.h>                     /* N.B.: also included in `system.h' */
  #if ! HAVE_LC_MESSAGES
  #define LC_MESSAGES LC_ALL              /* system.h: `emit_ancillary_info (char const *program)' */
  #endif

  #include <string.h>                     /* c-strcaseeq.h, path_win32.c: _stricmp(), _memicmp() */
  extern int c_strcasecmp (const char *s1, const char *s2);
  #define c_strcasecmp(s1,s2) _stricmp(s1,s2)
  #define C_STRCASE_H                     /* c-strcaseeq.h: prevent `#include "c-strcase.h"' */

  #include "assure.h"                     /* assert(), assure() */

  #include <errno.h>
  #include <math.h>                       /* OVERFLOW */
  #ifndef EOVERFLOW
  #define EOVERFLOW  OVERFLOW             /* dd.c: `lseek_errno = EOVERFLOW;' */
  #endif

  /*===  C95/NA1 headers  ===*/
  #include <iso646.h>                     /* not, and, or, xor */

  /*===  C99 headers  ===*/
/*#if _MSC_VER >= 1600  // VS2010 */
  #include <stdint.h>                     /* uintmax_t [introduced with VS2010] */
/*#endif*/
/*#if _MSC_VER >= 1800  // VS2013 */
  #include <inttypes.h>                   /* PRIuMAX */
  #include <stdbool.h>                    /* bool [introduced with VS2013] */
//#endif

  /*===  Windows types  ===*/
  #ifndef _SSIZE_T_DEFINED
    #include <BaseTsd.h>                  /* SSIZE_T */
    #undef ssize_t
    typedef SSIZE_T ssize_t;
    #define _SSIZE_T_DEFINED
  #endif

  /*===  Global externs  ===*/
  #define PACKVERSION(major,minor)    MAKELONG(minor,major)
  #define MAJOR_VERSION(packvers)     HIWORD(packvers)
  #define MINOR_VERSION(packvers)     LOWORD(packvers)

  extern LONG os_version;
  extern bool elevated_process;

  /*===  Text console  ===*/
  #include "win32\console_win32.h"

  #ifndef STDIN_FILENO
  # define STDIN_FILENO 0
  #endif
  #ifndef STDOUT_FILENO
  # define STDOUT_FILENO 1
  #endif
  #ifndef STDERR_FILENO
  # define STDERR_FILENO 2
  #endif
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
  #include "msvc/asprintf.h"              /* xvasprintf.c: vasprintf() */

  /*===  clock and timer functions  ===*/
  #include "msvc/clock_gettime.h"
  void microuptime (struct timeval *tv);
  #define microuptime(tv) (tv)->tv_sec = 0, (tv)->tv_usec = 0  /*_WIN32: dummy definition - unused!*/
  #ifndef HAVE_MICROUPTIME
  #define HAVE_MICROUPTIME 1
  #endif

  /*===  fake support for signal()s  ===*/
  #include <signal.h>

  #ifndef _SIG_ATOMIC_T_DEFINED
  typedef int sig_atomic_t;
  #define _SIG_ATOMIC_T_DEFINED
  #endif

  #ifndef SIGUSR1
  /*#define SIGUSR1 30  // user defined signal 1 */
  #define SIGUSR1   SIGBREAK              /* As install_signal_handlers() is no longer called, this value is irrelevant. */

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
/*#include <io.h>*/     /* "warning C4985: 'close': attributes not present on previous declaration." [in `raw_win32.h'] */
/*
  _CRTIMP intptr_t __cdecl _get_osfhandle (_In_ int _FileHandle);
  _Check_return_opt_ _CRTIMP int __cdecl _commit (_In_ int _FileHandle);
*/
  #include "win32\msvcr_fileops.h"        /* crt_get_osfhandle(), crt_xxx() */

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

  // http://pubs.opengroup.org/onlinepubs/9699919799/functions/open.html
  #include <fcntl.h>                      /* O_ constants supported under Windows */

  #define CHGFLAGS_ONLY_FILENO  0x10000   /* Only change file flags, don't open any files. */
  #define CHGFLAGS_FILDES_MASK  0x0FFFF

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
  #include "posix\posix_win32.h"

  off_t lseek (int fd, off_t offset, int whence);
  #define lseek(fd, offset, whence) ((fd==STDIN_FILENO)? (if_fop.lseek_func (fd, offset, whence)) : \
                                                         (of_fop.lseek_func (fd, offset, whence)))
  ssize_t read (int fd, void *buf, size_t count);
  #define read(fd, buf, count) (if_fop.read_func (fd, buf, count))

  ssize_t write (int fd, const void *buf, size_t count);
  #define write(fd, buf, count) (of_fop.write_func (fd, buf, count))

  /* Helper functions to simplify POSIX/COM/WIN32 error handling: */
  #include "win32\error_win32.h"
#endif

/* Coreutils header files: */
#if POST_COMPAT_SETTINGS == 1
  # define _GL_ATTRIBUTE_FORMAT_PRINTF(a, b)  /* see `error.c' */
  # define _GL_ARG_NONNULL(a)

  #include <system.h>                     /* _() macro */

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
  #define NOLCK_OPTION_DESCRIPTION \
    _("  -k, --nolock   don't lock (or dismount) any drives\n")
  #define NOOPT_OPTION_DESCRIPTION \
    _("  -n, --nopt     disables all (programmatically chosen) optimizations\n")
  #define FORCE_OPTION_DESCRIPTION \
    _("  -f, --force    force operation (ignore open handles etc.)\n")
  #define CONFIRM_OPTION_DESCRIPTION \
    _("  -c, --confirm  always ask back, before doing anything critical\n")

  /* See `dd.c': let's make these flags globally visible: */
  enum
  {
    C_ASCII = 01,

    C_EBCDIC = 02,
    C_IBM = 04,
    C_BLOCK = 010,
    C_UNBLOCK = 020,
    C_LCASE = 040,
    C_UCASE = 0100,
    C_SWAB = 0200,
    C_NOERROR = 0400,
    C_NOTRUNC = 01000,
    C_SYNC = 02000,

    /* Use separate input and output buffers, and combine partial
    input blocks. */
    C_TWOBUFS = 04000,

    C_NOCREAT = 010000,
    C_EXCL = 020000,
    C_FDATASYNC = 040000,
    C_FSYNC = 0100000,

    C_SPARSE = 0200000
  };

  /* Some bittwiddling tricks: */
  #define IsPower2(n) ((n & ~(n-1))==n)

  /* Call dd's main() from our (Windows) main() function: */
  extern bool
    disable_optimizations;

  extern int linux_main (int argc, char **argv);
  #  define initialize_main(ac, av)
  #define main linux_main

  /* Input or output stdin or stdout, respectively? (if=con: <or> of=con:) */
  extern void set_fd_flags (int fd, int add_flags, char const *name);
#endif
