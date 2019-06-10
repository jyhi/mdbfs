/**
 * @file mdbfs-sqlite.h
 *
 * Public interfaces of the MDBFS SQLite database backend.
 */

#ifndef MDBFS_SQLITE_H
#define MDBFS_SQLITE_H

/**
 * Retrieve a bunch of functions that are necessary to map a SQLite database
 * into a file system.
 *
 * The caller can then use mdbfs_operations_map_to_fuse_operations to convert
 * it into a FUSE version, which in turn can be fed into FUSE to run a real
 * file system.
 *
 * @return A struct mdbfs_operations containing FUSE implementations.
 */
struct mdbfs_operations *mdbfs_backend_sqlite_register(void);

#endif
