#include <config.h>


/* See `winioctl.h': */
#ifndef PARTITION_SPACES
#define PARTITION_SPACES                0xE7      // Storage Spaces protective partition
#endif

int const
  mbr_recognized_type [] = {
    PARTITION_FAT_12,             // 12-bit FAT entries
    PARTITION_XENIX_1,            // Xenix
    PARTITION_XENIX_2,            // Xenix
    PARTITION_FAT_16,             // 16-bit FAT entries
    PARTITION_EXTENDED,           // Extended partition entry
    PARTITION_HUGE,               // Huge partition MS-DOS V4
    PARTITION_IFS,                // IFS Partition
    PARTITION_OS2BOOTMGR,         // OS/2 Boot Manager/OPUS/Coherent swap
    PARTITION_FAT32,              // FAT32
    PARTITION_FAT32_XINT13,       // FAT32 using extended int13 services
    PARTITION_XINT13,             // Win95 partition using extended int13 services
    PARTITION_XINT13_EXTENDED,    // Same as type 5 but uses extended int13 services
    PARTITION_PREP,               // PowerPC Reference Platform (PReP) Boot Partition
    PARTITION_LDM,                // Logical Disk Manager partition
    PARTITION_UNIX,               // Unix
    PARTITION_SPACES              // Storage Spaces protective partition
  };

#ifndef IsRecognizedPartition
#define IsRecognizedPartition( PartitionType ) (    \
    ((PartitionType) == PARTITION_FAT_12)       ||  \
    ((PartitionType) == PARTITION_FAT_16)       ||  \
    ((PartitionType) == PARTITION_HUGE)         ||  \
    ((PartitionType) == PARTITION_IFS)          ||  \
    ((PartitionType) == PARTITION_FAT32)        ||  \
    ((PartitionType) == PARTITION_FAT32_XINT13) ||  \
    ((PartitionType) == PARTITION_XINT13) )
#endif

extern bool MBRTypeRecognizedByWindows (PARTITION_INFORMATION_EX const *part)
{
  if (part == NULL)
    return (false);

  if (IsRecognizedPartition (part->Mbr.PartitionType)
   || part->Mbr.PartitionType == PARTITION_PREP
   || part->Mbr.PartitionType == PARTITION_LDM
   || part->Mbr.PartitionType == PARTITION_SPACES
     )
    return (true);

  return (false);
/*return (part->Mbr.RecognizedPartition);*/  /* always '1'? */
}


/* See `diskguid.h': */
#include <initguid.h>

//
// Define the GPT partition guids known by disk drivers and volume managers.
//
DEFINE_GUID(PARTITION_ENTRY_UNUSED_GUID,    0x00000000L, 0x0000, 0x0000, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);    // Entry unused
DEFINE_GUID(PARTITION_SYSTEM_GUID,          0xC12A7328L, 0xF81F, 0x11D2, 0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B);    // EFI system partition
DEFINE_GUID(PARTITION_MSFT_RESERVED_GUID,   0xE3C9E316L, 0x0B5C, 0x4DB8, 0x81, 0x7D, 0xF9, 0x2D, 0xF0, 0x02, 0x15, 0xAE);    // Microsoft reserved space
DEFINE_GUID(PARTITION_BASIC_DATA_GUID,      0xEBD0A0A2L, 0xB9E5, 0x4433, 0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7);    // Basic data partition
DEFINE_GUID(PARTITION_LDM_METADATA_GUID,    0x5808C8AAL, 0x7E8F, 0x42E0, 0x85, 0xD2, 0xE1, 0xE9, 0x04, 0x34, 0xCF, 0xB3);    // Logical Disk Manager metadata partition
DEFINE_GUID(PARTITION_LDM_DATA_GUID,        0xAF9B60A0L, 0x1431, 0x4F62, 0xBC, 0x68, 0x33, 0x11, 0x71, 0x4A, 0x69, 0xAD);    // Logical Disk Manager data partition
DEFINE_GUID(PARTITION_MSFT_RECOVERY_GUID,   0xDE94BBA4L, 0x06D1, 0x4D40, 0xA1, 0x6A, 0xBF, 0xD5, 0x01, 0x79, 0xD6, 0xAC);    // Microsoft recovery partition
DEFINE_GUID(PARTITION_CLUSTER_GUID,         0xDB97DBA9L, 0x0840, 0x4BAE, 0x97, 0xF0, 0xFF, 0xB9, 0xA3, 0x27, 0xC7, 0xE1);    // Cluster metadata partition
DEFINE_GUID(PARTITION_MSFT_SNAPSHOT_GUID,   0xCADDEBF1L, 0x4400, 0x4DE8, 0xB1, 0x03, 0x12, 0x11, 0x7D, 0xCF, 0x3C, 0xCF);    // Microsoft shadow copy partition
DEFINE_GUID(PARTITION_SPACES_GUID,          0xE75CAF8FL, 0xF680, 0x4CEE, 0xAF, 0xA3, 0xB0, 0x01, 0xE5, 0x6E, 0xFC, 0x2D);    // Storage Spaces protective partition
DEFINE_GUID(PARTITION_PATCH_GUID,           0x8967A686L, 0x96AA, 0x6AA8, 0x95, 0x89, 0xA8, 0x42, 0x56, 0x54, 0x10, 0x90);    // Patch partition

GUID const *
  gpt_recognized_guid [] = {
   &PARTITION_ENTRY_UNUSED_GUID,  // Entry unused
   &PARTITION_SYSTEM_GUID,        // EFI system partition
   &PARTITION_MSFT_RESERVED_GUID, // Microsoft reserved space
   &PARTITION_BASIC_DATA_GUID,    // Basic data partition
   &PARTITION_LDM_METADATA_GUID,  // Logical Disk Manager metadata partition
   &PARTITION_LDM_DATA_GUID,      // Logical Disk Manager data partition
   &PARTITION_MSFT_RECOVERY_GUID, // Microsoft recovery partition
   &PARTITION_CLUSTER_GUID,       // Cluster metadata partition
   &PARTITION_MSFT_SNAPSHOT_GUID, // Microsoft shadow copy partition
   &PARTITION_SPACES_GUID,        // Storage Spaces protective partition
   &PARTITION_PATCH_GUID,         // Patch partition
  };

extern bool GPTGuidRecognizedByWindows (PARTITION_INFORMATION_EX const *part)
{
  if (part == NULL)
    return (false);

  { size_t
      i;

    for (i = 0; i < _countof (gpt_recognized_guid); ++i)
    {
      if (IsEqualGUID (&part->Gpt.PartitionType, gpt_recognized_guid [i]))
        return (true);
    }
  }

  return (false);
}


/* List of partition identifiers for PCs:
   https://www.win.tue.nl/~aeb/partitions/partition_types-1.html
   Last updated: 2016-09-21, 182 partition identifers total
   -> 181 listed on www.win.tue.nl
   ->  +1 additional listed in `winioctl.h': PARTITION_SPACES
*/
struct MBRPartitionType {
  unsigned char type;
  char const   *desc;
};

struct MBRPartitionType const mbr_part_type [] = {
     {0x00, "Empty"}
    ,{PARTITION_FAT_12, "DOS 12-bit FAT"}
    ,{PARTITION_XENIX_1, "XENIX root"}
    ,{PARTITION_XENIX_2, "XENIX usr"}
    ,{PARTITION_FAT_16, "DOS 3.0+ 16-bit FAT (up to 32M)"}
    ,{PARTITION_EXTENDED, "Extended partition"}
    ,{PARTITION_HUGE, "DOS 3.31+ 16-bit FAT (over 32M)"}
    ,{PARTITION_IFS, "HPFS/NTFS/exFAT"}
    ,{0x08, "AIX data partition"}
    ,{0x09, "AIX boot partition"}
    ,{PARTITION_OS2BOOTMGR, "OS/2 Boot Manager"}
    ,{PARTITION_FAT32, "WIN95 OSR2 FAT32"}
    ,{PARTITION_FAT32_XINT13, "WIN95 OSR2 FAT32 (LBA-mapped)"}
    ,{PARTITION_XINT13, "WIN95 FAT16 (LBA-mapped)"}
    ,{PARTITION_XINT13_EXTENDED, "WIN95 Extended partition (LBA-mapped)"}
    ,{0x10, "OPUS"}
    ,{0x11, "Hidden DOS 12-bit FAT"}
    ,{0x12, "Compaq diagnostic partition"}
    ,{0x14, "Hidden DOS 16-bit FAT <32M"}
    ,{0x16, "Hidden DOS 16-bit FAT >=32M"}
    ,{0x17, "Hidden HPFS/NTFS/exFAT"}
    ,{0x18, "AST SmartSleep Partition"}
    ,{0x1b, "Hidden WIN95 OSR2 FAT32"}
    ,{0x1c, "Hidden WIN95 OSR2 FAT32 (LBA-mapped)"}
    ,{0x1e, "Hidden WIN95 FAT16 (LBA-mapped)"}
    ,{0x24, "NEC DOS 3.x"}
    ,{0x27, "Windows RE hidden partition"}
    ,{0x2a, "AtheOS File System (AFS)"}
    ,{0x2b, "SyllableSecure (SylStor)"}
    ,{0x32, "Network operating system (NOS)"}
    ,{0x35, "JFS on OS/2 or eComStation"}
    ,{0x38, "THEOS ver 3.2 2gb partition"}
    ,{0x39, "Plan 9 partition"}
    ,{0x3a, "THEOS ver 4 4gb partition"}
    ,{0x3b, "THEOS ver 4 extended partition"}
    ,{0x3c, "PartitionMagic recovery partition"}
    ,{0x3d, "Hidden NetWare"}
    ,{0x40, "Venix 80286"}
    ,{PARTITION_PREP, "PPC Power PC Reference Platform Boot"}
    ,{PARTITION_LDM, "Logical Disk Manager partition"}
    ,{0x43, "Linux native (sharing disk with DRDOS)"}
    ,{0x44, "GoBack partition"}
    ,{0x45, "EUMEL/Elan"}
    ,{0x46, "EUMEL/Elan"}
    ,{0x47, "EUMEL/Elan"}
    ,{0x48, "EUMEL/Elan"}
    ,{0x4a, "ALFS/THIN lightweight filesystem for DOS"}
    ,{0x4c, "Oberon partition"}
    ,{0x4d, "QNX4.x"}
    ,{0x4e, "QNX4.x 2nd part"}
    ,{0x4f, "QNX4.x 3rd part"}
    ,{0x50, "OnTrack Disk Manager (RO)"}
    ,{0x51, "OnTrack Disk Manager Aux1 (RW)"}
    ,{0x52, "CP/M"}
    ,{0x53, "Disk Manager 6.0 Aux3"}
    ,{0x54, "Disk Manager 6.0 Dynamic Drive Overlay"}
    ,{0x55, "EZ Drive"}
    ,{0x56, "Golden Bow VFeature Partitioned Volume"}
    ,{0x57, "VNDI Partition"}
    ,{0x5c, "Priam EDisk"}
    ,{0x61, "SpeedStor"}
    ,{PARTITION_UNIX, "Unix System V/Mach/GNU Hurd"}
    ,{0x64, "Novell Netware 286"}
    ,{0x65, "Novell Netware 386"}
    ,{0x66, "Novell Netware SMS Partition"}
    ,{0x67, "Novell"}
    ,{0x68, "Novell"}
    ,{0x69, "Novell Netware NSS Partition"}
    ,{0x70, "DiskSecure Multi-Boot"}
    ,{0x72, "UNIX Version 7 port (V7/x86)"}
    ,{0x74, "Scramdisk partition"}
    ,{0x75, "IBM PC/IX"}
    ,{0x77, "M2FS/M2CS partition"}
    ,{0x78, "XOSL FS"}
    ,{0x80, "Minix <1.4a"}
    ,{0x81, "Minix >=1.4b"}
    ,{0x82, "Linux swap/Solaris x86"}
    ,{0x83, "Linux native partition"}
    ,{0x84, "OS/2 hidden C: drive"}
    ,{0x85, "Linux extended partition"}
    ,{0x86, "Legacy Fault Tolerant FAT16 volume"}
    ,{0x87, "Legacy Fault Tolerant NTFS volume"}
    ,{0x88, "Linux plaintext partition table"}
    ,{0x8a, "Linux Kernel Partition (used by AiR-BOOT)"}
    ,{0x8b, "Legacy Fault Tolerant FAT32 volume"}
    ,{0x8c, "Legacy Fault Tolerant FAT32 (LBA mapped)"}
    ,{0x8d, "Free FDISK hidden Primary DOS FAT12"}
    ,{0x8e, "Linux Logical Volume Manager (LVM)"}
    ,{0x90, "Free FDISK hidden Primary DOS FAT16"}
    ,{0x91, "Free FDISK hidden DOS ext. partitition"}
    ,{0x92, "Free FDISK hidden Primary DOS large FAT16"}
    ,{0x93, "Amoeba"}
    ,{0x94, "Amoeba bad block table"}
    ,{0x95, "MIT EXOPC native partitions"}
    ,{0x96, "CHRP ISO-9660 filesystem"}
    ,{0x97, "Free FDISK hidden Primary DOS FAT32"}
    ,{0x98, "Free FDISK hidden Primary DOS FAT32 (LBA)"}
    ,{0x99, "DCE376 logical drive"}
    ,{0x9a, "Free FDISK hidden Primary DOS FAT16 (LBA)"}
    ,{0x9b, "Free FDISK hidden DOS ext. part. (LBA)"}
    ,{0x9e, "ForthOS partition"}
    ,{0x9f, "BSD/OS"}
    ,{0xa0, "Laptop hibernation partition"}
    ,{0xa1, "HP Volume Expansion (SpeedStor variant)"}
    ,{0xa3, "HP Volume Expansion (SpeedStor variant)"}
    ,{0xa4, "HP Volume Expansion (SpeedStor variant)"}
    ,{0xa5, "FreeBSD/NetBSD"}
    ,{0xa6, "OpenBSD"}
    ,{0xa7, "NeXTStep"}
    ,{0xa8, "Mac OS-X"}
    ,{0xa9, "NetBSD"}
    ,{0xaa, "Olivetti Fat 12 1.44MB Service Partition"}
    ,{0xab, "Mac OS-X Boot partition"}
    ,{0xad, "RISC OS ADFS"}
    ,{0xae, "ShagOS filesystem"}
    ,{0xaf, "MacOS X HFS"}
    ,{0xb0, "BootStar Dummy"}
    ,{0xb1, "QNX Neutrino Power-Safe filesystem"}
    ,{0xb2, "QNX Neutrino Power-Safe filesystem"}
    ,{0xb3, "QNX Neutrino Power-Safe filesystem"}
    ,{0xb4, "HP Volume Expansion (SpeedStor variant)"}
    ,{0xb6, "Corrupted Windows NT mirror set (master)"}
    ,{0xb7, "BSDI BSD/386 filesystem"}
    ,{0xb8, "BSDI BSD/386 swap partition"}
    ,{0xbb, "Boot Wizard hidden"}
    ,{0xbc, "Acronis backup partition"}
    ,{0xbd, "BonnyDOS/286"}
    ,{0xbe, "Solaris 8 boot partition"}
    ,{0xbf, "New Solaris x86 partition"}
    ,{0xc0, "DR-DOS/Novell DOS secured partition"}
    ,{0xc1, "DRDOS/secured (FAT-12)"}
    ,{0xc2, "Hidden Linux"}
    ,{0xc3, "Hidden Linux swap"}
    ,{0xc4, "DRDOS/secured (FAT-16, < 32M)"}
    ,{0xc5, "DRDOS/secured (extended)"}
    ,{0xc6, "DRDOS/secured (FAT-16, >= 32M)"}
    ,{0xc7, "Syrinx boot"}
    ,{0xc8, "Reserved for DR-DOS 8.0+"}
    ,{0xc9, "Reserved for DR-DOS 8.0+"}
    ,{0xca, "Reserved for DR-DOS 8.0+"}
    ,{0xcb, "DR-DOS 7.04+ secured FAT32 (CHS)"}
    ,{0xcc, "DR-DOS 7.04+ secured FAT32 (LBA)"}
    ,{0xcd, "CTOS Memdump?"}
    ,{0xce, "DR-DOS 7.04+ FAT16X (LBA)"}
    ,{0xcf, "DR-DOS 7.04+ secured extended DOS (LBA)"}
    ,{0xd0, "Multiuser DOS secured partition"}
    ,{0xd1, "Old Multiuser DOS secured FAT12"}
    ,{0xd4, "Old Multiuser DOS secured FAT16 <32M"}
    ,{0xd5, "Old Multiuser DOS secured ext. partition"}
    ,{0xd6, "Old Multiuser DOS secured FAT16 >=32M"}
    ,{0xd8, "CP/M-86"}
    ,{0xda, "Non-FS data"}
    ,{0xdb, "CP/M / CTOS"}
    ,{0xde, "Dell PowerEdge Server utilities"}
    ,{0xdf, "BootIt EMBRM"}
    ,{0xe0, "STMicroelectronics ST AVFS filesystem"}
    ,{0xe1, "SpeedStor 12-bit FAT ext. partition"}
    ,{0xe3, "DOS R/O or SpeedStor"}
    ,{0xe4, "SpeedStor 16-bit FAT ext. part.<1024 cyl."}
    ,{0xe5, "Tandy MSDOS with logically sectored FAT"}
    ,{0xe6, "Storage Dimensions SpeedStor"}
    ,{PARTITION_SPACES, "Storage Spaces protective partition"}
    ,{0xe8, "Linux Unified Key Setup (LUKS) enc. part."}
    ,{0xea, "Freedesktop boot"}
    ,{0xeb, "BeOS BFS"}
    ,{0xec, "SkyOS SkyFS"}
    ,{0xee, "Legacy MBR followed by GPT header"}
    ,{0xef, "Partition containing an EFI file system"}
    ,{0xf0, "Linux/PA-RISC boot loader"}
    ,{0xf1, "Storage Dimensions SpeedStor"}
    ,{0xf2, "DOS 3.3+ secondary partition"}
    ,{0xf4, "SpeedStor large partition"}
    ,{0xf5, "Prologue multi-volume partition"}
    ,{0xf6, "Storage Dimensions SpeedStor"}
    ,{0xf7, "DDRdrive Solid State File System"}
    ,{0xf9, "pCache"}
    ,{0xfa, "Bochs open source IA-32 (x86) PC emulator"}
    ,{0xfb, "VMware File System partition"}
    ,{0xfc, "VMware Swap partition"}
    ,{0xfd, "Linux RAID persistent superblock autodet."}
    ,{0xfe, "LANstep"}
    ,{0xff, "Xenix Bad Block Table"}
  };

extern char const *mbr_lookup_parttype (unsigned char const partition_type)
{ size_t
    i;

  for (i=0; i < _countof(mbr_part_type); ++i)
    if (mbr_part_type [i].type == partition_type)
      return (mbr_part_type [i].desc);

  return (NULL);
}


/* Partition type GUIDs:
   https://en.wikipedia.org/wiki/GUID_Partition_Table#Partition_type_GUIDs
   Last updated: 2016-09-20, 105 GPT partition type GUIDs total
   -> 102 listed on Wikipedia
   ->  +3 additional listed in `diskguid.h': PARTITION_CLUSTER_GUID, PARTITION_MSFT_SNAPSHOT_GUID, PARTITION_PATCH_GUID
*/
DEFINE_GUID(GPT_MBR_PARTITION_SCHEME, 0x024DEE41L, 0x33E7, 0x11D3, 0x9D, 0x69, 0x00, 0x08, 0xC7, 0x81, 0xF3, 0x9F);
DEFINE_GUID(GPT_BIOS_BOOT_PARTITION, 0x21686148L, 0x6449, 0x6E6F, 0x74, 0x4E, 0x65, 0x65, 0x64, 0x45, 0x46, 0x49);
DEFINE_GUID(GPT_INTEL_FAST_FLASH, 0xD3BFE2DEL, 0x3DAF, 0x11DF, 0xBA, 0x40, 0xE3, 0xA5, 0x56, 0xD8, 0x95, 0x93);
DEFINE_GUID(GPT_SONY_BOOT_PARTITION, 0xF4019732L, 0x066E, 0x4E12, 0x82, 0x73, 0x34, 0x6C, 0x56, 0x41, 0x49, 0x4F);
DEFINE_GUID(GPT_LENOVO_BOOT_PARTITION, 0xBFBFAFE7L, 0xA34F, 0x448A, 0x9A, 0x5B, 0x62, 0x13, 0xEB, 0x73, 0x6C, 0x22);
DEFINE_GUID(GPT_IBM_GENERAL_PARALLEL_FS, 0x37AFFC90L, 0xEF7D, 0x4E96, 0x91, 0xC3, 0x2D, 0x7A, 0xE0, 0x55, 0xB1, 0x74);
/* HP-UX */
DEFINE_GUID(GPT_HPUX_DATA_PARTITION, 0x75894C1EL, 0x3AEB, 0x11D3, 0xB7, 0xC1, 0x7B, 0x03, 0xA0, 0x00, 0x00, 0x00);
DEFINE_GUID(GPT_HPUX_SERVICE_PARTITION, 0xE2A1E728L, 0x32E3, 0x11D6, 0xA6, 0x82, 0x7B, 0x03, 0xA0, 0x00, 0x00, 0x00);
/* Linux */
DEFINE_GUID(GPT_LINUX_FILESYSTEM_DATA, 0x0FC63DAFL, 0x8483, 0x4772, 0x8E, 0x79, 0x3D, 0x69, 0xD8, 0x47, 0x7D, 0xE4);
DEFINE_GUID(GPT_LINUX_RAID_PARTITION, 0xA19D880FL, 0x05FC, 0x4D3B, 0xA0, 0x06, 0x74, 0x3F, 0x0F, 0x84, 0x91, 0x1E);
DEFINE_GUID(GPT_LINUX_ROOT_PARTITION_X86, 0x44479540L, 0xF297, 0x41B2, 0x9A, 0xF7, 0xD1, 0x31, 0xD5, 0xF0, 0x45, 0x8A);
DEFINE_GUID(GPT_LINUX_ROOT_PARTITION_X64, 0x4F68BCE3L, 0xE8CD, 0x4DB1, 0x96, 0xE7, 0xFB, 0xCA, 0xF9, 0x84, 0xB7, 0x09);
DEFINE_GUID(GPT_LINUX_ROOT_PARTITION_ARM32, 0x69DAD710L, 0x2CE4, 0x4E3C, 0xB1, 0x6C, 0x21, 0xA1, 0xD4, 0x9A, 0xBE, 0xD3);
DEFINE_GUID(GPT_LINUX_ROOT_PARTITION_ARM64, 0xB921B045L, 0x1DF0, 0x41C3, 0xAF, 0x44, 0x4C, 0x6F, 0x28, 0x0D, 0x3F, 0xAE);
DEFINE_GUID(GPT_LINUX_SWAP_PARTITION, 0x0657FD6DL, 0xA4AB, 0x43C4, 0x84, 0xE5, 0x09, 0x33, 0xC8, 0x4B, 0x4F, 0x4F);
DEFINE_GUID(GPT_LINUX_LVM_PARTITION, 0xE6D6D379L, 0xF507, 0x44C2, 0xA2, 0x3C, 0x23, 0x8F, 0x2A, 0x3D, 0xF9, 0x28);
DEFINE_GUID(GPT_LINUX_HOME_PARTITION, 0x933AC7E1L, 0x2EB4, 0x4F13, 0xB8, 0x44, 0x0E, 0x14, 0xE2, 0xAE, 0xF9, 0x15);
DEFINE_GUID(GPT_LINUX_SRV_PARTITION, 0x3B8F8425L, 0x20E0, 0x4F3B, 0x90, 0x7F, 0x1A, 0x25, 0xA7, 0x6F, 0x98, 0xE8);
DEFINE_GUID(GPT_LINUX_DMCRYPT_PARTITION, 0x7FFEC5C9L, 0x2D00, 0x49B7, 0x89, 0x41, 0x3E, 0xA1, 0x0A, 0x55, 0x86, 0xB7);
DEFINE_GUID(GPT_LINUX_LUKS_PARTITION, 0xCA7D7CCBL, 0x63ED, 0x4C53, 0x86, 0x1C, 0x17, 0x42, 0x53, 0x60, 0x59, 0xCC);
DEFINE_GUID(GPT_LINUX_RESERVED_PARTITION, 0x8DA63339L, 0x0007, 0x60C0, 0xC4, 0x36, 0x08, 0x3A, 0xC8, 0x23, 0x09, 0x08);
/* FreeBSD */
DEFINE_GUID(GPT_FREEBSD_BOOT_PARTITION, 0x83BD6B9DL, 0x7F41, 0x11DC, 0xBE, 0x0B, 0x00, 0x15, 0x60, 0xB8, 0x4F, 0x0F);
DEFINE_GUID(GPT_FREEBSD_DATA_PARTITION, 0x516E7CB4L, 0x6ECF, 0x11D6, 0x8F, 0xF8, 0x00, 0x02, 0x2D, 0x09, 0x71, 0x2B);
DEFINE_GUID(GPT_FREEBSD_SWAP_PARTITION, 0x516E7CB5L, 0x6ECF, 0x11D6, 0x8F, 0xF8, 0x00, 0x02, 0x2D, 0x09, 0x71, 0x2B);
DEFINE_GUID(GPT_FREEBSD_UFS_PARTITION, 0x516E7CB6L, 0x6ECF, 0x11D6, 0x8F, 0xF8, 0x00, 0x02, 0x2D, 0x09, 0x71, 0x2B);
DEFINE_GUID(GPT_FREEBSD_VINUM_VOLUME_MGR, 0x516E7CB8L, 0x6ECF, 0x11D6, 0x8F, 0xF8, 0x00, 0x02, 0x2D, 0x09, 0x71, 0x2B);
DEFINE_GUID(GPT_FREEBSD_ZFS_PARTITION, 0x516E7CBAL, 0x6ECF, 0x11D6, 0x8F, 0xF8, 0x00, 0x02, 0x2D, 0x09, 0x71, 0x2B);
/* OS X */
DEFINE_GUID(GPT_OSX_HFSPLUS_PARTITION, 0x48465300L, 0x0000, 0x11AA, 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC);
DEFINE_GUID(GPT_OSX_APPLE_UFS_PARTITION, 0x55465300L, 0x0000, 0x11AA, 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC);
DEFINE_GUID(GPT_OSX_ZFS_PARTITION, 0x6A898CC3L, 0x1DD2, 0x11B2, 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31);
DEFINE_GUID(GPT_OSX_APPLE_RAID_PARTITION, 0x52414944L, 0x0000, 0x11AA, 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC);
DEFINE_GUID(GPT_OSX_APPLE_RAID_OFFLINE, 0x52414944L, 0x5F4F, 0x11AA, 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC);
DEFINE_GUID(GPT_OSX_RECOVERY_HD_BOOT, 0x426F6F74L, 0x0000, 0x11AA, 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC);
DEFINE_GUID(GPT_OSX_APPLE_LABEL, 0x4C616265L, 0x6C00, 0x11AA, 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC);
DEFINE_GUID(GPT_APPLETV_RECOVERY_PARTITION, 0x5265636FL, 0x7665, 0x11AA, 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC);
DEFINE_GUID(GPT_APPLE_CORE_STORAGE, 0x53746F72L, 0x6167, 0x11AA, 0xAA, 0x11, 0x00, 0x30, 0x65, 0x43, 0xEC, 0xAC);
/* Solaris / illumos */
DEFINE_GUID(GPT_SOLARIS_BOOT_PARTITION, 0x6A82CB45L, 0x1DD2, 0x11B2, 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31);
DEFINE_GUID(GPT_SOLARIS_ROOT_PARTITION, 0x6A85CF4DL, 0x1DD2, 0x11B2, 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31);
DEFINE_GUID(GPT_SOLARIS_SWAP_PARTITION, 0x6A87C46FL, 0x1DD2, 0x11B2, 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31);
DEFINE_GUID(GPT_SOLARIS_BACKUP_PARTITION, 0x6A8B642BL, 0x1DD2, 0x11B2, 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31);
DEFINE_GUID(GPT_SOLARIS_USR_PARTITION, 0x6A898CC3L, 0x1DD2, 0x11B2, 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31);
DEFINE_GUID(GPT_SOLARIS_VAR_PARTITION, 0x6A8EF2E9L, 0x1DD2, 0x11B2, 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31);
DEFINE_GUID(GPT_SOLARIS_HOME_PARTITION, 0x6A90BA39L, 0x1DD2, 0x11B2, 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31);
DEFINE_GUID(GPT_SOLARIS_ALTERNATE_SECTOR, 0x6A9283A5L, 0x1DD2, 0x11B2, 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31);
DEFINE_GUID(GPT_SOLARIS_RESERVED_PARTITION_1, 0x6A945A3BL, 0x1DD2, 0x11B2, 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31);
DEFINE_GUID(GPT_SOLARIS_RESERVED_PARTITION_2, 0x6A9630D1L, 0x1DD2, 0x11B2, 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31);
DEFINE_GUID(GPT_SOLARIS_RESERVED_PARTITION_3, 0x6A980767L, 0x1DD2, 0x11B2, 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31);
DEFINE_GUID(GPT_SOLARIS_RESERVED_PARTITION_4, 0x6A96237FL, 0x1DD2, 0x11B2, 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31);
DEFINE_GUID(GPT_SOLARIS_RESERVED_PARTITION_5, 0x6A8D2AC7L, 0x1DD2, 0x11B2, 0x99, 0xA6, 0x08, 0x00, 0x20, 0x73, 0x66, 0x31);
/* NetBSD */
DEFINE_GUID(GPT_NETBSD_SWAP_PARTITION, 0x49F48D32L, 0xB10E, 0x11DC, 0xB9, 0x9B, 0x00, 0x19, 0xD1, 0x87, 0x96, 0x48);
DEFINE_GUID(GPT_NETBSD_FFS_PARTITION, 0x49F48D5AL, 0xB10E, 0x11DC, 0xB9, 0x9B, 0x00, 0x19, 0xD1, 0x87, 0x96, 0x48);
DEFINE_GUID(GPT_NETBSD_LFS_PARTITION, 0x49F48D82L, 0xB10E, 0x11DC, 0xB9, 0x9B, 0x00, 0x19, 0xD1, 0x87, 0x96, 0x48);
DEFINE_GUID(GPT_NETBSD_RAID_PARTITION, 0x49F48DAAL, 0xB10E, 0x11DC, 0xB9, 0x9B, 0x00, 0x19, 0xD1, 0x87, 0x96, 0x48);
DEFINE_GUID(GPT_NETBSD_CONCATENATED_PARTITION, 0x2DB519C4L, 0xB10F, 0x11DC, 0xB9, 0x9B, 0x00, 0x19, 0xD1, 0x87, 0x96, 0x48);
DEFINE_GUID(GPT_NETBSD_ENCRYPTED_PARTITION, 0x2DB519ECL, 0xB10F, 0x11DC, 0xB9, 0x9B, 0x00, 0x19, 0xD1, 0x87, 0x96, 0x48);
/* ChromeOS */
DEFINE_GUID(GPT_CHROMEOS_KERNEL_PARTITION, 0xFE3A2A5DL, 0x4F32, 0x41A7, 0xB7, 0x25, 0xAC, 0xCC, 0x32, 0x85, 0xA3, 0x09);
DEFINE_GUID(GPT_CHROMEOS_ROOTFS_PARTITION, 0x3CB8E202L, 0x3B7E, 0x47DD, 0x8A, 0x3C, 0x7F, 0xF2, 0xA1, 0x3C, 0xFC, 0xEC);
DEFINE_GUID(GPT_CHROMEOS_FUTURE_USE, 0x2E0A753DL, 0x9E48, 0x43B0, 0x83, 0x37, 0xB1, 0x51, 0x92, 0xCB, 0x1B, 0x5E);
/* Haiku */
DEFINE_GUID(GPT_HAIKU_BFS_PARTITION, 0x42465331L, 0x3BA3, 0x10F1, 0x80, 0x2A, 0x48, 0x61, 0x69, 0x6B, 0x75, 0x21);
DEFINE_GUID(GPT_MIDNIGHTBSD_BOOT_PARTITION, 0x85D5E45EL, 0x237C, 0x11E1, 0xB4, 0xB3, 0xE8, 0x9A, 0x8F, 0x7F, 0xC3, 0xA7);
DEFINE_GUID(GPT_MIDNIGHTBSD_DATA_PARTITION, 0x85D5E45AL, 0x237C, 0x11E1, 0xB4, 0xB3, 0xE8, 0x9A, 0x8F, 0x7F, 0xC3, 0xA7);
DEFINE_GUID(GPT_MIDNIGHTBSD_SWAP_PARTITION, 0x85D5E45BL, 0x237C, 0x11E1, 0xB4, 0xB3, 0xE8, 0x9A, 0x8F, 0x7F, 0xC3, 0xA7);
DEFINE_GUID(GPT_MIDNIGHTBSD_UFS_PARTITION, 0x0394EF8BL, 0x237E, 0x11E1, 0xB4, 0xB3, 0xE8, 0x9A, 0x8F, 0x7F, 0xC3, 0xA7);
DEFINE_GUID(GPT_MIDNIGHTBSD_VINUM_VOLUME_MGR, 0x85D5E45CL, 0x237C, 0x11E1, 0xB4, 0xB3, 0xE8, 0x9A, 0x8F, 0x7F, 0xC3, 0xA7);
DEFINE_GUID(GPT_MIDNIGHTBSD_ZFS_PARTITION, 0x85D5E45DL, 0x237C, 0x11E1, 0xB4, 0xB3, 0xE8, 0x9A, 0x8F, 0x7F, 0xC3, 0xA7);
/* Ceph */
DEFINE_GUID(GPT_CEPH_JOURNAL_PARTITION, 0x45B0969EL, 0x9B03, 0x4F30, 0xB4, 0xC6, 0xB4, 0xB8, 0x0C, 0xEF, 0xF1, 0x06);
DEFINE_GUID(GPT_CEPH_DMCRYPT_ENCRYPTED_JOURNAL, 0x45B0969EL, 0x9B03, 0x4F30, 0xB4, 0xC6, 0x5E, 0xC0, 0x0C, 0xEF, 0xF1, 0x06);
DEFINE_GUID(GPT_CEPH_OBJECT_STORAGE_DAEMON, 0x4FBD7E29L, 0x9D25, 0x41B8, 0xAF, 0xD0, 0x06, 0x2C, 0x0C, 0xEF, 0xF0, 0x5D);
DEFINE_GUID(GPT_CEPH_ENCRYPTED_OSD_PARTITION, 0x4FBD7E29L, 0x9D25, 0x41B8, 0xAF, 0xD0, 0x5E, 0xC0, 0x0C, 0xEF, 0xF0, 0x5D);
DEFINE_GUID(GPT_CEPH_DISK_IN_CREATION, 0x89C57F98L, 0x2FE5, 0x4DC0, 0x89, 0xC1, 0xF3, 0xAD, 0x0C, 0xEF, 0xF2, 0xBE);
DEFINE_GUID(GPT_CEPH_DMCRYPT_DISK_IN_CREATION, 0x89C57F98L, 0x2FE5, 0x4DC0, 0x89, 0xC1, 0x5E, 0xC0, 0x0C, 0xEF, 0xF2, 0xBE);
/* OpenBSD */
DEFINE_GUID(GPT_OPENBSD_DATA_PARTITION, 0x824CC7A0L, 0x36A8, 0x11E3, 0x89, 0x0A, 0x95, 0x25, 0x19, 0xAD, 0x3F, 0x61);
/* QNX */
DEFINE_GUID(GPT_QNX_POWER_SAFE_FILESYSTEM, 0xCEF5A9ADL, 0x73BC, 0x4601, 0x89, 0xF3, 0xCD, 0xEE, 0xEE, 0xE3, 0x21, 0xA1);
/* Plan 9 */
DEFINE_GUID(GPT_PLAN9_PARTITION, 0xC91818F9L, 0x8025, 0x47AF, 0x89, 0xD2, 0xF0, 0x30, 0xD7, 0x00, 0x0C, 0x2C);
/* VMware */
DEFINE_GUID(GPT_VMWARE_COREDUMP_PARTITION, 0x9D275380L, 0x40AD, 0x11DB, 0xBF, 0x97, 0x00, 0x0C, 0x29, 0x11, 0xD1, 0xB8);
DEFINE_GUID(GPT_VMWARE_VMS_FILESYSTEM, 0xAA31E02AL, 0x400F, 0x11DB, 0x95, 0x90, 0x00, 0x0C, 0x29, 0x11, 0xD1, 0xB8);
DEFINE_GUID(GPT_VMWARE_RESERVED_PARTITION, 0x9198EFFCL, 0x31C0, 0x11DB, 0x8F, 0x78, 0x00, 0x0C, 0x29, 0x11, 0xD1, 0xB8);
/* Android-IA */
DEFINE_GUID(GPT_ANDROID_IA_BOOTLOADER, 0x2568845DL, 0x2332, 0x4675, 0xBC, 0x39, 0x8F, 0xA5, 0xA4, 0x74, 0x8D, 0x15);
DEFINE_GUID(GPT_ANDROID_IA_BOOTLOADER_2, 0x114EAFFEL, 0x1552, 0x4022, 0xB2, 0x6E, 0x9B, 0x05, 0x36, 0x04, 0xCF, 0x84);
DEFINE_GUID(GPT_ANDROID_IA_BOOT, 0x49A4D17FL, 0x93A3, 0x45C1, 0xA0, 0xDE, 0xF5, 0x0B, 0x2E, 0xBE, 0x25, 0x99);
DEFINE_GUID(GPT_ANDROID_IA_RECOVERY, 0x4177C722L, 0x9E92, 0x4AAB, 0x86, 0x44, 0x43, 0x50, 0x2B, 0xFD, 0x55, 0x06);
DEFINE_GUID(GPT_ANDROID_IA_MISC_PARTITION, 0xEF32A33BL, 0xA409, 0x486C, 0x91, 0x41, 0x9F, 0xFB, 0x71, 0x1F, 0x62, 0x66);
DEFINE_GUID(GPT_ANDROID_IA_METADATA_PARTITION, 0x20AC26BEL, 0x20B7, 0x11E3, 0x84, 0xC5, 0x6C, 0xFD, 0xB9, 0x47, 0x11, 0xE9);
DEFINE_GUID(GPT_ANDROID_IA_SYSTEM_PARTITION, 0x38F428E6L, 0xD326, 0x425D, 0x91, 0x40, 0x6E, 0x0E, 0xA1, 0x33, 0x64, 0x7C);
DEFINE_GUID(GPT_ANDROID_IA_CACHE_PARTITION, 0xA893EF21L, 0xE428, 0x470A, 0x9E, 0x55, 0x06, 0x68, 0xFD, 0x91, 0xA2, 0xD9);
DEFINE_GUID(GPT_ANDROID_IA_DATA_PARTITION, 0xDC76DDA9L, 0x5AC1, 0x491C, 0xAF, 0x42, 0xA8, 0x25, 0x91, 0x58, 0x0C, 0x0D);
DEFINE_GUID(GPT_ANDROID_IA_PERSISTENT_PARTITION, 0xEBC597D0L, 0x2053, 0x4B15, 0x8B, 0x64, 0xE0, 0xAA, 0xC7, 0x5F, 0x4D, 0xB1);
DEFINE_GUID(GPT_ANDROID_IA_FACTORY_PARTITION, 0x8F68CC74L, 0xC5E5, 0x48DA, 0xBE, 0x91, 0xA0, 0xC8, 0xC1, 0x5E, 0x9C, 0x80);
DEFINE_GUID(GPT_ANDROID_IA_FASTBOOT_PARTITION, 0x767941D0L, 0x2085, 0x11E3, 0xAD, 0x3B, 0x6C, 0xFD, 0xB9, 0x47, 0x11, 0xE9);
DEFINE_GUID(GPT_ANDROID_IA_OEM_PARTITION, 0xAC6D7924L, 0xEB71, 0x4DF8, 0xB4, 0x8D, 0xE2, 0x67, 0xB2, 0x71, 0x48, 0xFF);
/* Open Network Install Environment */
DEFINE_GUID(GPT_ONIE_BOOT_PARTITION, 0x7412F7D5L, 0xA156, 0x4B13, 0x81, 0xDC, 0x86, 0x71, 0x74, 0x92, 0x93, 0x25);
DEFINE_GUID(GPT_ONIE_CONFIG_PARTITION, 0xD4E6E2CDL, 0x4469, 0x46F3, 0xB5, 0xCB, 0x1B, 0xFF, 0x57, 0xAF, 0xC1, 0x49);
/* PowerPC */
DEFINE_GUID(GPT_POWERPC_PREP_BOOT_PARTITION, 0x9E1A2D38L, 0xC612, 0x4316, 0xAA, 0x26, 0x8B, 0x49, 0x52, 0x1E, 0x5A, 0x8B);
/* Freedesktop */
DEFINE_GUID(GPT_FREEDESKTOP_EXT_BOOT_PARTITION, 0xBC13C2FFL, 0x59E6, 0x4262, 0xA3, 0x52, 0xB2, 0x75, 0xFD, 0x6F, 0x71, 0x72);

struct GPTPartitionType {
  GUID const *guid;
  char const *desc;
};

struct GPTPartitionType const gpt_part_type [] = {
    {&PARTITION_ENTRY_UNUSED_GUID,          "Unused entry"}
   ,{&GPT_MBR_PARTITION_SCHEME,             "MBR partition scheme"}
   ,{&PARTITION_SYSTEM_GUID,                "EFI System partition"}
   ,{&GPT_BIOS_BOOT_PARTITION,              "BIOS Boot partition"}
   ,{&GPT_INTEL_FAST_FLASH,                 "Intel Fast Flash(iFFS) for Intel RST"}
   ,{&GPT_SONY_BOOT_PARTITION,              "Sony boot partition"}
   ,{&GPT_LENOVO_BOOT_PARTITION,            "Lenovo boot partition"}
   /* Windows */
   ,{&PARTITION_MSFT_RESERVED_GUID,         "Microsoft Reserved Partition (MSR)"}
   ,{&PARTITION_BASIC_DATA_GUID,            "Windows Basic data partition"}
   ,{&PARTITION_LDM_METADATA_GUID,          "Windows Logical Disk Manager (LDM) metadata"}
   ,{&PARTITION_LDM_DATA_GUID,              "Windows Logical Disk Manager data partition"}
   ,{&PARTITION_MSFT_RECOVERY_GUID,         "Windows Recovery Environment"}
   ,{&PARTITION_CLUSTER_GUID,               "Windows Cluster metadata partition"}
   ,{&PARTITION_MSFT_SNAPSHOT_GUID,         "Microsoft shadow copy partition"}
   ,{&PARTITION_PATCH_GUID,                 "Microsoft patch partition"}
   ,{&GPT_IBM_GENERAL_PARALLEL_FS,          "IBM General Parallel File System (GPFS) partition"}
   ,{&PARTITION_SPACES_GUID,                "Windows Storage Spaces protective partition"}
   /* HP-UX */
   ,{&GPT_HPUX_DATA_PARTITION,              "HP-UX Data partition"}
   ,{&GPT_HPUX_SERVICE_PARTITION,           "HP-UX Service Partition"}
   /* Linux */
   ,{&GPT_LINUX_FILESYSTEM_DATA,            "Linux filesystem data"}
   ,{&GPT_LINUX_RAID_PARTITION,             "Linux RAID partition"}
   ,{&GPT_LINUX_ROOT_PARTITION_X86,         "Linux Root partition (x86)"}
   ,{&GPT_LINUX_ROOT_PARTITION_X64,         "Linux Root partition (x86-64)"}
   ,{&GPT_LINUX_ROOT_PARTITION_ARM32,       "Linux Root partition (32-bit ARM)"}
   ,{&GPT_LINUX_ROOT_PARTITION_ARM64,       "Linux Root partition (64-bit ARM/AArch64)"}
   ,{&GPT_LINUX_SWAP_PARTITION,             "Linux Swap partition"}
   ,{&GPT_LINUX_LVM_PARTITION,              "Linux Logical Volume Manager (LVM) partition"}
   ,{&GPT_LINUX_HOME_PARTITION,             "Linux /home partition"}
   ,{&GPT_LINUX_SRV_PARTITION,              "Linux /srv (server data) partition"}
   ,{&GPT_LINUX_DMCRYPT_PARTITION,          "Linux Plain dm-crypt partition"}
   ,{&GPT_LINUX_LUKS_PARTITION,             "Linux LUKS partition"}
   ,{&GPT_LINUX_RESERVED_PARTITION,         "Linux Reserved"}
   /* FreeBSD */
   ,{&GPT_FREEBSD_BOOT_PARTITION,           "FreeBSD Boot partition"}
   ,{&GPT_FREEBSD_DATA_PARTITION,           "FreeBSD Data partition"}
   ,{&GPT_FREEBSD_SWAP_PARTITION,           "FreeBSD Swap partition"}
   ,{&GPT_FREEBSD_UFS_PARTITION,            "FreeBSD Unix File System (UFS) partition"}
   ,{&GPT_FREEBSD_VINUM_VOLUME_MGR,         "FreeBSD Vinum volume manager partition"}
   ,{&GPT_FREEBSD_ZFS_PARTITION,            "FreeBSD ZFS partition"}
   /* OS X */
   ,{&GPT_OSX_HFSPLUS_PARTITION,            "OS X HFS+ partition"}
   ,{&GPT_OSX_APPLE_UFS_PARTITION,          "OS X Apple UFS"}
   ,{&GPT_OSX_ZFS_PARTITION,                "OS X ZFS"}
   ,{&GPT_OSX_APPLE_RAID_PARTITION,         "OS X Apple RAID partition"}
   ,{&GPT_OSX_APPLE_RAID_OFFLINE,           "OS X Apple RAID partition (offline)"}
   ,{&GPT_OSX_RECOVERY_HD_BOOT,             "OS X Apple Boot partition (Recovery HD)"}
   ,{&GPT_OSX_APPLE_LABEL,                  "OS X Apple Label"}
   ,{&GPT_APPLETV_RECOVERY_PARTITION,       "OS X Apple TV Recovery partition"}
   ,{&GPT_APPLE_CORE_STORAGE,               "OS X Apple Core Storage (Lion FileVault)"}
   /* Solaris / illumos */
   ,{&GPT_SOLARIS_BOOT_PARTITION,           "Solaris/illumos Boot partition"}
   ,{&GPT_SOLARIS_ROOT_PARTITION,           "Solaris/illumos Root partition"}
   ,{&GPT_SOLARIS_SWAP_PARTITION,           "Solaris/illumos Swap partition"}
   ,{&GPT_SOLARIS_BACKUP_PARTITION,         "Solaris/illumos Backup partition"}
   ,{&GPT_SOLARIS_USR_PARTITION,            "Solaris/illumos /usr partition"}
   ,{&GPT_SOLARIS_VAR_PARTITION,            "Solaris/illumos /var partition"}
   ,{&GPT_SOLARIS_HOME_PARTITION,           "Solaris/illumos /home partition"}
   ,{&GPT_SOLARIS_ALTERNATE_SECTOR,         "Solaris/illumos Alternate sector"}
   ,{&GPT_SOLARIS_RESERVED_PARTITION_1,     "Solaris/illumos Reserved partition"}
   ,{&GPT_SOLARIS_RESERVED_PARTITION_2,     "Solaris/illumos Reserved partition"}
   ,{&GPT_SOLARIS_RESERVED_PARTITION_3,     "Solaris/illumos Reserved partition"}
   ,{&GPT_SOLARIS_RESERVED_PARTITION_4,     "Solaris/illumos Reserved partition"}
   ,{&GPT_SOLARIS_RESERVED_PARTITION_5,     "Solaris/illumos Reserved partition"}
   /* NetBSD */
   ,{&GPT_NETBSD_SWAP_PARTITION,            "NetBSD Swap partition"}
   ,{&GPT_NETBSD_FFS_PARTITION,             "NetBSD Fast File System (FFS) partition"}
   ,{&GPT_NETBSD_LFS_PARTITION,             "NetBSD log-structured file system (LFS)"}
   ,{&GPT_NETBSD_RAID_PARTITION,            "NetBSD RAID partition"}
   ,{&GPT_NETBSD_CONCATENATED_PARTITION,    "NetBSD Concatenated partition"}
   ,{&GPT_NETBSD_ENCRYPTED_PARTITION,       "NetBSD Encrypted partition"}
   /* ChromeOS */
   ,{&GPT_CHROMEOS_KERNEL_PARTITION,        "ChromeOS kernel partition"}
   ,{&GPT_CHROMEOS_ROOTFS_PARTITION,        "ChromeOS rootfs partition"}
   ,{&GPT_CHROMEOS_FUTURE_USE,              "ChromeOS future use (reserved)"}
   /* BeOS/Haiku */
   ,{&GPT_HAIKU_BFS_PARTITION,              "Haiku Be File System (BFS)"}
   /* MidnightBSD */
   ,{&GPT_MIDNIGHTBSD_BOOT_PARTITION,       "MidnightBSD Boot partition"}
   ,{&GPT_MIDNIGHTBSD_DATA_PARTITION,       "MidnightBSD Data partition"}
   ,{&GPT_MIDNIGHTBSD_SWAP_PARTITION,       "MidnightBSD Swap partition"}
   ,{&GPT_MIDNIGHTBSD_UFS_PARTITION,        "MidnightBSD Unix File System (UFS)"}
   ,{&GPT_MIDNIGHTBSD_VINUM_VOLUME_MGR,     "MidnightBSD Vinum volume manager partition"}
   ,{&GPT_MIDNIGHTBSD_ZFS_PARTITION,        "MidnightBSD ZFS partition"}
   /* Ceph */
   ,{&GPT_CEPH_JOURNAL_PARTITION,           "Ceph Journal partition"}
   ,{&GPT_CEPH_DMCRYPT_ENCRYPTED_JOURNAL,   "Ceph dm-crypt Encrypted Journal"}
   ,{&GPT_CEPH_OBJECT_STORAGE_DAEMON,       "Ceph object storage daemon (OSD)"}
   ,{&GPT_CEPH_ENCRYPTED_OSD_PARTITION,     "Ceph dm-crypt object storage daemon (OSD)"}
   ,{&GPT_CEPH_DISK_IN_CREATION,            "Ceph disk in creation"}
   ,{&GPT_CEPH_DMCRYPT_DISK_IN_CREATION,    "Ceph dm-crypt disk in creation"}
   /* OpenBSD */
   ,{&GPT_OPENBSD_DATA_PARTITION,           "OpenBSD Data partition"}
   /* QNX */
   ,{&GPT_QNX_POWER_SAFE_FILESYSTEM,        "QNX6 Power-safe file system"}
   /* Plan 9 */
   ,{&GPT_PLAN9_PARTITION,                  "Plan 9 partition"}
   /* VMware */
   ,{&GPT_VMWARE_COREDUMP_PARTITION,        "VMware ESX vmkcore (coredump partition)"}
   ,{&GPT_VMWARE_VMS_FILESYSTEM,            "VMware ESX VMFS filesystem partition"}
   ,{&GPT_VMWARE_RESERVED_PARTITION,        "VMware ESX VMware Reserved"}
   /* Android-IA */
   ,{&GPT_ANDROID_IA_BOOTLOADER,            "Android-IA Bootloader"}
   ,{&GPT_ANDROID_IA_BOOTLOADER_2,          "Android-IA Bootloader2"}
   ,{&GPT_ANDROID_IA_BOOT,                  "Android-IA Boot"}
   ,{&GPT_ANDROID_IA_RECOVERY,              "Android-IA Recovery"}
   ,{&GPT_ANDROID_IA_MISC_PARTITION,        "Android-IA Misc"}
   ,{&GPT_ANDROID_IA_METADATA_PARTITION,    "Android-IA Metadata"}
   ,{&GPT_ANDROID_IA_SYSTEM_PARTITION,      "Android-IA System"}
   ,{&GPT_ANDROID_IA_CACHE_PARTITION,       "Android-IA Cache"}
   ,{&GPT_ANDROID_IA_DATA_PARTITION,        "Android-IA Data"}
   ,{&GPT_ANDROID_IA_PERSISTENT_PARTITION,  "Android-IA Persistent"}
   ,{&GPT_ANDROID_IA_FACTORY_PARTITION,     "Android-IA Factory"}
   ,{&GPT_ANDROID_IA_FASTBOOT_PARTITION,    "Android-IA Fastboot/Tertiary"}
   ,{&GPT_ANDROID_IA_OEM_PARTITION,         "Android-IA OEM Partition"}
   /* Open Network Install Environment */
   ,{&GPT_ONIE_BOOT_PARTITION,              "Open Network Install Environment (ONIE) Boot"}
   ,{&GPT_ONIE_CONFIG_PARTITION,            "Open Network Install Environment (ONIE) Config"}
   /* PowerPC */
   ,{&GPT_POWERPC_PREP_BOOT_PARTITION,      "PowerPC Reference Platform (PReP) Boot"}
   /* Freedesktop */
   ,{&GPT_FREEDESKTOP_EXT_BOOT_PARTITION,   "Freedesktop Extended Boot Partition ($BOOT)"}
  };

extern char const *gpt_lookup_partguid (GUID const *partition_guid)
{ size_t
    i;

  for (i=0; i < _countof(gpt_part_type); ++i)
    if (IsEqualGUID (gpt_part_type [i].guid, partition_guid))
      return (gpt_part_type[i].desc);

  return (NULL);
}
