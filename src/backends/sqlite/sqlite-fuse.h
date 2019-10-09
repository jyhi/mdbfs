/**
 * @file sqlite-fuse.h
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

#ifndef MDBFS_BACKENDS_SQLITE_SQLITE_FUSE_H
#define MDBFS_BACKENDS_SQLITE_SQLITE_FUSE_H

#include "mdbfs-config.h"
#include <fuse.h>

/**
 * FUSE operations implemented by the MDBFS SQLite backend.
 */
struct mdbfs_backend_sqlite_operations {
  /* FS operations */
  void *(*init)    (struct fuse_conn_info *conn, struct fuse_config *cfg);
  void  (*destroy) (void *private_data);

  /* File Object Manipulation */
  int (*mknod)  (const char *, mode_t, dev_t);
  int (*rename) (const char *, const char *, unsigned int);
  int (*unlink) (const char *);
  int (*mkdir)  (const char *, mode_t);
  int (*rmdir)  (const char *);

  /* I/O */
  int (*read)    (const char *, char *, size_t, off_t, struct fuse_file_info *);
  int (*write)   (const char *, const char *, size_t, off_t, struct fuse_file_info *);
  int (*opendir) (const char *, struct fuse_file_info *);
  int (*readdir) (const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *, enum fuse_readdir_flags);

  /* Metadata */
  int (*getattr) (const char *, struct stat *, struct fuse_file_info *);
};

/**
 * Retrieve a bunch of functions that the backend implemented and are necessary
 * to map a SQLite database into a file system.
 *
 * @return A struct mdbfs_backend_sqlite_operations containing FUSE
 *         implementations.
 */
struct mdbfs_backend_sqlite_operations mdbfs_backend_sqlite_get_operations(void);

#endif
