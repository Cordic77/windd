#include <config.h>


/* See `ntdddisk.h': */
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
