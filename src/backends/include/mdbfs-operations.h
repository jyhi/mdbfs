/**
 * @file db.h
 *
 * Generic database backend data structures and functions.
 */

#ifndef MDBFS_DB_H
#define MDBFS_DB_H

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
  int   (*statfs)  (const char *, struct statvfs *);

  /* File Object Manipulation */
  int (*create) (const char *, mode_t, struct fuse_file_info *);
  int (*mknod)  (const char *, mode_t, dev_t);
  int (*unlink) (const char *);
  int (*mkdir)  (const char *, mode_t);
  int (*rmdir)  (const char *);

  /* I/O */
  int (*open)    (const char *, struct fuse_file_info *);
  int (*read)    (const char *, char *, size_t, off_t, struct fuse_file_info *);
  int (*write)   (const char *, const char *, size_t, off_t, struct fuse_file_info *);
  int (*opendir) (const char *, struct fuse_file_info *);
  int (*readdir) (const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *, enum fuse_readdir_flags);

  /* Link */
  int (*symlink)  (const char *, const char *);
  int (*readlink) (const char *, char *, size_t);
};

#endif /* MDBFS_DB_H */
