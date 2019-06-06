/**
 * @file mdbfs-sqlite.h
 *
 * Public interface of the MDBFS SQLite database backend.
 */

#include <db.h>

/**
 * Register this database backend to the MDBFS driver.
 *
 * The `struct mdbfs_operations` contains implementations of file system
 * operations corresponding to the database that the backend is operating on.
 *
 * @return `struct mdbfs_operations` containing file system operation
 *         implementations. Upon failure, NULL should be returned.
 */
struct mdbfs_operations *mdbfs_register(void);
