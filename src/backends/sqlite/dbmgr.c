/**
 * @file dbmgr.c
 *
 * Implementation for public interfaces of the SQLite database manager for MDBFS
 * SQLite backend.
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sqlite3.h>
#include "utils/memory.h"
#include "utils/path.h"
#include "utils/print.h"
#include "dbmgr.h"

/********** Private SQL Statement Strings **********/

static const char const *sql_str_get_tables =
  "SELECT \"name\" FROM \"sqlite_master\" WHERE \"type\" = \"table\"";

static const char const *sql_fmt_select_from =
  "SELECT \"%s\" FROM \"%s\"";

static const char const *sql_fmt_select_all_from_where =
  "SELECT * FROM \"%s\" WHERE \"%s\" = \"%s\"";

static const char const *sql_fmt_select_from_where =
  "SELECT \"%s\" FROM \"%s\" WHERE \"%s\" = \"%s\"";

static const char const *sql_fmt_alter_table_add_column =
  "ALTER TABLE \"%s\" ADD COLUMN \"%s\"";

static const char const *sql_fmt_alter_table_rename_to =
  "ALTER TABLE \"%s\" RENAME TO \"%s\"";

static const char const *sql_fmt_alter_table_rename_column_to =
  "ALTER TABLE \"%s\" RENAME COLUMN \"%s\" TO \"%s\"";

static const char const *sql_fmt_update_set_where =
  "UPDATE \"%s\" SET \"%s\" = \"%s\" WHERE \"%s\" = \"%s\"";

static const char const *sql_fmt_drop_table =
  "DROP TABLE \"%s\"";

static const char const *sql_fmt_delete_from_where =
  "DELETE FROM \"%s\" WHERE \"%s\" = \"%s\"";

/********** Private States **********/

static sqlite3 *g_db = NULL;

/********** Private APIs **********/

static char *sql_from_fmt(const char *fmt, ...)
{
  va_list args_len;
  va_list args_str;
  char *sql = NULL;
  int r = 0;

  va_start(args_len, fmt);
  va_copy(args_str, args_len);

  /* "If the resulting string gets truncated due to buf_size limit, function
   * returns the total number of characters (not including the terminating
   * null-byte) which would have been written, if the limit was not imposed."
   *
   * -- https://en.cppreference.com/w/c/io/vfprintf
   */
  r = vsnprintf(NULL, 0, fmt, args_len);
  if (r < 0) {
    mdbfs_error("sqlite: sql_from_fmt: vsnprintf returned unexpected error %d while calculating length", r);
    goto end;
  }

  /* r was the output length */
  sql = mdbfs_malloc0(r + 1); /* + 1 NUL */
  r = vsnprintf(sql, r + 1, fmt, args_str);
  if (r < 0) {
    mdbfs_error("sqlite: sql_from_fmt: vsnprintf returned unexpected error %d while printing sql", r);
    mdbfs_free(sql);
    goto end;
  }

end:
  mdbfs_debug("sqlite: sql_from_fmt: %s", sql);

  va_end(args_len);
  va_end(args_str);
  return sql;
}

/********** Public APIs **********/

int mdbfs_backend_sqlite_open_database_from_file(const char *path)
{
  if (!path) {
    mdbfs_warning("sqlite: open: path is missing");
    return 0;
  }

  if (g_db) {
    mdbfs_warning("sqlite: open: it looks like a database is already loaded!");
    mdbfs_warning("sqlite: open: dropping the (previous?) session");
    g_db = NULL;
  }

  mdbfs_info("sqlite: opening database from %s", path);

  int r = sqlite3_open_v2(path, &g_db, SQLITE_OPEN_READWRITE, NULL);
  if (r != SQLITE_OK) {
    mdbfs_error(
      "unable to open SQLite3 database at %s: %s",
      path, sqlite3_errmsg(g_db)
    );
    return 0;
  }

  return 1;
}

void mdbfs_backend_sqlite_close_database(void)
{
  if (!g_db) {
    mdbfs_warning("sqlite: close: attempting to close a closed connection!");
    return;
  }

  mdbfs_info("closing sqlite3 database");
  sqlite3_close(g_db);
  g_db = NULL;
}

char *mdbfs_backend_sqlite_get_database_name(void)
{
  char *ret = mdbfs_malloc0(strlen("main") + 1);
  strcpy(ret, "main");
  return ret;
}

char **mdbfs_backend_sqlite_get_table_names(void)
{
  sqlite3_stmt *stmt = NULL;
  char **ret = NULL;
  size_t ret_length = 0;
  int r = 0;

  mdbfs_debug("sqlite: listing table names");

  r = sqlite3_prepare_v2(g_db, sql_str_get_tables, -1, &stmt, NULL);
  if (r != SQLITE_OK) {
    mdbfs_error("sqlite: get_table_names: sqlite3 cannot prepare a sql statement for us: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  for (;;) {
    r = sqlite3_step(stmt);
    if (r != SQLITE_ROW)
      break;

    const char *table_name = sqlite3_column_text(stmt, 0);
    if (!table_name) {
      mdbfs_warning("sqlite: get_table_names: unexpected null");
      continue;
    }

    mdbfs_debug("sqlite: get_table_names: .. %s", table_name);

    /* Stretch vector */
    ret_length += 1;
    ret = mdbfs_realloc(ret, ret_length * sizeof(char *));

    /* Fill string element */
    size_t name_length = strlen(table_name) + 1;
    ret[ret_length - 1] = mdbfs_malloc0(name_length);
    strncpy(ret[ret_length - 1], table_name, name_length);
  }

  if (r != SQLITE_DONE) {
    mdbfs_warning("sqlite: get_table_names: sqlite3 reported an error: %s", sqlite3_errmsg(g_db));
    for (int i = 0; i < ret_length; i++) {
      mdbfs_free(ret[i]);
    }
    mdbfs_free(ret);
    sqlite3_reset(stmt);
    return NULL;
  }

  /* Additionally add a NULL at the end of list for iteration */
  ret_length += 1;
  ret = mdbfs_realloc(ret, ret_length * sizeof(char *));
  ret[ret_length - 1] = NULL;

  mdbfs_debug("sqlite: get_table_names: done listing table names");

quit:
  r = sqlite3_finalize(stmt);
  if (r != SQLITE_OK) {
    mdbfs_warning("sqlite: get_table_names: sqlite3 cannot finalize a sql statement for us: %s", sqlite3_errmsg(g_db));
    mdbfs_warning("sqlite: get_table_names: *leaking memory*");
  }
  return ret;
}

char **mdbfs_backend_sqlite_get_column_names(const char *table_name, const char *row_name)
{
  sqlite3_stmt *stmt = NULL;
  char *sql = NULL;
  char **ret = NULL;
  size_t ret_length = 0;
  int r = 0;

  if (!table_name) {
    mdbfs_warning("sqlite: get_column_names: table name is missing, this is unexpected. returning");
    return NULL;
  }

  mdbfs_debug("sqlite: listing column names in table \"%s\"", table_name);

  sql = sql_from_fmt(sql_fmt_select_all_from_where, table_name, "ROWID", row_name);
  if (!sql) {
    mdbfs_error("sqlite: get_column_names: no sql no life!");
    goto quit;
  }

  r = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
  if (r != SQLITE_OK) {
    mdbfs_error("sqlite: get_column_names: sqlite3 cannot prepare a sql statement for us: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  r = sqlite3_step(stmt);

  if (r == SQLITE_DONE) {
    mdbfs_debug("sqlite: get_column_names: nothing to show, the row may not exist");
    goto quit;
  }

  if (r != SQLITE_ROW) {
    mdbfs_warning("sqlite: get_column_names: sqlite3 reported an error: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  int ncol = sqlite3_column_count(stmt);
  for (int icol = 0; icol < ncol; icol++) {
    const char *column_name = sqlite3_column_origin_name(stmt, icol);
    if (!column_name) {
      mdbfs_warning("sqlite: get_column_names: unexpected null");
      continue;
    }

    /* Stretch vector */
    ret_length += 1;
    ret = mdbfs_realloc(ret, ret_length * sizeof(char *));

    /* Fill string element */
    size_t name_length = strlen(column_name) + 1;
    ret[ret_length - 1] = mdbfs_malloc0(name_length);
    strncpy(ret[ret_length - 1], column_name, name_length);
  }

  /* Additionally add a NULL at the end of list for iteration */
  ret_length += 1;
  ret = mdbfs_realloc(ret, ret_length * sizeof(char *));
  ret[ret_length - 1] = NULL;

  mdbfs_debug("sqlite: done listing column names in table \"%s\"", table_name);

quit:
  mdbfs_free(sql);
  r = sqlite3_finalize(stmt);
  if (r != SQLITE_OK) {
    mdbfs_warning("sqlite: get_column_names: sqlite3 cannot finalize a sql statement for us: %s", sqlite3_errmsg(g_db));
    mdbfs_warning("sqlite: get_column_names: *leaking memory*");
  }
  return ret;
}

char **mdbfs_backend_sqlite_get_row_names(const char *table_name)
{
  sqlite3_stmt *stmt = NULL;
  char *sql = NULL;
  char **ret = NULL;
  size_t ret_length = 0;
  int r = 0;

  if (!table_name) {
    mdbfs_warning("sqlite: get_row_names: table name is missing, this is unexpected. returning");
    return NULL;
  }

  mdbfs_debug("sqlite: listing rows in table \"%s\"", table_name);

  sql = sql_from_fmt(sql_fmt_select_from, "ROWID", table_name);
  if (!sql) {
    mdbfs_error("sqlite: get_row_names: no sql no life!");
    goto quit;
  }

  r = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
  if (r != SQLITE_OK) {
    mdbfs_error("sqlite: get_row_names: sqlite3 cannot prepare a sql statement for us: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  /* Iterate over the result to get a list of rows */
  for (;;) {
    r = sqlite3_step(stmt);
    if (r != SQLITE_ROW)
      break;

    const char *row_name = sqlite3_column_text(stmt, 0);
    if (!row_name) {
      mdbfs_warning("sqlite: get_row_names: unexpected null");
      continue;
    }

    mdbfs_debug("sqlite: get_row_names: .. %s", row_name);

    /* Stretch vector */
    ret_length += 1;
    ret = mdbfs_realloc(ret, ret_length * sizeof(char *));

    /* Fill string element */
    size_t name_length = strlen(row_name) + 1;
    ret[ret_length - 1] = mdbfs_malloc0(name_length);
    strncpy(ret[ret_length - 1], row_name, name_length);
  }

  if (r != SQLITE_DONE) {
    mdbfs_warning("sqlite: get_row_names: sqlite3 reported an error: %s", sqlite3_errmsg(g_db));
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

  mdbfs_debug("sqlite: done listing rows in table \"%s\"", table_name);

quit:
  mdbfs_free(sql);
  r = sqlite3_finalize(stmt);
  if (r != SQLITE_OK) {
    mdbfs_warning("sqlite: get_row_names: sqlite3 cannot finalize a sql statement for us: %s", sqlite3_errmsg(g_db));
    mdbfs_warning("sqlite: get_row_names: *leaking memory*");
  }
  return ret;
}

uint8_t *mdbfs_backend_sqlite_get_cell(size_t *cell_length, const char *table_name, const char *row_name, const char *col_name)
{
  sqlite3_stmt *stmt = NULL;
  char *sql = NULL;
  uint8_t *ret = NULL;
  size_t   ret_length = 0;
  int r = 0;

  if (!table_name || !row_name || !col_name) {
    mdbfs_warning("sqlite: get_cell: either table name, row name, or column name is missing, this is unexpected. returning");
    return NULL;
  }

  mdbfs_debug("sqlite: get_cell: querying content in cell (\"%s\", \"%s\", \"%s\")", table_name, row_name, col_name);

  sql = sql_from_fmt(sql_fmt_select_from_where, col_name, table_name, "ROWID", row_name);
  if (!sql) {
    mdbfs_error("sqlite: get_cell: no sql no life!");
    goto quit;
  }

  r = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
  if (r != SQLITE_OK) {
    mdbfs_error("sqlite: get_cell: sqlite3 cannot prepare a sql statement for us: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  r = sqlite3_step(stmt);

  if (r == SQLITE_DONE) {
    mdbfs_debug("sqlite: get_cell: nothing to show, confused");
    goto quit;
  }

  if (r != SQLITE_ROW) {
    mdbfs_warning("sqlite: get_cell: sqlite3 reported an error: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  const uint8_t *cell = sqlite3_column_text(stmt, 0);
  if (!cell) {
    mdbfs_warning("sqlite: get_cell: unexpected null");
    goto quit;
  }

  /* NOTE: Trick: If we get a string that is identical to the column name, the
   * column does not exist. Jesus.
   */
  if (strcmp(cell, col_name) == 0) {
    mdbfs_debug("sqlite: get_cell: the column does not exist");
    goto quit;
  }

  /* NOTE: If we get an empty string, the cell is empty */
  if (strcmp(cell, "") == 0) {
    mdbfs_debug("sqlite: get_cell: the cell is empty");
  }

  /* NOTE: sqlite3_column_bytes does not include NUL */
  ret_length = sqlite3_column_bytes(stmt, 0);
  ret = mdbfs_malloc0(ret_length + 1); /* + 1 NUL */
  memcpy(ret, cell, ret_length);
  if (cell_length)
    *cell_length = ret_length;

  mdbfs_debug("sqlite: get_cell: done querying content in cell (\"%s\", \"%s\", \"%s\")", table_name, row_name, col_name);

quit:
  mdbfs_free(sql);
  r = sqlite3_finalize(stmt);
  if (r != SQLITE_OK) {
    mdbfs_warning("sqlite: get_cell: sqlite3 cannot finalize a sql statement for us: %s", sqlite3_errmsg(g_db));
    mdbfs_warning("sqlite: get_cell: *leaking memory*");
  }
  return ret;
}

size_t mdbfs_backend_sqlite_get_cell_length(const char *table_name, const char *row_name, const char *col_name)
{
  sqlite3_stmt *stmt = NULL;
  char *sql = NULL;
  size_t ret = 0;
  int r = 0;

  if (!table_name || !row_name || !col_name) {
    mdbfs_warning("sqlite: get_cell_length: either table name, row name, or column name is missing, this is unexpected. returning");
    return 0;
  }

  mdbfs_debug("sqlite: get_cell_length: querying length of cell (\"%s\", \"%s\", \"%s\")", table_name, row_name, col_name);

  sql = sql_from_fmt(sql_fmt_select_from_where, col_name, table_name, "ROWID", row_name);
  if (!sql) {
    mdbfs_error("sqlite: get_cell_length: no sql no life!");
    goto quit;
  }

  r = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
  if (r != SQLITE_OK) {
    mdbfs_error("sqlite: get_cell_length: sqlite3 cannot prepare a sql statement for us: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  r = sqlite3_step(stmt);

  if (r == SQLITE_DONE) {
    mdbfs_debug("sqlite: get_cell_length: nothing to show, confused");
    goto quit;
  }

  if (r != SQLITE_ROW) {
    mdbfs_warning("sqlite: get_cell_length: sqlite3 reported an error: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  /* It's still inevitable to get the content first (see below) */
  const uint8_t *cell = sqlite3_column_text(stmt, 0);
  if (!cell) {
    mdbfs_warning("sqlite: get_cell_length: unexpected null");
    goto quit;
  }

  /* NOTE: Trick: If we get a string that is identical to the column name, the
   * column does not exist. Jesus.
   */
  if (strcmp(cell, col_name) == 0) {
    mdbfs_debug("sqlite: get_cell_length: the column does not exist");
    goto quit;
  }

  /* NOTE: sqlite3_column_bytes does not include NUL, but we include it */
  ret = sqlite3_column_bytes(stmt, 0);

  mdbfs_debug("sqlite: get_cell_length: done querying length of cell (\"%s\", \"%s\", \"%s\")", table_name, row_name, col_name);

quit:
  mdbfs_free(sql);
  r = sqlite3_finalize(stmt);
  if (r != SQLITE_OK) {
    mdbfs_warning("sqlite: get_cell_length: sqlite3 cannot finalize a sql statement for us: %s", sqlite3_errmsg(g_db));
    mdbfs_warning("sqlite: get_cell_length: *leaking memory*");
  }
  return ret;
}

int mdbfs_backend_sqlite_set_cell(const uint8_t *content, const size_t content_length, const char *table_name, const char *row_name, const char *col_name)
{
  sqlite3_stmt *stmt = NULL;
  char *sql = NULL;
  int r = 0;

  if (!table_name || !row_name || !col_name) {
    mdbfs_warning("sqlite: set_cell: either table name, row name, or column name is missing, this is unexpected. returning");
    return 0;
  }

  mdbfs_debug("sqlite: set_cell: updating content in cell (\"%s\", \"%s\", \"%s\")", table_name, row_name, col_name);

  sql = sql_from_fmt(sql_fmt_update_set_where, table_name, col_name, content, "ROWID", row_name);
  if (!sql) {
    mdbfs_error("sqlite: set_cell: no sql no life!");
    goto quit;
  }

  r = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
  if (r != SQLITE_OK) {
    mdbfs_error("sqlite: set_cell: sqlite3 cannot prepare a sql statement for us: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  r = sqlite3_step(stmt);

  if (r != SQLITE_DONE) {
    mdbfs_warning("sqlite: set_cell: sqlite3 reported an error: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  mdbfs_debug("sqlite: set_cell: done updating content in cell (\"%s\", \"%s\", \"%s\")", table_name, row_name, col_name);

quit:
  mdbfs_free(sql);
  r = sqlite3_finalize(stmt);
  if (r != SQLITE_OK) {
    mdbfs_warning("sqlite: set_cell: sqlite3 cannot finalize a sql statement for us: %s", sqlite3_errmsg(g_db));
    mdbfs_warning("sqlite: set_cell: *leaking memory*");
  }
  return 1;
}

int mdbfs_backend_sqlite_rename_table(const char *table_old, const char *table_new)
{
  sqlite3_stmt *stmt = NULL;
  char *sql = NULL;
  int r = 0;

  if (!table_old || !table_new) {
    mdbfs_warning("sqlite: rename_table: either old or new table name is empty, this is unexpected. returning");
    return 0;
  }

  mdbfs_debug("sqlite: rename_table: altering table name from %s to %s", table_old, table_new);

  sql = sql_from_fmt(sql_fmt_alter_table_rename_to, table_old, table_new);
  if (!sql) {
    mdbfs_error("sqlite: rename_table: no sql no life!");
    goto quit;
  }

  r = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
  if (r != SQLITE_OK) {
    mdbfs_error("sqlite: rename_table: sqlite3 cannot prepare a sql statement for us: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  r = sqlite3_step(stmt);

  /* No result is given */
  if (r != SQLITE_DONE) {
    mdbfs_warning("sqlite: rename_table: sqlite3 reported an error: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  mdbfs_debug("sqlite: rename_table: done altering table name from %s to %s", table_old, table_new);

quit:
  mdbfs_free(sql);
  r = sqlite3_finalize(stmt);
  if (r != SQLITE_OK) {
    mdbfs_warning("sqlite: rename_table: sqlite3 cannot finalize a sql statement for us: %s", sqlite3_errmsg(g_db));
    mdbfs_warning("sqlite: rename_table: *leaking memory*");
  }
  return 1;
}

int mdbfs_backend_sqlite_rename_column(const char *table_name, const char *column_old, const char *column_new)
{
  sqlite3_stmt *stmt = NULL;
  char *sql = NULL;
  int r = 0;

  if (!table_name || !column_old || !column_new) {
    mdbfs_warning("sqlite: rename_column: either table name, old column name, or new column name is missing, this is unexpected. returning");
    return 0;
  }

  mdbfs_debug("sqlite: rename_column: altering column name in table \"%s\" from \"%s\" to \"%s\"", table_name, column_old, column_new);

  sql = sql_from_fmt(sql_fmt_alter_table_rename_column_to, table_name, column_old, column_new);
  if (!sql) {
    mdbfs_error("sqlite: rename_column: no sql no life!");
    goto quit;
  }

  r = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
  if (r != SQLITE_OK) {
    mdbfs_error("sqlite: rename_column: sqlite3 cannot prepare a sql statement for us: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  r = sqlite3_step(stmt);

  /* No result is given */
  if (r != SQLITE_DONE) {
    mdbfs_warning("sqlite: rename_column: sqlite3 reported an error: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  mdbfs_debug("sqlite: rename_column: done altering column name in table \"%s\" from \"%s\" to \"%s\"", table_name, column_old, column_new);

quit:
  mdbfs_free(sql);
  r = sqlite3_finalize(stmt);
  if (r != SQLITE_OK) {
    mdbfs_warning("sqlite: rename_column: sqlite3 cannot finalize a sql statement for us: %s", sqlite3_errmsg(g_db));
    mdbfs_warning("sqlite: rename_column: *leaking memory*");
  }
  return 1;
}

int mdbfs_backend_sqlite_rename_row(const char *table_name, const char *row_old, const char *row_new)
{
  sqlite3_stmt *stmt = NULL;
  char *sql = NULL;
  int r = 0;

  if (!table_name || !row_old || !row_new) {
    mdbfs_warning("sqlite: rename_row: either table name, old row name, or new row name is missing, this is unexpected. returning");
    return 0;
  }

  mdbfs_debug("sqlite: rename_row: altering row name in table \"%s\" from \"%s\" to \"%s\"", table_name, row_old, row_new);

  sql = sql_from_fmt(sql_fmt_update_set_where, table_name, row_old, row_new);
  if (!sql) {
    mdbfs_error("sqlite: rename_row: no sql no life!");
    goto quit;
  }

  r = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
  if (r != SQLITE_OK) {
    mdbfs_error("sqlite: rename_row: sqlite3 cannot prepare a sql statement for us: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  r = sqlite3_step(stmt);

  /* No result is given */
  if (r != SQLITE_DONE) {
    mdbfs_warning("sqlite: rename_row: sqlite3 reported an error: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  mdbfs_debug("sqlite: rename_row: altered row name in table \"%s\" from \"%s\" to \"%s\"", table_name, row_old, row_new);

quit:
  mdbfs_free(sql);
  r = sqlite3_finalize(stmt);
  if (r != SQLITE_OK) {
    mdbfs_warning("sqlite: rename_row: sqlite3 cannot finalize a sql statement for us: %s", sqlite3_errmsg(g_db));
    mdbfs_warning("sqlite: rename_row: *leaking memory*");
  }
  return 1;
}

int mdbfs_backend_sqlite_create_table(const char *table_new)
{
  (void)table_new;

  mdbfs_info("sqlite: create_table: not implemented");

  return 0;
}

int mdbfs_backend_sqlite_create_column(const char *table_name, const char *column_new)
{
  sqlite3_stmt *stmt = NULL;
  char *sql = NULL;
  int r = 0;

  if (!table_name || !column_new) {
    mdbfs_warning("sqlite: create_column: either table name or new column name is missing, this is unexpected. returning");
    return 0;
  }

  mdbfs_debug("sqlite: create_column: creating column \"%s\" in table \"%s\"", column_new, table_name);

  sql = sql_from_fmt(sql_fmt_alter_table_add_column, table_name, column_new);
  if (!sql) {
    mdbfs_error("sqlite: create_column: no sql no life!");
    goto quit;
  }

  r = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
  if (r != SQLITE_OK) {
    mdbfs_error("sqlite: create_column: sqlite3 cannot prepare a sql statement for us: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  r = sqlite3_step(stmt);

  /* No result is given */
  if (r != SQLITE_DONE) {
    mdbfs_warning("sqlite: create_column: sqlite3 reported an error: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  mdbfs_debug("sqlite: create_column: done creating column \"%s\" in table \"%s\"", column_new, table_name);

quit:
  mdbfs_free(sql);
  r = sqlite3_finalize(stmt);
  if (r != SQLITE_OK) {
    mdbfs_warning("sqlite: create_column: sqlite3 cannot finalize a sql statement for us: %s", sqlite3_errmsg(g_db));
    mdbfs_warning("sqlite: create_column: *leaking memory*");
  }
  return 1;
}

int mdbfs_backend_sqlite_create_row(const char *table_name, const char *row_new)
{
  (void)table_name;
  (void)row_new;

  mdbfs_info("sqlite: create_row: not implemented");

  return 0;
}

int mdbfs_backend_sqlite_remove_table(const char *table_name)
{
  sqlite3_stmt *stmt = NULL;
  char *sql = NULL;
  int r = 0;

  if (!table_name) {
    mdbfs_warning("sqlite: remove_table: table name is missing, this is unexpected. returning");
    return 0;
  }

  mdbfs_debug("sqlite: remove_table: dropping table \"%s\"", table_name);

  sql = sql_from_fmt(sql_fmt_drop_table, table_name);
  if (!sql) {
    mdbfs_error("sqlite: remove_table: no sql no life!");
    goto quit;
  }

  r = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
  if (r != SQLITE_OK) {
    mdbfs_error("sqlite: remove_table: sqlite3 cannot prepare a sql statement for us: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  r = sqlite3_step(stmt);

  /* No result is given */
  if (r != SQLITE_DONE) {
    mdbfs_warning("sqlite: remove_table: sqlite3 reported an error: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  mdbfs_debug("sqlite: remove_table: dropped table \"%s\"", table_name);

quit:
  mdbfs_free(sql);
  r = sqlite3_finalize(stmt);
  if (r != SQLITE_OK) {
    mdbfs_warning("sqlite: remove_table: sqlite3 cannot finalize a sql statement for us: %s", sqlite3_errmsg(g_db));
    mdbfs_warning("sqlite: remove_table: *leaking memory*");
  }
  return 1;
}

int mdbfs_backend_sqlite_remove_column(const char *table_name, const char *column_name)
{
  (void)table_name;
  (void)column_name;

  mdbfs_info("sqlite: remove_column: not implemented");

  return 0;
}

int mdbfs_backend_sqlite_remove_row(const char *table_name, const char *row_name)
{
  sqlite3_stmt *stmt = NULL;
  char *sql = NULL;
  int r = 0;

  if (!table_name || !row_name) {
    mdbfs_warning("sqlite: remove_row: either table name or row name is missing, this is unexpected. returning");
    return 0;
  }

  mdbfs_debug("sqlite: remove_row: deleting row \"%s\" in table \"%s\"", row_name, table_name);

  sql = sql_from_fmt(sql_fmt_delete_from_where, table_name, "ROWID", row_name);
  if (!sql) {
    mdbfs_error("sqlite: remove_row: no sql no life!");
    goto quit;
  }

  r = sqlite3_prepare_v2(g_db, sql, -1, &stmt, NULL);
  if (r != SQLITE_OK) {
    mdbfs_error("sqlite: remove_row: sqlite3 cannot prepare a sql statement for us: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  r = sqlite3_step(stmt);

  /* No result is given */
  if (r != SQLITE_DONE) {
    mdbfs_warning("sqlite: remove_row: sqlite3 reported an error: %s", sqlite3_errmsg(g_db));
    goto quit;
  }

  mdbfs_debug("sqlite: remove_row: deleted row \"%s\" in table \"%s\"", row_name, table_name);

quit:
  mdbfs_free(sql);
  r = sqlite3_finalize(stmt);
  if (r != SQLITE_OK) {
    mdbfs_warning("sqlite: remove_row: sqlite3 cannot finalize a sql statement for us: %s", sqlite3_errmsg(g_db));
    mdbfs_warning("sqlite: remove_row: *leaking memory*");
  }
  return 1;
}
