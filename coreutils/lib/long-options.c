/* Utility to accept --help and --version options as unobtrusively as possible.

   Copyright (C) 1993-1994, 1998-2000, 2002-2006, 2009-2016 Free Software
   Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* Written by Jim Meyering.  */

#include <config.h>

#include "win32\diskmgmt\diskmgmt_win32.h"  /* DumpDrivesAndPartitions(), DEVFILE_*, force_operations/always_confirm */

/* Specification.  */
#include "long-options.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "version-etc.h"

static struct option const long_options[] =
{
  {"help", no_argument, NULL, 'h'},
  {"version", no_argument, NULL, 'v'},
  #if defined(_WIN32)  /*windd*/
  {"list", no_argument, NULL, 'l'},
  {"nolock", no_argument, NULL, 'k'},
  {"nopt", no_argument, NULL, 'n' },
  {"force", no_argument, NULL, 'f'},
  {"confirm", no_argument, NULL, 'c'},
  #endif
  {NULL, 0, NULL, 0}
};

/* Process long options --help and --version, but only if argc == 2.
   Be careful not to gobble up "--".  */

void
parse_long_options (int argc,
                    char **argv,
                    const char *command_name,
                    const char *package,
                    const char *version,
                    void (*usage_func) (int),
                    /* const char *author1, ...*/ ...)
{
  int c;
  int saved_opterr;
  bool list_partitions = false;  /*windd*/
  bool windd_opt = false;

  saved_opterr = opterr;

  /* Don't print an error message for unrecognized options.  */
  opterr = 0;

NextArgument:;
  if ((argc == 2 && (c = getopt_long (argc, argv, "+lhv", long_options, NULL)) != -1)
   #if defined _WIN32
   || (argc >= 2 && (c = getopt_long (argc, argv, "+lknfc", long_options, NULL)) != -1)
   #endif
     )
    {
      switch (c)
        {
        case 'h':
          (*usage_func) (EXIT_SUCCESS);

        case 'v':
          {
            va_list authors;
            va_start (authors, usage_func);
            version_etc_va (stdout, command_name, package, version, authors);
            exit (0);
          }

        case 'l':  /*windd*/
          #define INVALID_DISK    (uint32_t)-1
          { int
              index;
            extern enum EDriveType
              dump_dtype;
            extern uint32_t
              dump_dindex;
            bool
              drive_partition;

            fprintf (stdout, "%s (%s) %s%s\n\n", command_name, package, version,
                             (elevated_process? " [elevated execution]" : ""));

            /* "Parsing command line options with multiple arguments [getopt?]"
               http://stackoverflow.com/questions/15466782/parsing-command-line-options-with-multiple-arguments-getopt#answer-15467257
            */
            index = optind;
            dump_dtype = DTYPE_INVALID;
            dump_dindex = INVALID_DISK;

            if (index < argc)
            {
              if (not is_wdx_id (argv [index], &dump_dtype, &drive_partition))
              {
                fprintf (stderr, "\n[%s] Invalid identifer '%s' specified ... aborting!\n",
                                  PROGRAM_NAME, argv [index]);
                exit (241);
              }

              if (dump_dtype == DTYPE_LOC_DISK && drive_partition)
              {
                fprintf (stderr, "\n[%s] Specifying a partition id with `--list' isn't supported ... aborting!\n",
                                  PROGRAM_NAME);
                exit (241);
              }

              if (IsLocalDisk (dump_dtype))
                wdx2disk (argv [index], &dump_dindex);
              else
                wdx2cdrom (argv [index], &dump_dindex);

              optind = index;
            }

            if (dump_dindex == INVALID_DISK)
            {
              fprintf (stdout,
"Emulated data sources:\n"
DEVFILE_ZERO "         infinite stream of null characters (ASCII NUL, 0x00)\n"
DEVFILE_URANDOM "      std. CSPRNG of Microsofts CAPI (as prov. by CryptGenRandom())\n"
DEVFILE_RANDOM "       provided for compatibility; equivalent to /dev/urandom\n\n"

"Emulated data sinks:\n"
DEVFILE_NULL "         black hole that discards all data written to it\n\n"

"MS-DOS device driver names (R/W):\n"
"CON:              Keyboard and display\n"
"PRN:              System list device, usually a parallel port\n"
"AUX:              Auxiliary device, usually a serial port\n"
/*"CLOCK$          System real-time clock\n"*/
"NUL:              Bit-bucket device (same as " DEVFILE_NULL ")\n"
"A: - Z:           Drive letters (logical volumes)\n"
"COM1: - COM255:   Serial communications port\n"
"LPT1: - LPT255:   Parallel printer port\n\n");
            }

            DumpDrivesAndPartitions ();
          }
          #undef INVALID_DISK
          exit (0);

        case 'k':
          windd_opt = true;
          lock_volumes = false;
          break;

        case 'n':
          windd_opt = true;
          disable_optimizations = true;
          break;

        case 'f':
          windd_opt = true;
          force_operations = true;
          break;

        case 'c':
          windd_opt = true;
          always_confirm = true;
          break;

        default:
          /* Don't process any other long-named options.  */
          break;
        }

    goto NextArgument;
    }

  if (force_operations && always_confirm)
  {
    fprintf (stderr, "\n[%s] Options `--force' and `--confirm' are exclusive ... aborting!\n",
                      PROGRAM_NAME);
    exit (241);
  }

  /* Restore previous value.  */
  opterr = saved_opterr;

  /* Reset this to zero only if we didn't encounter any windd-specific options: */
  #if defined _WIN32
  if (not windd_opt)
  #endif /* ! defined _WIN32 */
  optind = 0;
}
