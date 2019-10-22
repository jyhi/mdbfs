/**
 * @file dbmgr.c
 *
 * Implementation for public interfaces of the Berkeley DB database manager for
 * MDBFS Berkeley DB backend.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <db.h>
#include "utils/memory.h"
#include "utils/path.h"
#include "utils/print.h"
#include "dbmgr.h"

/********** Private States **********/

static DB *g_db = NULL;

/********** Private APIs **********/

int mdbfs_backend_berkeleydb_open_database_from_file(const char *path)
{
  int r = 0;

  if (!path) {
    mdbfs_warning("berkeleydb: open: path is missing");
    return 0;
  }

  if (g_db) {
    mdbfs_warning("berkeleydb: open: it looks like a database is already loaded!");
    mdbfs_warning("berkeleydb: open: dropping the (previous?) session");
    mdbfs_backend_berkeleydb_close_database();
  }

  mdbfs_info("berkeleydb: opening database from %s", path);

  r = db_create(&g_db, NULL, 0);
  if (r != 0) {
    mdbfs_error("berkeleydb: open: %s", db_strerror(r));
    return 0;
  }

  r = g_db->open(g_db, NULL, path, NULL, DB_UNKNOWN, 0, 0);
  if (r != 0) {
    mdbfs_error("berkeleydb: open: cannot open the database: %s", db_strerror(r));
    mdbfs_backend_berkeleydb_close_database();
    return 0;
  }

  return 1;
}

void mdbfs_backend_berkeleydb_close_database(void)
{
  if (!g_db) {
    mdbfs_error("berkeleydb: close: attempting to perform close on an invalid handle!");
    return;
  }

  mdbfs_info("closing berkeley db database");

  int r = g_db->close(g_db, 0);
  if (r != 0) {
    mdbfs_warning("berkeleydb: close: %s", db_strerror(r));
    mdbfs_warning("berkeleydb: close: closing anyway");
  }

  g_db = NULL;
}

char *mdbfs_backend_berkeleydb_get_database_name(void)
{
  const char *db_name = NULL;
  size_t db_name_length = 0;
  char *ret = NULL;

  int r = g_db->get_dbname(g_db, NULL, &db_name);
  if (r != 0) {
    mdbfs_error("berkeleydb: get_database_name: %s", db_strerror(r));
    return NULL;
  }

  // Transfer ownership
  db_name_length = strlen(db_name) + 1; /* + 1 NUL */
  ret = mdbfs_malloc0(db_name_length);
  memcpy(ret, db_name, db_name_length);

  return ret;
}

char **mdbfs_backend_berkeleydb_get_record_keys(void)
{
  DBC *cursor = NULL;
  DBT key = {0};
  DBT value = {0};
  char **ret = 0;
  size_t ret_length = 0;
  int r = 0;

  r = g_db->cursor(g_db, NULL, &cursor, 0);
  if (r != 0) {
    mdbfs_error("berkeleydb: get_record_keys: %s", db_strerror(r));
    return NULL;
  }

  mdbfs_debug("iterating the whole database");

  for (;;) {
    /* Clear DBTs */
    memset(&key, 0, sizeof(DBT));
    memset(&value, 0, sizeof(DBT));

    r = cursor->get(cursor, &key, &value, DB_NEXT);
    if (r != 0)
      break;

    const char  *k    = (const char *)key.data;
    const size_t klen = key.size;

    /* Note that the returned data may not terminate with NUL */
    mdbfs_debug(".. %.*s", klen, k);

    /* Stretch vector */
    ret_length += 1;
    ret = mdbfs_realloc(ret, ret_length * sizeof(char *));

    /* Fill string element */
    ret[ret_length - 1] = mdbfs_malloc0(klen + 1);
    memcpy(ret[ret_length - 1], k, klen);
  }

  /* "The DBcursor->get() method will return DB_NOTFOUND if DB_NEXT is set and
   * the cursor isalready on the last record in the database."
   */
  if (r != DB_NOTFOUND) {
    mdbfs_error("berkeleydb: get_record_keys: error during iteration: %s", db_strerror(r));
    for (int i = 0; i < ret_length; i++) {
      mdbfs_free(ret[i]);
    }
    mdbfs_free(ret);
    goto quit;
  }

  /* Additionally add a NULL at the end of list for iteration */
  ret_length += 1;
  ret = mdbfs_realloc(ret, ret_length * sizeof(char *));
  ret[ret_length - 1] = NULL;

  mdbfs_debug("done iterating the whole database");

quit:
  r = cursor->close(cursor);
  if (r != 0) {
    mdbfs_warning("berkeleydb: get_record_keys: cannot close cursor: %s", db_strerror(r));
    mdbfs_warning("berkeleydb: get_record_keys: *leaking memory*");
  }

  return ret;
}

uint8_t *mdbfs_backend_berkeleydb_get_record_value(size_t *value_length, const char *key)
{
  DBT dbt_key = {0};
  DBT dbt_value = {0};
  uint8_t *ret = 0;
  size_t ret_length = 0;
  int r = 0;

  mdbfs_debug("berkeleydb: get_record_value: querying database");

  /* Prepare DBTs */
  dbt_key.data = (void *)key;
  dbt_key.size = strlen(key);
  dbt_key.flags = DB_DBT_READONLY;

  /* We tell Berkeley DB to allocate the memory for us so that the ownership of
   * memory can be moved instead of being copied.
   */
  dbt_value.flags = DB_DBT_MALLOC;

  r = g_db->get(g_db, NULL, &dbt_key, &dbt_value, 0);
  if (r != 0) {
    mdbfs_error("berkeleydb: get_record_value: %s", db_strerror(r));
    return NULL;
  }

  /* Is this move semantics? */
  ret = (uint8_t *)dbt_value.data;
  *value_length = dbt_value.size;

  mdbfs_debug("berkeleydb: get_record_value: done querying database");

  return ret;
}

int mdbfs_backend_berkeleydb_set_record_value(const char *key, const uint8_t *value, const size_t value_length)
{
  DBT dbt_key = {0};
  DBT dbt_value = {0};
  int r = 0;

  mdbfs_debug("berkeleydb: set_record_value: setting new %s", key);

  /* Prepare DBTs */
  dbt_key.data = (void *)key;
  dbt_key.size = strlen(key);
  dbt_key.flags = DB_DBT_READONLY;
  dbt_value.data = (void *)value;
  dbt_value.size = value_length;
  dbt_value.flags = DB_DBT_READONLY;

  r = g_db->put(g_db, NULL, &dbt_key, &dbt_value, 0);
  if (r != 0) {
    mdbfs_error("berkeleydb: set_record_value: %s", db_strerror(r));
    return 0;
  }

  mdbfs_debug("berkeleydb: set_record_value: done setting new %s", key);

  return 1;
}

int mdbfs_backend_berkeleydb_rename_record(const char *key_old, const char *key_new)
{
  DBT dbt_key_old = {0};
  DBT dbt_key_new = {0};
  DBT dbt_value = {0};
  int ret = 0;
  int r = 0;

  mdbfs_debug("berkeleydb: rename_record: renaming %s to %s", key_old, key_new);

  /* Prepare DBTs */
  dbt_key_old.data = (void *)key_old;
  dbt_key_old.size = strlen(key_old);
  dbt_key_old.flags = DB_DBT_READONLY;

  dbt_key_new.data = (void *)key_new;
  dbt_key_new.size = strlen(key_new);
  dbt_key_new.flags = DB_DBT_READONLY;

  dbt_value.flags = DB_DBT_MALLOC;

  /* Get the value first */
  r = g_db->get(g_db, NULL, &dbt_key_old, &dbt_value, 0);
  if (r != 0) {
    mdbfs_error("berkeleydb: rename_record: failed to get the old record: %s", db_strerror(r));
    ret = 0;
    goto quit;
  }

  /* Remove it */
  r = g_db->del(g_db, NULL, &dbt_key_old, 0);
  if (r != 0) {
    mdbfs_error("berkeleydb: rename_record: failed to delete the old record: %s", db_strerror(r));
    ret = 0;
    goto quit;
  }

  /* Then put it back using the new key */
  r = g_db->put(g_db, NULL, &dbt_key_new, &dbt_value, 0);
  if (r != 0) {
    mdbfs_error("berkeleydb: rename_record: failed to set the new record: %s", db_strerror(r));
    ret = 0;
    goto quit;
  }

  /* Done */
  ret = 1;

  mdbfs_debug("berkeleydb: rename_record: renamed %s to %s", key_old, key_new);

quit:
  mdbfs_free(dbt_value.data);
  return ret;
}

int mdbfs_backend_berkeleydb_create_record(const char *key_new)
{
  DBT dbt_key = {0};
  DBT dbt_value = {0};
  int r = 0;

  mdbfs_debug("berkeleydb: create_record: creating (empty) record %s", key_new);

  /* Prepare DBTs */
  dbt_key.data = (void *)key_new;
  dbt_key.size = strlen(key_new);
  dbt_key.flags = DB_DBT_READONLY;

  dbt_value.flags = DB_DBT_READONLY;

  r = g_db->put(g_db, NULL, &dbt_key, &dbt_value, 0);
  if (r != 0) {
    mdbfs_error("berkeleydb: create_record: %s", db_strerror(r));
    return 0;
  }

  mdbfs_debug("berkeleydb: create_record: created (empty) record %s", key_new);

  return 1;
}

int mdbfs_backend_berkeleydb_remove_record(const char *key)
{
  DBT dbt_key = {0};
  int r = 0;

  mdbfs_debug("berkeleydb: remove_record: removing record %s", key);

  /* Prepare DBT */
  dbt_key.data = (void *)key;
  dbt_key.size = strlen(key);
  dbt_key.flags = DB_DBT_READONLY;

  r = g_db->del(g_db, NULL, &dbt_key, 0);
  if (r != 0) {
    mdbfs_error("berkeleydb: remove_record: %s", db_strerror(r));
    return 0;
  }

  mdbfs_debug("berkeleydb: remove_record: removed record %s", key);

  return 1;
}
