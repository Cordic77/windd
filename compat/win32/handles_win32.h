#ifndef WINDD_WINHANDLES_H_
#define WINDD_WINHANDLES_H_

/* */
#define ENUM_HANDLES_CONTINUE true
#define ENUM_HANDLES_STOP     false

/* */
typedef bool
 (*EnumFileHandlesCallbackFunc) (ULONG ProcessId, HANDLE Handle, UCHAR ObjectTypeNumber, LPCWSTR FileName);

/* */
extern bool
  EnumFileHandles (EnumFileHandlesCallbackFunc EnumFileHandlesCallbackPtr, size_t *handle_count);

#endif
