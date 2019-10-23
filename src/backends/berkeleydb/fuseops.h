/**
 * @file fuseops.h
 *
 * Public interfaces of the MDBFS Berkeley DB database backend.
 *
 * ## Hierarchy
 *
 * The mapped file system hierarchy is:
 *
 * ```
 * /R
 * ```
 *
 * where:
 *
 * - `R` is the name of records (keys).
 *
 * There are only files, no directories, since Berkeley DB is a key-value
 * database.
 */

#ifndef MDBFS_BACKENDS_BERKELEYDB_FUSEOPS_H
#define MDBFS_BACKENDS_BERKELEYDB_FUSEOPS_H

#include "mdbfs-config.h"
#include <fuse.h>

/**
 * FUSE operations implemented by the MDBFS Berkeley DB backend.
 */
struct mdbfs_backend_berkeleydb_operations {
  /* FS operations */
  void *(*init)    (struct fuse_conn_info *conn, struct fuse_config *cfg);
  void  (*destroy) (void *private_data);

  /* File Object Manipulation */
  int (*mknod)  (const char *, mode_t, dev_t);
  int (*rename) (const char *, const char *, unsigned int);
  int (*unlink) (const char *);

  /* I/O */
  int (*read)    (const char *, char *, size_t, off_t, struct fuse_file_info *);
  int (*write)   (const char *, const char *, size_t, off_t, struct fuse_file_info *);
  int (*readdir) (const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *, enum fuse_readdir_flags);

  /* Metadata */
  int (*getattr) (const char *, struct stat *, struct fuse_file_info *);
};

/**
 * Retrieve a bunch of functions that the backend implemented and are necessary
 * to map a Berkeley DB database into a file system.
 *
 * @return A struct mdbfs_backend_berkeleydb_operations containing FUSE
 *         implementations.
 */
struct mdbfs_backend_berkeleydb_operations mdbfs_backend_berkeleydb_get_operations(void);

#endif
