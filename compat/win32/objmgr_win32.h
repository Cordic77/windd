#ifndef WINDD_OBJMGR_H_
#define WINDD_OBJMGR_H_

/* */
#define ENUM_ENTRIES_CONTINUE true
#define ENUM_ENTRIES_STOP     false

/* */
enum EObjectTypes
{
  ObjType_Directory, ObjType_SymbolicLink,
  ObjType_Mutant, ObjType_Section, ObjType_Event, ObjType_Semaphore,
  ObjType_Timer, ObjType_Key, ObjType_EventPair, ObjType_IoCompletion,
  ObjType_Device, ObjType_File, ObjType_Controller, ObjType_Profile,
  ObjType_Type, ObjType_Desktop, ObjType_WindowStation, ObjType_Driver,
  ObjType_Token, ObjType_Process, ObjType_Thread, ObjType_Adapter, ObjType_Port,
  ObjType_Count
};

extern LPCWSTR const
  ObjectTypesStr [ObjType_Count];

/* */
typedef bool
 (*EnumObjectDirectoryCallbackFunc) (LPCWSTR ObjectName, enum EObjectTypes ObjectType, LPCWSTR LinksToName);

/* */
extern bool
  EnumObjectDirectory (LPCWSTR ObjectDirectoryPath, EnumObjectDirectoryCallbackFunc EnumObjectDirectoryCallbackPtr);

extern size_t
  NormalizeNTDevicePath (wchar_t nt_device_path []);
extern size_t
  IsDeviceObjectA (char const file [], char const device_prefix [], int *prefix_length);
extern size_t
  IsDeviceObjectW (wchar_t const file [], wchar_t const device_prefix [], int *prefix_length);

#ifdef UNICODE
#define IsDeviceObject IsDeviceObjectW
#else
#define IsDeviceObject IsDeviceObjectA
#endif

extern bool
  EqualDeviceObjectA (char const device_path_1 [], char const device_path_2 [], char const device_prefix []);
extern bool
  EqualDeviceObjectW (wchar_t const device_path_1 [], wchar_t const device_path_2 [], wchar_t const device_prefix []);

#ifdef UNICODE
#define EqualDeviceObject EqualDeviceObjectW
#else
#define EqualDeviceObject EqualDeviceObjectA
#endif

/* "\Device\CdRom0" */
extern size_t
  IsCdromDriveA (char const file []);
extern size_t
  IsCdromDriveW (wchar_t const file []);

#ifdef UNICODE
#define IsCdromDrive IsCdromDriveW
#else
#define IsCdromDrive IsCdromDriveA
#endif

extern bool
  EqualCdromDriveA (char const device_path_1 [], char const device_path_2 []);
extern bool
  EqualCdromDriveW (wchar_t const device_path_1 [], wchar_t const device_path_2 []);

#ifdef UNICODE
#define EqualCdromDrive EqualCdromDriveW
#else
#define EqualCdromDrive EqualCdromDriveA
#endif

/* "\\.\PHYSICALDRIVE" */
extern size_t
  IsPhysicalDriveA (char const file []);
extern size_t
  IsPhysicalDriveW (wchar_t const file []);

#ifdef UNICODE
#define IsPhysicalDrive IsPhysicalDriveW
#else
#define IsPhysicalDrive IsPhysicalDriveA
#endif

extern bool
  EqualPhysicalDriveA (char const device_path_1 [], char const device_path_2 []);
extern bool
  EqualPhysicalDriveW (wchar_t const device_path_1 [], wchar_t const device_path_2 []);

#ifdef UNICODE
#define EqualPhysicalDrive EqualPhysicalDriveW
#else
#define EqualPhysicalDrive EqualPhysicalDriveA
#endif

/* "\Device\HarddiskVolume1" */
extern size_t
  IsHarddiskVolumeA (char const file []);
extern size_t
  IsHarddiskVolumeW (wchar_t const file []);

#ifdef UNICODE
#define IsHarddiskVolume IsHarddiskVolumeW
#else
#define IsHarddiskVolume IsHarddiskVolumeA
#endif

extern bool
  EqualHarddiskVolumeA (char const device_path_1 [], char const device_path_2 []);
extern bool
  EqualHarddiskVolumeW (wchar_t const device_path_1 [], wchar_t const device_path_2 []);

#ifdef UNICODE
#define EqualHarddiskVolume EqualHarddiskVolumeW
#else
#define EqualHarddiskVolume EqualHarddiskVolumeA
#endif

/* "\\?\Volume{916307dc-0000-0000-0000-600000000000}\" */
extern size_t
  IsLogicalVolumeGuidA (char const file []);
extern size_t
  IsLogicalVolumeGuidW (wchar_t const file []);

#ifdef UNICODE
#define IsLogicalVolumeGuid IsLogicalVolumeGuidW
#else
#define IsLogicalVolumeGuid IsLogicalVolumeGuidA
#endif

extern bool
  EqualLogicalVolumeGuidA (char const device_path_1 [], char const device_path_2 []);
extern bool
  EqualLogicalVolumeGuidW (wchar_t const device_path_1 [], wchar_t const device_path_2 []);

#ifdef UNICODE
#define EqualLogicalVolumeGuid EqualLogicalVolumeGuidW
#else
#define EqualLogicalVolumeGuid EqualLogicalVolumeGuidA
#endif

#endif
