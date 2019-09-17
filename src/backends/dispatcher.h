/**
 * @file dispatcher.h
 *
 * Database backend related common interfaces.
 */

#ifndef MDBFS_BACKENDS_H
#define MDBFS_BACKENDS_H

#include "include/mdbfs-operations.h"

/**
 * Given an enum MdbfsBackend, return the struct mdbfs_operations of the
 * requested backend.
 *
 * Possible values of backends may vary depending on the compile-time options.
 * The following is a list of all supported backends:
 *
 * - `sqlite`: SQLite3 backend
 *
 * @param backend [in] A string representing the backend to choose.
 * @return A struct mdbfs_operations for the caller to register them in FUSE.
 */
struct mdbfs_operations *mdbfs_backend_get(const char const *backend);

/**
 * Convert a struct mdbfs_operations to struct fuse_operations.
 *
 * Since struct mdbfs_operations is mostly a subset of struct fuse_operations
 * this is done by removing mdbfs-specific functions (e.g. load) and filling in
 * the structure with holes (NULL) in it.
 *
 * @param mdbfs_ops [in] A struct mdbfs_operations.
 * @return A struct fuse_operations filled by mdbfs_ops.
 */
struct fuse_operations mdbfs_operations_map_to_fuse_operations(const struct mdbfs_operations mdbfs_ops);

#endif
