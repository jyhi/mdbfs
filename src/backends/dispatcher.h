/**
 * @file dispatcher.h
 *
 * Database backend related common interfaces.
 */

#ifndef MDBFS_BACKENDS_H
#define MDBFS_BACKENDS_H

#include "include/mdbfs-operations.h"

/**
 * Backends supported.
 */
enum MdbfsBackend {
  MDBFS_BACKEND_UNKNOWN = 0, /**< Reserved for unknown status. */
  MDBFS_BACKEND_SQLITE,      /**< SQLite backend. */
};

/**
 * Given an enum MdbfsBackend, return the struct mdbfs_operations of the
 * requested backend.
 *
 * @param backend [in] An enum MdbfsBackend, specifying the backend to choose.
 * @return A struct mdbfs_operations for the caller to register them in FUSE.
 */
struct mdbfs_operations *mdbfs_backend_get(const enum MdbfsBackend backend);

/**
 * Convert a string representation to the corresponding backend enumeration.
 *
 * @param backend [in] Name of the backend.
 * @return enum MdbfsBackend indicating the backend.
 */
enum MdbfsBackend mdbfs_backend_str_to_enum(const char * const backend);

/**
 * Convert a struct mdbfs_operations to struct fuse_operations.
 *
 * Since struct mdbfs_operations is basically a subset of struct fuse_operations
 * this is done by filling in the structure with holes (NULL) in it.
 *
 * @param mdbfs_ops [in] A struct mdbfs_operations.
 * @return A struct fuse_operations filled by mdbfs_ops.
 */
struct fuse_operations mdbfs_operations_map_to_fuse_operations(const struct mdbfs_operations mdbfs_ops);

#endif
