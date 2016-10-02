#include <config.h>


/* */
void *(MemAlloc) (size_t elements, size_t size, char const *file, int line)
{
  void *ptr = malloc (elements * size);

  if (ptr == NULL)
  {
    fprintf (stderr, "\n[%s] %s:%" PRIu32 " MemAlloc (%" PRIuMAX ",%" PRIuMAX ") failed:\n%s\n",
                        PROGRAM_NAME, file, (uint32_t)line,
                        (uintmax_t)elements, (uintmax_t)size,
                        strerror (errno));
    exit (253);
  }

  return (ptr);
}


void *(MemRealloc) (void *ptr, size_t elements, size_t size, char const *file, int line)
{
  ptr = realloc (ptr, elements * size);

  if (ptr == NULL)
  {
    fprintf (stderr, "\n[%s] %s:%" PRIu32 " MemRealloc (%" PRIuMAX ",%" PRIuMAX ") failed:\n%s\n",
                        PROGRAM_NAME, file, (uint32_t)line,
                        (uintmax_t)elements, (uintmax_t)size,
                        strerror (errno));
    exit (253);
  }

  return (ptr);
}


void (Free) (void **ptr)
{
  if (ptr!=NULL && *ptr!=NULL)
  {
    free (*ptr);
    *ptr = NULL;
  }
}
