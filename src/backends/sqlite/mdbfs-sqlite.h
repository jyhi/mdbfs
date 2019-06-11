/**
 * @file mdbfs-sqlite.h
 *
 * Public interfaces of the MDBFS SQLite database backend.
 *
 * ## Hierarchy
 *
 * The mapped file system hierarchy is:
 *
 * ```
 * /T/R/C
 * ```
 *
 * where:
 *
 * - `T` is the name of table.
 * - `R` is the index number of the row, starting from 0.
 * - `C` is the name of the column, which is specified in the database schema
 *   when creating the table.
 *
 * `T` and `R` are directories, while `C` is a file. The content of `C` is the
 * value stored in the cell, which is located in <T, R, C> in the original
 * SQLite database management system.
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
