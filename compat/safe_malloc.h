#ifndef WINDD_MALLOC_H_
#define WINDD_MALLOC_H_

#include <crtdefs.h>              /* size_t */

/* */
#define MemAlloc(elements, size) (MemAlloc) (elements, size, __FILE__, __LINE__)
#define MemRealloc(ptr, elements, size) (MemRealloc) (ptr, elements, size, __FILE__, __LINE__)
#define Free(memblock) (Free) ((void **)&memblock)

/* */
extern void *
 (MemAlloc) (size_t elements, size_t size, char const *file, int line);
extern void *
 (MemRealloc) (void *ptr, size_t elements, size_t size, char const *file, int line);
extern void
 (Free) (void **ptr);

#endif
