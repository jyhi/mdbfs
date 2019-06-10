/**
 * @file memory.h
 *
 * Public interface of memory related utilities.
 */

#ifndef MDBFS_UTIL_MEMORY_H
#define MDBFS_UTIL_MEMORY_H

#include <stdlib.h>

/**
 * Allocate a memory region with the given size.
 *
 * Upon failure, this function will properly handle errors, exit the driver, and
 * will not return. Thus, extra error handling is not needed.
 *
 * @param size [in] Size of the memory to allocate.
 * @return A pointer pointing to the allocated memory region.
 */
void *mdbfs_malloc(size_t size);

/**
 * Same as mdbfs_malloc, with additional zero-out.
 *
 * @param size [in] Size of the memory to allocate.
 * @return A pointer pointing to the zeroed allocated memory region.
 */
void *mdbfs_malloc0(size_t size);

/**
 * Free the memeory region pointed by ptr, then set ptr to NULL.
 *
 * @param ptr [in,out] A pointer pointing to a pointer pointing to the memory
 *                     region to be free'd.
 */
#define mdbfs_free(ptr) { if (ptr) free(ptr), ptr = NULL; }

#endif
