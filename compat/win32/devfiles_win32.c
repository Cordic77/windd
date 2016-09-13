#include <config.h>


/* Emulation for some POSIX special device files: */

/* /dev/zero */
static off_t
  off_dev_zero; /* = 0; */

extern ssize_t read_dev_zero (int fd, void *buf, size_t count)
{
  memset (buf, 0, count);

  off_dev_zero = off_dev_zero + count;
  return (count);
}


extern off_t lseek_dev_zero (int fd, off_t offset, int whence)
{
  (void)fd;

  switch (whence)
  {
  case SEEK_SET: off_dev_zero = offset; break;
  case SEEK_CUR: off_dev_zero += offset; break;
  case SEEK_END: off_dev_zero = (off_t)LLONG_MAX; break;
  }

  return (off_dev_zero);
}


/* /dev/random, /dev/urandom */
static off_t
  off_dev_random; /* = 0; */

#if defined _WIN32
  #define RtlGenRandom SystemFunction036
  EXTERN_C BOOLEAN NTAPI RtlGenRandom(PVOID RandomBuffer, ULONG RandomBufferLength);
  /*#pragma comment(lib, "advapi32.lib")*/
#endif /* defined _WIN32 */

extern ssize_t read_dev_random (int fd, void *buf, size_t count)
{
  (void)fd;

  /*// Only for testing purposes:
  { size_t i;
    for (i=0; i < count; ++i)
      *((char *)buf + i) = rand ();
  }
  */

  /* CryptGenRandom is a cryptographically secure pseudorandom number
     generator function that is included in Microsoft CryptoAPI: */
  if (not RtlGenRandom (buf, (ULONG)count))
  {
    WinErrReturn (-1);
  }

  off_dev_random = off_dev_random + count;
  return (count);
}


extern off_t lseek_dev_random (int fd, off_t offset, int whence)
{
  (void)fd;

  switch (whence)
  {
  case SEEK_SET: off_dev_zero = offset; break;
  case SEEK_CUR: off_dev_zero += offset; break;
  case SEEK_END: off_dev_zero = (off_t)LLONG_MAX; break;
  }

  return (off_dev_zero);
}
