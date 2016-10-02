#ifndef WINDD_GETTIME_H_
#define WINDD_GETTIME_H_

/* Clock that cannot be set and represents monotonic time since
   some unspecified starting point:
*/
#define CLOCK_MONOTONIC 1

/* */
#ifndef _CLOCKID_T
#define _CLOCKID_T
typedef int clockid_t;            /* clock identifier type */
#endif

/* http://pubs.opengroup.org/onlinepubs/9699919799/functions/clock_getres.html */
extern int
  clock_gettime (clockid_t clock_id, struct timespec *tp);

#endif
