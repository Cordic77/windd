#include "win32utf16.h"           /* switch to UTF-16 in this file! */

#include <config.h>


/* */
#if _MSC_VER >= 1900  /* VS2015? */
/* See `corecrt_internal_stdio.h': */
enum
{
  // If the stream is open in update (read/write) mode, then the _IOUPDATE bit
  // will be set.
  _IOREAD = 0x0001,
  _IOWRITE = 0x0002,
  _IOUPDATE = 0x0004,
};

struct __crt_stdio_stream_data
{
  union
  {
    FILE  _public_file;
    char* _ptr;
  };

  char*            _base;
  int              _cnt;
  long             _flags;
  long             _file;
  int              _charbuf;
  int              _bufsiz;
  char*            _tmpfname;
  CRITICAL_SECTION _lock;
};
#define FILE_WRITABLE(stream)  ((((struct __crt_stdio_stream_data *)(stream))->_flags & (_IOWRITE|_IOUPDATE)) != 0)
#else  /* up to VS2013 */
#define FILE_WRITABLE(stream)  (((stream)->_flag & (_IOWRT|_IORW)) != 0)
#endif


/* */
_locale_t
  cons_locale;

/* stdin/stdout/stderr: */
int
  stdin_fd,
  stdout_fd,
  stderr_fd;
static FILE
 *kbd_stdin = NULL,
 *scr_stdout = NULL,
 *scr_stderr = NULL;
static bool
  stdin_redir,
  stdout_redir,
  stderr_redir;

/* Textmode console: */
HANDLE
  hcon = INVALID_HANDLE_VALUE;
CONSOLE_SCREEN_BUFFER_INFO
  default_csbi;
static unsigned short
  console_color = (BLACK << 4)  /* Background */
                + (LIGHTGRAY);  /* Foreground */


/* */
extern bool InitializeConsole (void)
{ HANDLE
    win_handle;

/*dd:main() { setlocale (LC_ALL, ""); }*/
  cons_locale = _create_locale (LC_ALL, "");

  /* kbd_stdin */
  win_handle = GetStdHandle (STD_INPUT_HANDLE);
  stdin_redir = IsHandleRedirected (win_handle);
  /*if (stdin_redir)*/
  {
    win_handle = CreateFile (TEXT("CONIN$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (win_handle == INVALID_HANDLE_VALUE)
      return (false);
  }
  stdin_fd = crt_open_osfhandle ((intptr_t)win_handle, _O_TEXT);
  if (stdin_fd < 0)
    goto CloseCONIN;
  kbd_stdin = _fdopen (stdin_fd, "r");
  if (kbd_stdin == NULL)
    goto CloseCONINfd;
  setvbuf (kbd_stdin, NULL, _IONBF, 0);     /* Make kbd_stdin unbuffered */

  /* scr_stdout */
  win_handle = GetStdHandle (STD_OUTPUT_HANDLE);
  stdout_redir = IsHandleRedirected (win_handle);
/*if (stdout_redir)*/
  {
    win_handle = CreateFile (TEXT("CONOUT$"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (win_handle == INVALID_HANDLE_VALUE)
      goto CloseCONINfile;
  }
  stdout_fd = crt_open_osfhandle ((intptr_t)win_handle, _O_TEXT);
  if (stdout_fd < 0)
    goto CloseCONOUT;
  scr_stdout = _fdopen (stdout_fd, "w");
  if (kbd_stdin == NULL)
    goto CloseCONOUTfd;
/*setvbuf (scr_stdout, NULL, _IONBF, 0);*/  /* Make scr_stdout unbuffered */

  /* Console setup: */
  hcon = win_handle;
  GetConsoleScreenBufferInfo (hcon, &default_csbi);
  console_color = default_csbi.wAttributes;

  /* scr_stderr */
  win_handle = GetStdHandle (STD_ERROR_HANDLE);
  stderr_redir = IsHandleRedirected (win_handle);
/*if (stderr_redir)*/
  {
    if (not DuplicateHandle (GetCurrentProcess(), win_handle,
                             GetCurrentProcess(), &win_handle,
                             0, FALSE, DUPLICATE_SAME_ACCESS))
      goto CloseCONOUTfile;
  }
  stderr_fd = crt_open_osfhandle ((intptr_t)win_handle, _O_TEXT);
  if (stderr_fd < 0)
    goto CloseCONERR;
  scr_stderr = _fdopen (stderr_fd, "w");
  if (scr_stderr == NULL)
    goto CloseCONERRfd;
  setvbuf (scr_stderr, NULL, _IONBF, 0);    /* Make scr_stderr unbuffered */
  return (true);

/* scr_stderr */
/*CloseCONERRfile:
  fclose (scr_stderr); goto CloseCONOUTfile;*/
CloseCONERRfd:
  crt_close (stderr_fd); goto CloseCONOUTfile;
CloseCONERR:
  if (stderr_redir)
    CloseHandle (win_handle);

/* scr_stdout */
CloseCONOUTfile:
  fclose (scr_stdout); goto CloseCONINfile;
CloseCONOUTfd:
  crt_close (stdout_fd); goto CloseCONINfile;
CloseCONOUT:
  if (stdout_redir)
    CloseHandle (win_handle);

/* kbd_stdin */
CloseCONINfile:
  fclose (kbd_stdin);
  return (false);
CloseCONINfd:
  crt_close (stdin_fd);
  return (false);
CloseCONIN:
  if (stdin_redir)
    CloseHandle (win_handle);
  return (false);
}


extern void RestoreConsole (void)
{
  if (kbd_stdin != NULL)
  {
    fclose (kbd_stdin);
    kbd_stdin = NULL;
  }
  if (scr_stdout != NULL)
  {
    fclose (scr_stdout);
    scr_stdout = NULL;
  }
  if (scr_stderr != NULL)
  {
    fclose (scr_stderr);
    scr_stderr = NULL;
  }
}


extern int IsDescriptorRedirected (int fd)
{/*
 fpos_t
    pos;
  fgetpos (stream, &pos);
  return (pos >= 0);
 */
  int
    tty;

  errno = 0;
  tty = crt_isatty (fd);
  if (errno != 0)
  {
    RecordPosixErr (errno);
    RecordFunction (__FUNCTION__, __LINE__);
    return (-1);
  }

  return (not tty);
}


extern int IsFileRedirected (FILE *stream)
{
  int stream_fd = _fileno (stream);

  return (IsDescriptorRedirected (stream_fd));
}


extern bool IsHandleRedirected (HANDLE handle)
{
  DWORD file_type = GetFileType (handle);

  if (file_type == FILE_TYPE_CHAR)
    return (false);

  return (true);
}


extern void FlushStdout (void)
{
  fflush (scr_stdout);
}


extern void FlushStderr (void)
{
  fflush (scr_stderr);
}


/* Windows console: */
extern int gotoxy (int x, int y)
{
  if (x < 1 || y < 1)
    return (-1);

  x--;
  y--;

  { COORD coord;

    coord.X = (short)x;
    coord.Y = (short)y;

    if (not SetConsoleCursorPosition (hcon, coord))
      WinErrReturn (-1);
  }

  return (0);
}


extern int getcursorxy (int *x, int *y)
{
  if (x!=NULL) *x = 0;
  if (y!=NULL) *y = 0;

  { CONSOLE_SCREEN_BUFFER_INFO
      csbi;

    if (not GetConsoleScreenBufferInfo (hcon, &csbi))
      WinErrReturn (-1);

    if (x!=NULL) *x = csbi.dwCursorPosition.X;
    if (y!=NULL) *y = csbi.dwCursorPosition.Y;
  }

  return (0);
}


extern int getcursorx (void)
{ int
    ret, x;

  if ((ret=getcursorxy (&x, NULL)) != 0)
    return (-abs (ret));

  return (x);
}


extern int getcursory (void)
{ int
    ret, y;

  if ((ret=getcursorxy (NULL, &y)) != 0)
    return (-abs (ret));

  return (y);
}


extern int clrscr (void)
{
  return (system ("cls"));
}


extern int textcolor (enum ETextColor color)
{ int
    prev_foreground;

  if (color == DEFAULT_COLOR)
    color = LIGHTGRAY;

  prev_foreground = (console_color & 0x0F);
  console_color = (unsigned short)((console_color & 0xF0) + (color & 0x0F)); /* Foreground */
  if (not SetConsoleTextAttribute (hcon, console_color))
    return (-1);

  return (prev_foreground);
}


extern int textbackground (enum ETextColor color)
{ int
    prev_background;

  if (color == DEFAULT_COLOR)
    color = BLACK;

  prev_background = (console_color >> 4);
  console_color = (unsigned short)((console_color & 0x0F) + (color << 4)); /* Background */
  if (not SetConsoleTextAttribute (hcon, console_color))
    return (-1);

  return (prev_background);
}


extern FILE *GetConsoleFile (FILE *stream, bool *print_twice)
{
  if (print_twice != NULL)
   *print_twice = false;

  /* kbd_stdin */
  if (stream == stdin)
  {
    return (stdin_redir? kbd_stdin : stream);
  }
  else
  /* scr_stdout */
  if (stream == stdout)
  {
    if (stdout_redir)
    {
   /*if (print_twice != NULL)  // Not a good idea, since `dd' writes to STDOUT_FILENO by default!
       *print_twice = true;*/
      return (scr_stdout);
    }
  }
  else
  /* scr_stderr */
  if (stream == stderr)
  {
    if (stderr_redir)
    {
      if (print_twice != NULL)
       *print_twice = true;
      return (scr_stderr);
    }
  }
  else
  {
    /* In newer versions, `typedef struct _iobuf' only defines a `void* _Placeholder' */
    if (FILE_WRITABLE (stream))
  {
      if (print_twice != NULL)
       *print_twice = true;
      return (scr_stdout);
    }
  }

  return (stream);
}


extern char PromptUserYesNo (void)
{ char
    key;

  do {
    FlushStdout (); FlushStderr ();

    if (not IsFileRedirected (stdin))
      key = _getch ();
    else
      fscanf (stdin, " %c", &key);

      key = toupper (key);
  } while (key!='Y' && key!='N');

  return (key);
}


/*===  scanf functions  ===*/

#if _MSC_VER >= 1800  /* VS2013 */

/* int vfscanf (FILE *stream, char const format [], va_list ap); */
#undef vfscanf
_Check_return_opt_ int (kbdvfscanf) (_Inout_ FILE * const stream, _In_z_ _Printf_format_string_ char const* const format, va_list argptr)
{ FILE *
    keyboard = GetConsoleFile (stream, NULL);

  if (keyboard == scr_stdout)
    keyboard = stream;

  return ((vfscanf) (keyboard, format, argptr));
}

#undef vfwscanf
_Check_return_opt_ int (kbdvfwscanf) (_Inout_ FILE * const stream, _In_z_ _Printf_format_string_ wchar_t const* const format, va_list argptr)
{ FILE *
    keyboard = GetConsoleFile (stream, NULL);

  if (keyboard == scr_stdout)
    keyboard = stream;

  return ((vfwscanf) (keyboard, format, argptr));
}

/* int fscanf (FILE *stream, char const format [], ...); */
#undef fscanf
_Check_return_opt_ int (kbdfscanf) (_Inout_ FILE * const stream, _In_z_ _Scanf_format_string_ char const* const format, ...)
{ va_list
    argptr;
  int
    read;

  va_start (argptr, format);
  read = kbdvfscanf (stream, format, argptr);
  va_end (argptr);
  return (read);
}

#undef fwscanf
_Check_return_opt_ int (kbdfwscanf) (_Inout_ FILE * const stream, _In_z_ _Scanf_format_string_ wchar_t const* const format, ...)
{ va_list
    argptr;
  int
    read;

  va_start (argptr, format);
  read = kbdvfwscanf (stream, format, argptr);
  va_end (argptr);
  return (read);
}


/* int vscanf (char const format [], va_list ap); */
#undef vscanf
_Check_return_opt_ int (kbdvscanf) (_In_z_ _Printf_format_string_ char const* const format, va_list argptr)
{ FILE *
    keyboard = GetConsoleFile (stdin, NULL);

  if (keyboard == scr_stdout)
    keyboard = kbd_stdin;

  return ((vfscanf) (keyboard, format, argptr));
}

#undef vwscanf
_Check_return_opt_ int (kbdvwscanf) (_In_z_ _Printf_format_string_ wchar_t const* const format, va_list argptr)
{ FILE *
    keyboard = GetConsoleFile (stdin, NULL);

  if (keyboard == scr_stdout)
    keyboard = kbd_stdin;

  return ((vfwscanf) (keyboard, format, argptr));
}

/* int scanf (char const format [], ...); */
#undef scanf
_Check_return_opt_ int (kbdscanf) (_In_z_ _Scanf_format_string_ char const* const format, ...)
{ va_list
    argptr;
  int
    read;

  va_start (argptr, format);
  read = kbdvscanf (format, argptr);
  va_end (argptr);
  return (read);
}

#undef wscanf
_Check_return_opt_ int (kbdwscanf) (_In_z_ _Scanf_format_string_ wchar_t const* const format, ...)
{ va_list
    argptr;
  int
    read;

  va_start (argptr, format);
  read = kbdvwscanf (format, argptr);
  va_end (argptr);
  return (read);
}

#endif


/*===  printf functions  ===*/

/* int vfprintf (FILE *stream, char const format [], va_list ap); */
#undef vfprintf
_Check_return_opt_ int (scrvfprintf) (_Inout_ FILE * const stream, _In_z_ _Printf_format_string_ char const* const format, va_list argptr)
{ va_list
    argptr2;
  FILE *
    console;
  bool
    print_twice;

  if ((console=GetConsoleFile (stream, &print_twice)) != NULL)
  {
    gl_va_copy (argptr2, argptr);
    {
      int written = vfprintf (console, format, argptr2);
      va_end (argptr2);
      if (not print_twice)
        return (written);
    }
  }

  return ((vfprintf) (stream, format, argptr));
}

#undef vfwprintf
_Check_return_opt_ int (scrvfwprintf) (_Inout_ FILE * const stream, _In_z_ _Printf_format_string_ wchar_t const* const format, va_list argptr)
{ va_list
    argptr2;
  FILE *
    console;
  bool
    print_twice;

  if ((console=GetConsoleFile (stream, &print_twice)) != NULL)
  {
    gl_va_copy (argptr2, argptr);
    {
      int written = vfwprintf (console, format, argptr2);
      va_end (argptr2);
      if (not print_twice)
        return (written);
    }
  }

  return ((vfwprintf) (stream, format, argptr));
}

/* int fprintf (FILE *stream, char const format [], ...); */
#undef fprintf
_Check_return_opt_ int (scrfprintf) (_Inout_ FILE * const stream, _In_z_ _Printf_format_string_ char const* const format, ...)
{ va_list
    argptr;
  int
    written;

  va_start (argptr, format);
  written = scrvfprintf (stream, format, argptr);
  va_end (argptr);
  return (written);
}

#undef fwprintf
_Check_return_opt_ int (scrfwprintf) (_Inout_ FILE * const stream, _In_z_ _Printf_format_string_ wchar_t const* const format, ...)
{ va_list
    argptr;
  int
    written;

  va_start (argptr, format);
  written = scrvfwprintf (stream, format, argptr);
  va_end (argptr);
  return (written);
}


/* int vprintf (char const format [], va_list ap); */
#undef vprintf
_Check_return_opt_ int (scrvprintf) (_In_z_ _Printf_format_string_ char const* const format, va_list argptr)
{ va_list
    argptr2;
  FILE *
    console;
  bool
    print_twice;

  if ((console=GetConsoleFile (stdout, &print_twice)) != NULL)
  {
    gl_va_copy (argptr2, argptr);
    {
      int written = vfprintf (console, format, argptr2);
      va_end (argptr2);
      if (not print_twice)
        return (written);
    }
  }

  return ((vprintf) (format, argptr));
}

#undef vwprintf
_Check_return_opt_ int (scrvwprintf) (_In_z_ _Printf_format_string_ wchar_t const* const format, va_list argptr)
{ va_list
    argptr2;
  FILE *
    console;
  bool
    print_twice;

  if ((console=GetConsoleFile (stdout, &print_twice)) != NULL)
  {
    gl_va_copy (argptr2, argptr);
    {
      int written = vfwprintf (console, format, argptr2);
      va_end (argptr2);
      if (not print_twice)
        return (written);
    }
  }

  return ((vwprintf) (format, argptr));
}

/* int printf (char const format [], ...); */
#undef printf
_Check_return_opt_ int (scrprintf) (_In_z_ _Printf_format_string_ char const* const format, ...)
{ va_list
    argptr;
  int
    written;

  va_start (argptr, format);
  written = scrvprintf (format, argptr);
  va_end (argptr);
  return (written);
}

#undef wprintf
_Check_return_opt_ int (scrwprintf) (_In_z_ _Printf_format_string_ wchar_t const* const format, ...)
{ va_list
    argptr;
  int
    written;

  va_start (argptr, format);
  written = scrvwprintf (format, argptr);
  va_end (argptr);
  return (written);
}
