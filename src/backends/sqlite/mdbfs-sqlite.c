/**
 * @file db_sqlite.c
 *
 * Implementation of the MDBFS SQLite database backend.
 */

#include <assert.h>
#include <signal.h>
#include <sqlite3.h>
#include <mdbfs-operations.h>
#include <configmgr/configmgr.h>
#include <utils/mdbfs-utils.h>

/********** Private States **********/

/**
 * The SQLite database handle
 */
static sqlite3 *_db = NULL;

/********** Private APIs **********/

static void destroy(void *private_data)
{
  assert(private_data);

  sqlite3 *db = (sqlite3 *)private_data;
  assert(db == _db);

  mdbfs_debug("closing database");

  sqlite3_close(db);
  _db = NULL;
}

static void *load(const char * const path)
{
  sqlite3 *db = NULL;

  int r = sqlite3_open_v2(path, &db, SQLITE_OPEN_READONLY, NULL);
  if (r != SQLITE_OK) {
    mdbfs_error("unable to open SQLite database at %s: %s", path, sqlite3_errstr(r));
    return NULL;
  }

  /* Save it as a local state */
  _db = db;

  /* Is this polymorphism? */
  return (void *)db;
}

/********** Public APIs **********/

struct mdbfs_operations *mdbfs_backend_sqlite_register(void)
{
  struct mdbfs_operations *ret = mdbfs_malloc0(sizeof (struct mdbfs_operations));

  ret->destroy = destroy;
  ret->load = load;

  return ret;
}
