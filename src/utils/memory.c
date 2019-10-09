/**
 * @file memory.c
 *
 * Implementation of memory related utilities.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <mdbfs-config.h>

static const char const *msg_alloc_error =
  "** " PROJECT_NAME ": memory allocation failed\n";

void *mdbfs_malloc(size_t size)
{
  void *ret = malloc(size);
  if (!ret) {
    fputs(msg_alloc_error, stderr);
    abort();
  }

  return ret;
}

void *mdbfs_malloc0(size_t size)
{
  return memset(mdbfs_malloc(size), 0, size);
}

void *mdbfs_realloc(void *ptr, size_t size)
{
  void *ret = realloc(ptr, size);
  if (!ret) {
    fputs(msg_alloc_error, stderr);

    /* "The original pointer ptr remains valid and may need to be deallocated
     * with free() or realloc()."
     * -- https://en.cppreference.com/w/c/memory/realloc
     */
    free(ptr);

    abort();
  }
}
