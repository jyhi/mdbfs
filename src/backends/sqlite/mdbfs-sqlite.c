/**
 * @file db_sqlite.c
 *
 * Implementation of the MDBFS SQLite database backend.
 */

#include <signal.h>
#include <sqlite3.h>
#include <mdbfs-operations.h>
#include <configmgr/configmgr.h>
#include <utils/mdbfs-utils.h>

/********** Private APIs **********/

void *init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
  char *path = mdbfs_configmgr_path_load();
  sqlite3 *db = NULL;

  int r = sqlite3_open_v2(path, &db, SQLITE_OPEN_READONLY, NULL);
  if (r != SQLITE_OK) {
    raise(SIGINT);
    return NULL;
  }

  return db;
}

static void destroy(void *private_data)
{
  sqlite3 *db = (sqlite3 *)private_data;

  sqlite3_close(db);
}

/********** Public APIs **********/

struct mdbfs_operations *mdbfs_backend_sqlite_register(void)
{
  struct mdbfs_operations *ret = mdbfs_malloc0(sizeof (struct mdbfs_operations));

  ret->init = init;
  ret->destroy = destroy;

  return ret;
}
