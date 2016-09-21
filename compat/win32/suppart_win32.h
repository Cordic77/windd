#ifndef WINDD_SUPPART_H_
#define WINDD_SUPPART_H_

/* */
extern bool
  MBRTypeRecognizedByWindows (void const *part);
extern bool
  GPTGuidRecognizedByWindows (void const *part);

/* */
extern char const *
  mbr_lookup_parttype (unsigned char const partition_type);
extern char const *
  gpt_lookup_partguid (GUID const *partition_guid);

#endif
