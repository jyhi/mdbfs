/**
 * @file mdbfs.c
 *
 * Implementation of public interfaces exposed by the Berkeley DB backend for
 * MDBFS.
 */

#include <db.h>
#include "utils/memory.h"
#include "utils/print.h"
#include "backend.h"
#include "dbmgr.h"
#include "fuseops.h"

/********** Non-Exported Public APIs **********/

static const char const *mdbfs_backend_name = "berkeleydb";
static const char const *mdbfs_backend_description = "backend for reading Berkeley DB files";
static const char const *mdbfs_backend_help = NULL;
static const char const *mdbfs_backend_version = "0.1.0";

static const char *mdbfs_backend_berkeleydb_get_name(void)
{
  return mdbfs_backend_name;
}

static const char *mdbfs_backend_berkeleydb_get_description(void)
{
  return mdbfs_backend_description;
}

static const char *mdbfs_backend_berkeleydb_get_help(void)
{
  return mdbfs_backend_help;
}

static const char *mdbfs_backend_berkeleydb_get_version(void)
{
  return mdbfs_backend_version;
}

static int mdbfs_backend_berkeleydb_init(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  /* No-op */

  return 1;
}

static void mdbfs_backend_berkeleydb_deinit(void)
{
  /* No-op */
}

static struct fuse_operations mdbfs_backend_berkeleydb_get_fuse_operations(void)
{
  struct mdbfs_backend_berkeleydb_operations ops = mdbfs_backend_berkeleydb_get_operations();

  return (struct fuse_operations) {
    .getattr         = ops.getattr,
    .readlink        = NULL,
    .mknod           = ops.mknod,
    .mkdir           = NULL,
    .unlink          = ops.unlink,
    .rmdir           = NULL,
    .symlink         = NULL,
    .rename          = ops.rename,
    .link            = NULL,
    .chmod           = NULL,
    .chown           = NULL,
    .truncate        = NULL,
    .open            = NULL,
    .read            = ops.read,
    .write           = ops.write,
    .statfs          = NULL,
    .flush           = NULL,
    .release         = NULL,
    .fsync           = NULL,
    .setxattr        = NULL,
    .getxattr        = NULL,
    .listxattr       = NULL,
    .removexattr     = NULL,
    .opendir         = NULL,
    .readdir         = ops.readdir,
    .releasedir      = NULL,
    .fsyncdir        = NULL,
    .init            = ops.init,
    .destroy         = ops.destroy,
    .access          = NULL,
    .create          = NULL,
    .lock            = NULL,
    .utimens         = NULL,
    .bmap            = NULL,
    .ioctl           = NULL,
    .poll            = NULL,
    .write_buf       = NULL,
    .read_buf        = NULL,
    .flock           = NULL,
    .fallocate       = NULL,
    .copy_file_range = NULL,
  };
}

/********** Public APIs **********/

struct mdbfs_backend *mdbfs_backend_berkeleydb_get_mdbfs_backend(void)
{
  struct mdbfs_backend *ret = mdbfs_malloc0(sizeof(struct mdbfs_backend));

  ret->get_name            = mdbfs_backend_berkeleydb_get_name;
  ret->get_description     = mdbfs_backend_berkeleydb_get_description;
  ret->get_help            = mdbfs_backend_berkeleydb_get_help;
  ret->get_version         = mdbfs_backend_berkeleydb_get_version;
  ret->init                = mdbfs_backend_berkeleydb_init;
  ret->deinit              = mdbfs_backend_berkeleydb_deinit;
  ret->get_fuse_operations = mdbfs_backend_berkeleydb_get_fuse_operations;

  /* The following two are from dbmgr */
  ret->open  = mdbfs_backend_berkeleydb_open_database_from_file;
  ret->close = mdbfs_backend_berkeleydb_close_database;

  return ret;
}
