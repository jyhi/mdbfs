/**
 * @file mdbfs-operations.h
 *
 * Generic database backend data structures and functions.
 */

#ifndef MDBFS_OPERATIONS_H
#define MDBFS_OPERATIONS_H

#define FUSE_USE_VERSION 32

#include <fuse.h>

/**
 * The file system operations, which basically form a subset of
 * `struct fuse_operations`.
 *
 * These functions may or may not be implemented by database backends, but
 * it is generally advised to implement all of them. These functions will later
 * be registered to FUSE, whom will take the responsibilty of calling them in
 * appropriate time. See the LibFUSE documentation for details on semantics of
 * these functions.
 */
struct mdbfs_operations {
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

  /* Link */
  int (*symlink)  (const char *, const char *);
  int (*readlink) (const char *, char *, size_t);

  /* Load database */
  void *(*load) (const char const *path);
};

#endif /* MDBFS_OPERATIONS_H */
