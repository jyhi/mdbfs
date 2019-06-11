/**
 * @file dispatcher.c
 *
 * Implementation of database backend related common interfaces.
 */

#include <string.h>
#include "include/mdbfs-operations.h"
#include "sqlite/mdbfs-sqlite.h"
#include "dispatcher.h"

struct mdbfs_operations *mdbfs_backend_get(const enum MdbfsBackend backend)
{
  switch (backend) {
    case MDBFS_BACKEND_SQLITE:
      return mdbfs_backend_sqlite_register();
    case MDBFS_BACKEND_UNKNOWN: // fall through
    default:
      return NULL;
  }
}

enum MdbfsBackend mdbfs_backend_str_to_enum(const char * const backend)
{
  enum MdbfsBackend ret = MDBFS_BACKEND_UNKNOWN;

  if (!backend)
    return ret;

  if (strcmp("sqlite", backend) == 0) {
    ret = MDBFS_BACKEND_SQLITE;
  } else {}

  return ret;
}

struct fuse_operations mdbfs_operations_map_to_fuse_operations(const struct mdbfs_operations mdbfs_ops)
{
  return (struct fuse_operations){
    .getattr = mdbfs_ops.getattr,
    .readlink = mdbfs_ops.readlink,
    .mknod = mdbfs_ops.mknod,
    .mkdir = mdbfs_ops.mkdir,
    .unlink = mdbfs_ops.unlink,
    .rmdir = mdbfs_ops.rmdir,
    .symlink = mdbfs_ops.symlink,
    .rename = mdbfs_ops.rename,
    .link = NULL,
    .chmod = NULL,
    .chown = NULL,
    .truncate = NULL,
    .open = mdbfs_ops.open,
    .read = mdbfs_ops.read,
    .write = mdbfs_ops.write,
    .statfs = NULL,
    .flush = NULL,
    .release = NULL,
    .fsync = NULL,
    .setxattr = NULL,
    .getxattr = NULL,
    .listxattr = NULL,
    .removexattr = NULL,
    .opendir = mdbfs_ops.opendir,
    .readdir = mdbfs_ops.readdir,
    .releasedir = NULL,
    .fsyncdir = NULL,
    .init = mdbfs_ops.init,
    .destroy = mdbfs_ops.destroy,
    .access = NULL,
    .create = mdbfs_ops.create,
    .lock = NULL,
    .utimens = NULL,
    .bmap = NULL,
    .ioctl = NULL,
    .poll = NULL,
    .write_buf = NULL,
    .read_buf = NULL,
    .flock = NULL,
    .fallocate = NULL,
    .copy_file_range = NULL,
  };
}
