#include "win32utf16.h"           /* switch to UTF-16 in this file! */

#include <config.h>

#if defined(main)
#undef main                       /* This is the real `main()'! */
#endif

#include "win32\com_supp.h"       /* InitializeCOM(), UninitializeCOM() */
#include "win32/wmi_supp.h"       /* InitializeWMI() */
#include "win32\diskmgmt\diskmgmt_win32.h"  /* diskmgmt_initialize(), WMIEnumDrives(), diskmgmt_free() */

/* Additional _WIN32 headers: */
#include <process.h>              /* _execv() */
#include <shellapi.h>             /* CommandLineToArgvW() */

/*#define EXECUTE_WINDD_AS_CHILD_PROCESS*/


/* */
static LONG
  GetWindowsVer (void);
static void
  SetWinddVersion (void);
static bool
  IsElevated(void);

static BOOL
  CtrlHandler (DWORD ctrl_type);
static LONG WINAPI
  SEH_Handler (PEXCEPTION_POINTERS pExceptionPtrs);

#if defined(EXECUTE_WINDD_AS_CHILD_PROCESS)
static WCHAR *
  EncodeCommandLineBase64 (int const nArgs, LPWSTR const *szArglist);
static void
  DecodeCommandLineBase64 (LPWSTR const *szArglist, int *nArgs, WCHAR ***utf16_argv);
#endif


/* */
bool
  child_process; /* = false; */
bool
  disable_optimizations; /* = false; */  /* Don't attempt any (programmatically chosen) optimizations? */
LONG
  os_version; /* = 0; */
bool
  elevated_process; /* = false; */


/* */
extern int __cdecl main (void)
{ int
    status = 0,
    nArgs;
  LPWSTR *
    szArglist = CommandLineToArgvW (GetCommandLineW(), &nArgs);

  if (szArglist == NULL)
  {
    fprintf (stderr, "\n[%s] CommandLineToArgvW() failed.\n",
                         PROGRAM_NAME);
    exit (255); /*TODO: adjust exit() codes! */
  }

  /* Windows version? */
  os_version = GetWindowsVer ();

  if (os_version < PACKVERSION(6,0))
  {
    fprintf (stderr, "[%s] Windows %hd.%hd detected: this application requires Windows Vista or higher ... aborting.\n",
             PROGRAM_NAME, MAJOR_VERSION(os_version), MINOR_VERSION(os_version));
    exit (254);
  }

  SetWinddVersion ();

  /* Install top-level SEH handler */
  SetUnhandledExceptionFilter (SEH_Handler);

  #if defined(EXECUTE_WINDD_AS_CHILD_PROCESS)
  child_process = (nArgs >= 2 && wcscmp (szArglist[1], L"{A2F721F0-56A0-4abc-873A-EFE2A84149BE}") == 0);
  #else
  child_process = true;
  #endif

  __try
  {
    /* Inital `windd' execution or child process? */
    #if defined(EXECUTE_WINDD_AS_CHILD_PROCESS)
    if (not child_process)
    { WCHAR
       *command_line;
      STARTUPINFO
        si = {0};
      PROCESS_INFORMATION
        pi = {0};

      si.cb = sizeof(si);

      /*status = (int)_wexecv (szArglist [0], szArglist);*/
      command_line = EncodeCommandLineBase64 (nArgs, szArglist);

      if (not CreateProcessW (NULL,           /* No module name (use command line) */
                              command_line,   /* Command line */
                              NULL,           /* Process handle not inheritable */
                              NULL,           /* Thread handle not inheritable */
                              FALSE,          /* Set handle inheritance to FALSE */
                              0,              /* No creation flags */
                              NULL,           /* Use parent's environment block */
                              NULL,           /* Use parent's starting directory */
                              &si,            /* Pointer to STARTUPINFO structure */
                              &pi))           /* Pointer to PROCESS_INFORMATION structure */
      {
        printf ("CreateProcess failed (%d).\n", GetLastError ());
        return (252);
      }

      /* In order to provide more robust error handling the idea is to establish
      // the initial `windd' process as a debugger for the above child process.
      // While the main scaffolding (BASE-64 encoding/decoding of all command
      // line arguments) is in place, all the necessary calls into the "Win32
      // Debug API" are still missing ... for that we'll need to tap into the
      // following two resources:
      // 1. "http://win32assembly.programminghorizon.com/tut28.html"
      // 2. Inside Windows Debugging: A Practical Guide to Debugging and Tracing
      //    Strategies in Windows, Tarik Soulami
      */

      /* Wait until child process exits: */
      WaitForSingleObject (pi.hProcess, INFINITE);

      /* Get exit code: */
      if (not GetExitCodeProcess (pi.hProcess, (DWORD *)&nArgs))
        exit (252);

      /* Close process and thread handles: */
      CloseHandle (pi.hProcess);
      CloseHandle (pi.hThread);

      Free (command_line);
      LocalFree (szArglist);
      szArglist = NULL;
      return (status);
    }
    else
    #endif
    { char **
        utf8_argv;
      int
        status;

      InitializeConsole ();

      /* (BASE-64) argument to arguments array: */
      #if defined(EXECUTE_WINDD_AS_CHILD_PROCESS)
      { int
          prev_argc = nArgs;
        WCHAR **
          utf16_argv;

        DecodeCommandLineBase64 (szArglist, &nArgs, &utf16_argv);

        /* Free previous arguments and store newly decoded array: */
        LocalFree (szArglist);
        szArglist = utf16_argv;
      }
      #endif

      /* Convert arguments to UTF-8: */
      { int
          i = 0;

        utf8_argv = (char **)MemAlloc (nArgs + 1, sizeof(*utf8_argv));
        memset (utf8_argv, 0, (nArgs + 1)*sizeof(*utf8_argv));

        for (i=0; i < nArgs; ++i)
        {
          utf8_argv [i] = utf16_to_utf8 (szArglist [i]);
        /*fprintf (stdout, "Child arg %d: |%s|\n", i, utf8_argv [i]);*/
        }

        #if !defined(EXECUTE_WINDD_AS_CHILD_PROCESS)
        LocalFree (szArglist);
        szArglist = NULL;
        #endif
      }

      /* Initializiation: */
      elevated_process = IsElevated ();

      if (init_msvcr_fileops() < 0)
        goto Failed;

      if (not InitializeCOM() || not InitializeWMI())
        goto Failed;

      if (not dskmgmt_initialize() || not WMIEnumDrives ())
        goto Failed;

      /* Finally we can execute `dd': */
      status = linux_main (nArgs, utf8_argv);

      /* Clean up and return exit code: */
      dskmgmt_free ();

      FreeWMI ();
      UninitializeCOM ();

      RestoreConsole ();

      /* Free converted UTF-8 arguments: */
      { int
          i;

        for (i=0; i < nArgs; ++i)
          Free (utf8_argv [i]);
        Free (utf8_argv);
      }

      FlushStdout ();
      FlushStderr ();
      return (status);

Failed:
      SystemError (253, __FUNCTION__);
      return (253);
    }
  }
  __except (SEH_Handler (GetExceptionInformation ()))
  {
    return (GetExceptionCode ());
  }
}


/* From Windows 8.1 onwards GetVersion() and GetVersionEx()
   have been deprecated! */
typedef NTSTATUS (WINAPI * PFN_RTLGETVERSIONEX) (LPOSVERSIONINFOEX lpVersionInformation);

static LONG GetWindowsVer (void)
{ LONG
    os_version = -1L;

  { PFN_RTLGETVERSIONEX
      RtlGetVersionPtr;

    HMODULE hNtdll = LoadLibrary (TEXT("ntdll.dll"));

    if (not hNtdll)
      goto LoadLibraryFailed;

    RtlGetVersionPtr = (PFN_RTLGETVERSIONEX)GetProcAddress (hNtdll, "RtlGetVersion");

    if (RtlGetVersionPtr == NULL)
      goto GetProcFailed;

    { OSVERSIONINFOEXW
        os_info;

      os_info.dwOSVersionInfoSize = sizeof(os_info);
      if ((*RtlGetVersionPtr) (&os_info) != STATUS_SUCCESS)
        goto GetVersionFailed;

      os_version = PACKVERSION(os_info.dwMajorVersion, os_info.dwMinorVersion);
    }

GetVersionFailed:
GetProcFailed:
    FreeLibrary (hNtdll);
  }

LoadLibraryFailed:
  return (os_version);
}


static char const *GetCompilerVersion (void)
{
/*#if _MSC_VER >= 2000*/  /* VS2016 */
    #define VS15_2016_RTM     200000000
/*#elif _MSC_VER >= 1900*/  /* VS2015 */
    #define VS14_2015_RTM     190023026   /* RTM */
    #define VS14_2015_U1      190023506   /* Update 1 [2015-11-30] */
    #define VS14_2015_U2      190023918   /* Update 2 [2016-03-30] */
    #define VS14_2015_U3      190024210   /* Update 3 [2016-07-27] */
/*#elif _MSC_VER >= 1800*/  /* VS2013 */
    #define VS12_2013_RTM     180021005   /* RTM */
    #define VS12_2013_U1      180021005   /* Update 1 [2014-02-20] (1) */
    #define VS12_2013_U2      180030501   /* Update 2 [2014-05-12] */
    #define VS12_2013_U3      180030723   /* Update 3 [2014-08-04] */
    #define VS12_2013_U4      180031101   /* Update 4 [2014-11-06] */
    #define VS12_2013_U5      180040629   /* Update 5 [2015-07-20] */
/*#elif _MSC_VER >= 1700*/  /* VS2012 */
    #define VS11_2012_RTM     170050727   /* RTM */
    #define VS11_2012_U1      170051106   /* Update 1 [2012-11-26] */
    #define VS11_2012_U2      170060315   /* Update 2 [2013-04-04] */
    #define VS11_2012_U3      170060610   /* Update 3 [2013-06-26] */
    #define VS11_2012_U4      170061030   /* Update 4 [2013-11-13] */
    #define VS11_2012_U5      170061030   /* Update 5 [2015-08-21] (1) */
/*#elif _MSC_VER >= 1600*/  /* VS2010 */
    #define VS10_2010_RTM     160030319   /* RTM */
    #define VS10_2010_SP1     160040219   /* SP1 */
/*#elif _MSC_VER >= 1500*/  /* VS2008 */
    #define VS9_2008_RTM      150021022   /* RTM */
    #define VS9_2008_SP1      150030729   /* SP1 */
/*#endif*/
/*(1) Microsoft apparently neglected to update _MSC_FULL_VER for this release - Doh! */

  #if _MSC_VER >= 2100 || _MSC_VER < 1500
    #error "Unsupported VC++ compiler."
  #endif

  if (_MSC_FULL_VER >= VS15_2016_RTM)
    return ("VS2016");

  if (_MSC_FULL_VER >= VS14_2015_U3)
    return ("VS2015.3");
  if (_MSC_FULL_VER >= VS14_2015_U2)
    return ("VS2015.2");
  if (_MSC_FULL_VER >= VS14_2015_U1)
    return ("VS2015.1");
  if (_MSC_FULL_VER >= VS14_2015_RTM)
    return ("VS2015");

  if (_MSC_FULL_VER >= VS12_2013_U5)
    return ("VS2013.5");
  if (_MSC_FULL_VER >= VS12_2013_U4)
    return ("VS2013.4");
  if (_MSC_FULL_VER >= VS12_2013_U3)
    return ("VS2013.3");
  if (_MSC_FULL_VER >= VS12_2013_U2)
    return ("VS2013.2");
  if (_MSC_FULL_VER >= VS12_2013_U1)
    return ("VS2013.1");
  if (_MSC_FULL_VER >= VS12_2013_RTM)
    return ("VS2013");

  if (_MSC_FULL_VER >= VS11_2012_U5)
    return ("VS2012.5");
  if (_MSC_FULL_VER >= VS11_2012_U4)
    return ("VS2012.4");
  if (_MSC_FULL_VER >= VS11_2012_U3)
    return ("VS2012.3");
  if (_MSC_FULL_VER >= VS11_2012_U2)
    return ("VS2012.2");
  if (_MSC_FULL_VER >= VS11_2012_U1)
    return ("VS2012.1");
  if (_MSC_FULL_VER >= VS11_2012_RTM)
    return ("VS2012");

  if (_MSC_FULL_VER >= VS10_2010_SP1)
    return ("VS2010-SP1");
  if (_MSC_FULL_VER >= VS10_2010_RTM)
    return ("VS2010");

  if (_MSC_FULL_VER >= VS9_2008_SP1)
    return ("VS2008-SP1");
  if (_MSC_FULL_VER >= VS9_2008_RTM)
    return ("VS2008");
}


static void SetWinddVersion (void)
{ static char
    windd_ver [63+1];

  _snprintf (windd_ver, _countof(windd_ver), "%s -- %s build v0.15 [%s]",
             Version, GetCompilerVersion (),
  #if defined(_WIN64) || defined(_M_X64) || defined(_M_AMD64)
             "x64"
  #else
             "x86"
  #endif
            );
  Version = windd_ver;
}


/* Administrative privileges: see "How to check if a process has the administrative rights"
   stackoverflow.com/questions/8046097/how-to-check-if-a-process-has-the-administrative-rights#answer-8196291
*/
static bool IsElevated (void)
{ bool
    is_elevated = false;
  HANDLE
    process_token;

  if (OpenProcessToken (GetCurrentProcess(), TOKEN_QUERY, &process_token))
  { TOKEN_ELEVATION
      elevation;
    DWORD
      ret_length;

    if (GetTokenInformation (process_token, TokenElevation, &elevation, sizeof(elevation), &ret_length))
      is_elevated = (elevation.TokenIsElevated != 0);

    CloseHandle (process_token);
  }

  return (is_elevated);
}


/* `dd.c' global variables: */
#include <signal.h>
extern sig_atomic_t volatile
  info_signal_count;

static BOOL CtrlHandler (DWORD ctrl_type)  /* Control Handler Function */
{
  switch (ctrl_type)
  {
  /* A CTRL+C signal was received */
  case CTRL_C_EVENT:
    cleanup ();
    return (FALSE);  /* allow `windd' to be aborted this way */

  /* The user is closing the console */
  case CTRL_CLOSE_EVENT:
    cleanup ();
    return (FALSE);  /* allow `windd' to be aborted this way */

  /* Simulate SIGINFO to force `print_stats()' to be called: */
  case CTRL_BREAK_EVENT:
    #if defined(IO_STATISTICS_REGISTER_CTRL_BREAK)
    info_signal_count++;
    #else
    cleanup ();
    #endif
    return (TRUE);

  case CTRL_LOGOFF_EVENT:
    cleanup ();
    return (FALSE);  /* allow `windd' to be aborted this way */

  case CTRL_SHUTDOWN_EVENT:
    cleanup ();
    return (FALSE);  /* allow `windd' to be aborted this way */

/*default: break;*/
  }

  return (FALSE);
}

void
install_signal_handlers_win32 (void)
{
  if (not SetConsoleCtrlHandler ((PHANDLER_ROUTINE)CtrlHandler, TRUE))
  {
    fprintf (stderr, "[%s] signal() failed: Ctrl-Break SIGINFO handler will not be available.\n",
                     PROGRAM_NAME);
  /*continue nonetheless!*/
  }
}


/* _WIN32 SEH Exception Handler: */
#include <float.h>

static LONG WINAPI SEH_Handler (PEXCEPTION_POINTERS pExceptionPtrs)
{ DWORD
    exCode;  /* Exception code */

  exCode = pExceptionPtrs->ExceptionRecord->ExceptionCode;

  /* User exception? */
  if ((exCode & 0x20000000) != 0)
    return (EXCEPTION_EXECUTE_HANDLER);

  /* See `WinBase.h': */
  _ftprintf (stderr, TEXT("[%s] Caught an SEH exception:"), TEXT(PROGRAM_NAME));

  switch (exCode)
  {
  case EXCEPTION_ACCESS_VIOLATION:
    { DWORD rwx = (DWORD)pExceptionPtrs->ExceptionRecord->ExceptionInformation [0];
      ULONG_PTR virtAddr = pExceptionPtrs->ExceptionRecord->ExceptionInformation [1];

      _ftprintf  (stderr, TEXT("\n%s violation exception at address %lX.\n"),
      ((rwx==0)? TEXT("Read access") : (rwx==1)? TEXT("Write access") : TEXT("DEP")), virtAddr);
    }
    break;

  case EXCEPTION_DATATYPE_MISALIGNMENT:
    _ftprintf  (stderr, TEXT("\nMisaligned data type exception.\n"));
    break;

  case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    _ftprintf  (stderr, TEXT("\nArray bounds exceeded exception.\n"));
    break;

  case EXCEPTION_FLT_DENORMAL_OPERAND:
    _ftprintf  (stderr, TEXT("\nDenormalized operand FPU exception.\n"));
    _clearfp ();
    break;

  case EXCEPTION_FLT_DIVIDE_BY_ZERO:
    _ftprintf  (stderr, TEXT("\nDivide by Zero FPU exception.\n"));
    _clearfp ();
    break;

  case EXCEPTION_FLT_INEXACT_RESULT:
    _ftprintf  (stderr, TEXT("\nInexact result FPU exception.\n"));
    _clearfp ();
    break;

  case EXCEPTION_FLT_INVALID_OPERATION:
    _ftprintf  (stderr, TEXT("\nInvalid operand FPU exception.\n"));
    _clearfp ();
    break;

  case EXCEPTION_FLT_OVERFLOW:
    _ftprintf  (stderr, TEXT("\nResult overflow FPU exception.\n"));
    _clearfp ();
    break;

  case EXCEPTION_FLT_STACK_CHECK:
    _ftprintf  (stderr, TEXT("\nFloating point stack exhausted exception.\n"));
    _clearfp ();
    break;

  case EXCEPTION_FLT_UNDERFLOW:
    _ftprintf  (stderr, TEXT("\nResult underflow FPU exception.\n"));
    _clearfp ();
    break;

  case EXCEPTION_INT_DIVIDE_BY_ZERO:
    _ftprintf  (stderr, TEXT("\nDivide by Zero exception.\n"));
    break;

  case EXCEPTION_INT_OVERFLOW:
    _ftprintf  (stderr, TEXT("\nResult overflow exception.\n"));
    break;

  case EXCEPTION_PRIV_INSTRUCTION:
    _ftprintf  (stderr, TEXT("\nPrivileged instruction exception.\n"));
    break;

  case EXCEPTION_IN_PAGE_ERROR:
    _ftprintf  (stderr, TEXT("\nIn-page I/O error.\n"));
    break;

  case EXCEPTION_ILLEGAL_INSTRUCTION:
    _ftprintf  (stderr, TEXT("\nIllegal instruction exception.\n"));
    break;

  case EXCEPTION_STACK_OVERFLOW:
    _ftprintf  (stderr, TEXT("\nStack exhausted exception.\n"));
    break;

  case EXCEPTION_INVALID_HANDLE:
    _ftprintf  (stderr, TEXT("\nInvalid handle exception.\n"));
    break;

  default:
    return (EXCEPTION_CONTINUE_SEARCH);
  }

  fflush (stderr);
  return (EXCEPTION_EXECUTE_HANDLER);
}


/* Argument encoder/decoder: */
#if defined(EXECUTE_WINDD_AS_CHILD_PROCESS)
#include <b64/cencode.h>
#include <b64/cdecode.h>


static WCHAR *EncodeCommandLineBase64 (int const nArgs, LPWSTR const *szArglist)
{ WCHAR *
    arglist_string;
  size_t
    total_arglist_len;
  WCHAR *
    arglist_base64;

  /* Store all arguments as a single Multi-String: */
  { size_t
      multi_index;
    int
      i;

    /* Get total argument length: */
    for (i=1, total_arglist_len=0; i < nArgs; ++i)
      total_arglist_len += wcslen (szArglist [i]) + 1/*Null terminator*/;

    ++total_arglist_len;

    /* Copy arguments: */
    arglist_string = (WCHAR *)MemAlloc (total_arglist_len, sizeof(arglist_string[0]));

    for (i=1, multi_index=0; i < nArgs; ++i)
    {
      _tcscpy (arglist_string + multi_index, szArglist [i]);
      multi_index += wcslen (szArglist [i]) + 1/*Null terminator*/;
    }

    arglist_string [multi_index] = L'\0';
    assert (multi_index == total_arglist_len-1);
  }

  total_arglist_len *= sizeof(arglist_string[0]);  /* We have WCHARs not CHARs */

  /* Encode in BASE-64: */
  { int const
      CHARS_PER_LINE = 72;  /* See `cencode.c'! */
    base64_encodestate
      state;
    char *
      enc_output;
    size_t
      max_padded_len = ((4 * total_arglist_len / 3) + 3) & ~3,
      enc_length;

    max_padded_len += CHARS_PER_LINE - 1;
    max_padded_len /= CHARS_PER_LINE;
    max_padded_len *= CHARS_PER_LINE;  /* Account for `CHARS_PER_LINE'! */

    enc_output = (char *)MemAlloc (max_padded_len + 1, sizeof(*enc_output));
    memset (enc_output, 0xFF, max_padded_len + 1);

    base64_init_encodestate (&state);  /* Initialize the encoder state */
    enc_length  = base64_encode_block ((char const *)arglist_string, (int)total_arglist_len, enc_output, &state);
    Free (arglist_string);
    enc_length += base64_encode_blockend (enc_output + enc_length, &state);
    enc_output [enc_length] = '\0';

    arglist_base64 = (WCHAR *)MemAlloc (enc_length + 1, sizeof(*arglist_base64));
    mbstowcs (arglist_base64, enc_output, enc_length + 1);
    Free (enc_output);
  }

  /* See "How to get the application executable name in Windows"
     http://stackoverflow.com/questions/124886/how-to-get-the-application-executable-name-in-windowsc-cli#answer-125079
  */
  { WCHAR
      executable_filename [MAX_PATH + 1];
    size_t const
      argument_guid_len = 38;

    if (not GetModuleFileName (NULL, executable_filename, MAX_PATH))
      wcscpy (executable_filename, szArglist [0]);

    /* Build complete command line: */
    { WCHAR *
        command_line;

      total_arglist_len = wcslen(executable_filename) + 1
                        + argument_guid_len + 1
                        + wcslen(arglist_base64) + 1;
      command_line = (WCHAR *)MemAlloc (total_arglist_len, sizeof(*command_line));
      wcscpy (command_line, executable_filename);
      wcscat (command_line, L" ");
      wcscat (command_line, L"{A2F721F0-56A0-4abc-873A-EFE2A84149BE}");
      wcscat (command_line, L" ");
      wcscat (command_line, arglist_base64);

      /*// For debugging purposes only!
      { FILE *
          f;

        f = fopen ("d:\\cmdline.txt", "w");
        fputws (command_line, f);
        fclose (f);
      }
      exit (200);*/
      return (command_line);
    }
  }
}


static void DecodeCommandLineBase64 (LPWSTR const *szArglist, int *nArgs, WCHAR ***utf16_argv)
{ char *
    enc_output;
  size_t
    enc_length;

  enc_length = wcslen (szArglist [*nArgs-1]);
  enc_output = (char *)MemAlloc (enc_length + 1, sizeof(*enc_output));
  wcstombs (enc_output, szArglist [*nArgs-1], enc_length + 1);

  /* Decode from BASE-64: */
  { base64_decodestate
      state;
    WCHAR *
      dec_output;
    size_t
      dec_length;

    base64_init_decodestate (&state);  /* Initialize the decoder state */
    dec_output = (WCHAR *)MemAlloc (enc_length + 1, sizeof(*dec_output));
    dec_length = base64_decode_block (enc_output, (int const)enc_length, (char *)dec_output, &state);
    dec_length /= sizeof(*dec_output);
    dec_output [dec_length] = '\0';  /* Actually unnecessary: dec_output is already
                                        terminated by two consecutive '\0'! */
    /* Get number of arguments: */
    { WCHAR
        executable_filename [MAX_PATH + 1];
      size_t
        multi_index,
        string_length;

      if (not GetModuleFileName (NULL, executable_filename, MAX_PATH))
        wcscpy (executable_filename, szArglist [0]);

     *nArgs = 1;  /* Executable filename */
      for (multi_index=0; dec_output[multi_index] != '\0'
                       || dec_output[multi_index+1] != '\0'; (*nArgs) += 1)
      {
        multi_index += (multi_index > 0);
        string_length = wcslen (dec_output + multi_index);
        multi_index += string_length;
      }

      /* Store arguments as WCHAR array: */
      { int
          i;

       *utf16_argv = (WCHAR **)MemAlloc (*nArgs + 1, sizeof(WCHAR *));
      (*utf16_argv) [0] = (WCHAR *)MemAlloc (wcslen(executable_filename)+1, sizeof(WCHAR));
        wcscpy ((*utf16_argv) [0], executable_filename);

        for (multi_index=0, i=1; dec_output[multi_index] != '\0'
                              || dec_output[multi_index+1] != '\0'; ++i)
        {
          multi_index += (multi_index > 0);
          string_length = wcslen (dec_output + multi_index);
        (*utf16_argv) [i] = (WCHAR *)MemAlloc (string_length + 1, sizeof(WCHAR));
          wcscpy ((*utf16_argv) [i], dec_output + multi_index);
          multi_index += string_length;
        }

        (*utf16_argv) [i] = NULL;
      }
    }
  }
}
#endif
