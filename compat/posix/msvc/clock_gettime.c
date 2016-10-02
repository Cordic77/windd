#include "config.h"

#if !defined(XTIME_PRECISION)
#include "xtime.h"
#endif


/* "Porting clock_gettime to windows?"
   Carl Staelin, answered Mar 23 '11 at 11:22
   http://stackoverflow.com/questions/5404277/porting-clock-gettime-to-windows#answer-5404467
*/
static LARGE_INTEGER GetFileTimeOffset (void)
{ SYSTEMTIME
    s;
  LARGE_INTEGER
    t;

  s.wYear = 1970;  s.wMonth  = 1;  s.wDay    = 1;
  s.wHour = 0;     s.wMinute = 0;  s.wSecond = 0;  s.wMilliseconds = 0;

  /* Contains a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 (UTC): */
  { FILETIME f;

    SystemTimeToFileTime (&s, &f);

    t.QuadPart = f.dwHighDateTime;
    t.QuadPart <<= 32;
    t.QuadPart |= f.dwLowDateTime;
  }

  return (t);
}


extern int clock_gettime (clockid_t clock_id, struct timespec *tp)
{ static double
    frequencyToMicroseconds = -1.0;
  static BOOL
    usePerformanceCounter;
  static LARGE_INTEGER
    offset;
  LARGE_INTEGER
    t;

  (void)clock_id;  /* Ignored; right now, we only really fake a single clock! */

  if (frequencyToMicroseconds < 0.0)
  { LARGE_INTEGER
      performanceFrequency;

    usePerformanceCounter = QueryPerformanceFrequency (&performanceFrequency);

    if (usePerformanceCounter)
    {
      QueryPerformanceCounter (&offset);
      frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.;
    }
    else
    {
      offset = GetFileTimeOffset ();
      frequencyToMicroseconds = 10.;  /* 100-nanosecond intervals to microsecond intervals */
    }
  }

  if (usePerformanceCounter)
    QueryPerformanceCounter (&t);
  else
  { FILETIME f;

    GetSystemTimeAsFileTime (&f);
    t.QuadPart = f.dwHighDateTime;
    t.QuadPart <<= 32;
    t.QuadPart |= f.dwLowDateTime;
  }

  t.QuadPart -= offset.QuadPart;

  { double microseconds = (double)t.QuadPart / frequencyToMicroseconds;

    #if defined(XTIME_PRECISION) && XTIME_PRECISION == 1000000000
    t.QuadPart   = 1000ll * (LONGLONG)microseconds;
    tp->tv_sec   = (long)(t.QuadPart / XTIME_PRECISION);
    tp->tv_nsec  = (long)(t.QuadPart % XTIME_PRECISION);
    #else
    t.QuadPart   = (LONGLONG)microseconds;
    tp->tv_sec   = (long)(t.QuadPart / 1000000);
    tp->tv_usec  = (long)(t.QuadPart % 1000000);
    #error "XTIME_PRECISION has an unsupported value"
    #endif
  }

  return (0);
}
