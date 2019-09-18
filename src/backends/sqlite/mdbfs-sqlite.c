/**
 * @file mdbfs-sqlite.c
 *
 * Implementation of the MDBFS SQLite database backend.
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <sqlite3.h>
#include <mdbfs-operations.h>
#include <utils/mdbfs-utils.h>

static const char const *sql_get_tables =
  "SELECT name FROM 'sqlite_master' WHERE type = 'table'";

static const char const *fmt_sql_select_from =
  "SELECT %s FROM '%s'";

static const char const *fmt_sql_select_from_where =
  "SELECT %s FROM '%s' WHERE %s = '%s'";

static const char const *fmt_sql_update_set_where =
  "UPDATE '%s' SET '%s' = '%s' WHERE %s = '%s'";

/********** Private States **********/

/**
 * The SQLite database handle.
 */
static sqlite3 *_db = NULL;

/********** Private APIs **********/

/**
 * Verify if a path represents a file or not, according to the mapping
 * specification.
 *
 * @param path [in] The path to examine.
 * @return true if the path represents a file, false if not.
 */
static bool is_path_file(const char const *path)
{
  /* Normalize the path first to get rid of unusual paths */
  char *normal_path = mdbfs_path_lexically_normal(path);
  assert(normal_path);

  const char *pstr = normal_path;
  int counter = 0;

  while (*pstr) {
    if (*pstr == '/')
      ++counter;

    ++pstr;
  }

  /* A file should exactly be at `/T/R/C` */
  if (counter == 3)
    return true;

  return false;
}

/**
 * From a path string, extract the 3 components (/table/row/column).
 *
 * The three output arguments are copied out, so the caller is responsible for
 * freeing these memories using mdbfs_free.
 *
 * @param table  [out] The "table" element in the path (1st, directory).
 * @param row    [out] The "row" element in the path (2nd, directory).
 * @param column [out] The "column" element in the path (3rd, file).
 * @param path   [in]  The path to be extracted.
 * @return 1 if the process succeed, 0 if there is something wrong with the
 *         input.
 */
static int extract_path(char **table, char **row, char **column, const char const *path)
{
  assert(table);
  assert(row);
  assert(column);
  assert(path);

  char *table_ret  = NULL;
  char *row_ret    = NULL;
  char *column_ret = NULL;
  ssize_t table_length  = 0;
  ssize_t row_length    = 0;
  ssize_t column_length = 0;

  /* The path may be in the following correct form:
   *
   * - `/`
   * - `/table` or `/table/`
   * - `/table/row` or `/table/row/`
   * - `/table/row/column` or `/table/row/column/`
   *
   * Ill-formed paths don't theoretically appear, but should be considered too.
   */

  char *normal_path = mdbfs_path_lexically_normal(path);
  assert(normal_path);
  mdbfs_debug("normal_path: %s", normal_path);

  if (!mdbfs_path_is_absolute(normal_path)) {
    mdbfs_warning("extract_path: not an absolute path.");
    return 0;
  }

  /* Again */
  assert(normal_path[0] == '/');

  /* Special case: root */
  if (strcmp("/", normal_path) == 0)
    goto finish;

  /* /path/to/cell
   *  ^   ^
   *  |   +-- ptr_end
   *  +-- ptr_start
   */
  const char *ptr_start = normal_path + 1;
  const char *ptr_end   = ptr_start;

  /* table */
  while (*ptr_end && *ptr_end != '/')
    ++ptr_end;

  table_length = ptr_end - ptr_start;
  table_ret = mdbfs_malloc0(table_length + 1);
  memcpy(table_ret, ptr_start, table_length);

  if (!*ptr_end)
    goto finish;

  ptr_start = ptr_end + 1;
  ptr_end = ptr_start;

  /* row */
  while (*ptr_end && *ptr_end != '/')
    ++ptr_end;

  row_length = ptr_end - ptr_start;
  row_ret = mdbfs_malloc0(row_length + 1);
  memcpy(row_ret, ptr_start, row_length);

  if (!*ptr_end)
    goto finish;

  ptr_start = ptr_end + 1;
  ptr_end = ptr_start;

  /* column */
  while (*ptr_end && *ptr_end != '/')
    ++ptr_end;

  column_length = ptr_end - ptr_start;
  column_ret = mdbfs_malloc0(column_length + 1);
  memcpy(column_ret, ptr_start, column_length);

  if (!*ptr_end)
    goto finish;

  ptr_start = ptr_end + 1;
  ptr_end = ptr_start;

  /* If there are more characters afterwards, the path is ill-formed */
  mdbfs_warning("ill-formed path: more than 3 components in the path, this is not allowed.");
  mdbfs_free(column_ret);
  mdbfs_free(row_ret);
  mdbfs_free(table_ret);
  mdbfs_free(normal_path);
  return 0;

  /* Finishing */
finish:
  *table  = table_ret;
  *row    = row_ret;
  *column = column_ret;

  mdbfs_debug("extract_path: /%s/%s/%s", table_ret, row_ret, column_ret);

  /* Free unused memory */
  mdbfs_free(normal_path);

  return 1;
}

/**
 * Check if the given path is valid in the file system, according to the mapping
 * scheme.
 *
 * @param path [in] The path to be checked.
 * @return true if the path is valid, or false if the path is invalid, and
 *         should not (will not) exist in the file system.
 */
static bool path_exists(const char const *path)
{
  char *table  = NULL;
  char *row    = NULL;
  char *column = NULL;
  sqlite3_stmt *stmt = NULL;
  bool retval = 0; /* Value to be returned by this function */
  int  r      = 0; /* Used to receive return values of calling functions */

  r = extract_path(&table, &row, &column, path);
  if (!r) {
    retval = false;
    goto quit;
  }

  mdbfs_debug("%s -> /%s/%s/%s", path, table, row, column);

  if (!table && !row && !column) {

    /* root always exists */
    retval = true;
    goto quit;

  } else if (table && !row && !column) {

    /* Prepare the SQL query
     * We check if the table is in sqlite_master.
     */
    r = sqlite3_prepare_v2(_db, sql_get_tables, -1, &stmt, NULL);
    if (r != SQLITE_OK) {
      mdbfs_error("path_exists: sqlite3 cannot prepare a SQL statement for us: %s", sqlite3_errmsg(_db));
      retval = -ENOENT;
      goto quit_free;
    }

    for (;;) {
      r = sqlite3_step(stmt);
      if (r != SQLITE_ROW)
        break;

      const char const *table_name = sqlite3_column_text(stmt, 0);
      if (!table_name) {
        mdbfs_warning("path_exists: sqlite3 returned a NULL for table name, this is not expected");
        continue;
      }

      if (strcmp(path, table_name) == 0) {
        mdbfs_debug("%s exists in db", path);
        retval = true;
        goto quit_finalize;
      }
    }

    /* Anyway. */
    retval = -ENOENT;
    goto quit_finalize;

  } else if (table && row && !column) {

    /* Prepare the SQL query
     * We select the row to see if there is any result
     */
    size_t sql_length = strlen(fmt_sql_select_from_where) + strlen("*") + strlen(table) + strlen("ROWID") + strlen(row);
    char *sql = mdbfs_malloc(sql_length);
    snprintf(sql, sql_length, fmt_sql_select_from_where, "*", table, "ROWID", row);

    mdbfs_debug("To be prepared: %s", sql);

    r = sqlite3_prepare_v2(_db, sql, -1, &stmt, NULL);
    if (r != SQLITE_OK) {
      mdbfs_error("path_exists: sqlite3 cannot prepare a SQL statement for us: %s", sqlite3_errmsg(_db));
      retval = false;
      goto quit_free;
    }

    mdbfs_free(sql);

    /* Execute the SQL query */
    r = sqlite3_step(stmt);
    if (r != SQLITE_ROW) {
      mdbfs_warning("path_exists: sqlite3 execution error: %s", sqlite3_errmsg(_db));
      retval = false;
      goto quit_finalize;
    }

    /* If there is any result, the file exists */
    retval = true;
    mdbfs_debug("%s exists in db", path);

    /* Extra check on more results */
    r = sqlite3_step(stmt);
    if (r != SQLITE_DONE) {
      mdbfs_warning("path_exists: more than 1 result for the query, ignoring...");
    }

  } else if (table && row && column) {

    /* Prepare the SQL query
     * We select the cell to see if there is any result
     */
    size_t sql_length = strlen(fmt_sql_select_from_where) + strlen(column) + strlen(table) + strlen("ROWID") + strlen(row);
    char *sql = mdbfs_malloc(sql_length);
    snprintf(sql, sql_length, fmt_sql_select_from_where, column, table, "ROWID", row);

    mdbfs_debug("To be prepared: %s", sql);

    r = sqlite3_prepare_v2(_db, sql, -1, &stmt, NULL);
    if (r != SQLITE_OK) {
      mdbfs_error("path_exists: sqlite3 cannot prepare a SQL statement for us: %s", sqlite3_errmsg(_db));
      retval = false;
      goto quit_free;
    }

    mdbfs_free(sql);

    /* Execute the SQL query */
    r = sqlite3_step(stmt);
    if (r != SQLITE_ROW) {
      mdbfs_warning("path_exists: sqlite3 execution error: %s", sqlite3_errmsg(_db));
      retval = false;
      goto quit_finalize;
    }

    /* If there is any result, the file exists */
    retval = true;
    mdbfs_debug("%s exists in db", path);

    /* Extra check on more results */
    r = sqlite3_step(stmt);
    if (r != SQLITE_DONE) {
      mdbfs_warning("path_exists: more than 1 result for the query, ignoring...");
    }

  }

quit_finalize:
  /* Destruct statement */
  r = sqlite3_finalize(stmt);
  if (r != SQLITE_OK) {
    mdbfs_warning("path_exists: sqlite3 cannot finalize the SQL statement: %s", sqlite3_errmsg(_db));
    mdbfs_warning("path_exists: dropping the statement object anyway, but memory leak happens...");
  }

quit_free:
  mdbfs_free(column);
  mdbfs_free(row);
  mdbfs_free(table);

quit:
  return retval;
}

/********** Public-But-Not-Exported APIs **********/

/**
 * Open and load the database so that the database is ready for use.
 */
static void *_load(const char const *path)
{
  sqlite3 *db = NULL;

  int r = sqlite3_open_v2(path, &db, SQLITE_OPEN_READWRITE, NULL);
  if (r != SQLITE_OK) {
    mdbfs_error("unable to open SQLite database at %s: %s", path, sqlite3_errmsg(_db));
    return NULL;
  }

  /* Save it as a local state */
  _db = db;

  /* Is this polymorphism? */
  return (void *)db;
}

/**
 * Initialize global (FUSE) configurations of the file system.
 */
static void *_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
  cfg->use_ino = 0;
  cfg->direct_io = 1;

  return NULL;
}

/**
 * Close the file system gracefully.
 */
static void _destroy(void *private_data)
{
  mdbfs_debug("closing database");
  sqlite3_close(_db);
  _db = NULL;
}

static int _create(const char *path, mode_t mode, struct fuse_file_info *fileinfo)
{
  /* "A call to creat() is equivalent to calling open() with flags equal to
   * O_CREAT|O_WRONLY|O_TRUNC."
   * -- Linux Programmer's Manual, open(2)
   */

  /* So far we only implement a read-only file system */
  return -EROFS;
}

static int _rename(const char *path1, const char *path2, unsigned int flags)
{
  /* So far we only implement a read-only file system */
  return -EROFS;
}

/**
 *
 */
static int _unlink(const char *path)
{
  /* So far we only implement a read-only file system */
  return -EROFS;
}

/**
 *
 */
static int _mkdir(const char *path, mode_t mode)
{
  /* So far we only implement a read-only file system */
  return -EROFS;
}

/**
 *
 */
static int _rmdir(const char *path)
{
  /* So far we only implement a read-only file system */
  return -EROFS;
}

/**
 *
 */
static int _read(const char *path, char *buf, size_t bufsize, off_t offset, struct fuse_file_info *fileinfo)
{
  char *table  = NULL;
  char *row    = NULL;
  char *column = NULL;
  sqlite3_stmt *stmt = NULL;
  int retval = 0; /* Value to be returned by this function */
  int r      = 0; /* Used to receive return values of calling functions */

  /* Cell content is returned all at once, no seeking */
  if (offset > 0) {
    return 0;
  }

  r = extract_path(&table, &row, &column, path);
  if (!r) {
    retval = -EINTR;
    goto quit;
  }

  mdbfs_debug("%s -> /%s/%s/%s", path, table, row, column);

  if ((!table && !row && !column) ||
      ( table && !row && !column) ||
      ( table &&  row && !column))
  {
    /* Directory access */
    retval = -EISDIR;
    goto quit_free;
  }

  /* Again */
  assert(table && row && column);

  /* Prepare the SQL query
   * We select the cell to see if there is any result
   */
  size_t sql_length = strlen(fmt_sql_select_from_where) + strlen(column) + strlen(table) + strlen("ROWID") + strlen(row);
  char *sql = mdbfs_malloc(sql_length);
  snprintf(sql, sql_length, fmt_sql_select_from_where, column, table, "ROWID", row);

  mdbfs_debug("To be prepared: %s", sql);

  r = sqlite3_prepare_v2(_db, sql, -1, &stmt, NULL);
  if (r != SQLITE_OK) {
    mdbfs_error("read: sqlite3 cannot prepare a SQL statement for us: %s", sqlite3_errmsg(_db));
    retval = -EINTR;
    goto quit_free;
  }

  mdbfs_free(sql);

  /* Execute the SQL query */
  r = sqlite3_step(stmt);
  if (r != SQLITE_ROW) {
    mdbfs_warning("read: sqlite3 execution error: %s", sqlite3_errmsg(_db));
    retval = -EINTR;
    goto quit_finalize;
  }

  /* There should be only one column in only one row
   * This result may not be free'd
   */
  const char const *cell = sqlite3_column_text(stmt, 0);
  if (!cell) {
    /* The cell is null (no content) */
    mdbfs_info("read: sqlite3 does not give a result, the cell is probably empty.");
  }

  /* Size to return to the caller */
  int cell_bytes = sqlite3_column_bytes(stmt, 0);

  /* If there is any result, the file exists */
  mdbfs_debug("%s exists in db", path);

  /* Return the read result */
  if (cell) {
    memcpy(buf, cell, bufsize);
    retval = cell_bytes <= bufsize ? cell_bytes : bufsize;
  }

  /* Extra check on more results */
  r = sqlite3_step(stmt);
  if (r != SQLITE_DONE) {
    mdbfs_warning("read: more than 1 result for the query, ignoring...");
  }

quit_finalize:
  /* Destruct statement */
  r = sqlite3_finalize(stmt);
  if (r != SQLITE_OK) {
    mdbfs_warning("read: sqlite3 cannot finalize the SQL statement: %s", sqlite3_errmsg(_db));
    mdbfs_warning("read: dropping the statement object anyway, but memory leak happens...");
  }

quit_free:
  mdbfs_free(column);
  mdbfs_free(row);
  mdbfs_free(table);

quit:
  return retval;
}

/**
 * Write content to a file.
 *
 * @param path     [in] Path to the file.
 * @param buf      [in] A buffer containing data to be written.
 * @param bufsize  [in] Size of the buffer `buf`.
 * @param offset   [in] Offset to the file. This is ignored.
 * @param fileinfo [in] Information about the file.
 */
static int _write(const char *path, const char *buf, size_t bufsize, off_t offset, struct fuse_file_info *fileinfo)
{
  char *table  = NULL;
  char *row    = NULL;
  char *column = NULL;
  sqlite3_stmt *stmt = NULL;
  int retval = 0; /* Value to be returned by this function */
  int r      = 0; /* Used to receive return values of calling functions */

  (void) offset;
  (void) fileinfo;

  /* Only files mapping to cells are writable, otherwise illegal */
  if (!is_path_file(path))
    return -ENOSPC; /* XXX: Is this error code correct? */

  r = extract_path(&table, &row, &column, path);
  if (!r) {
    retval = -EINTR;
    goto quit;
  }

  assert(table);
  assert(row);
  assert(column);

  /* Prepare the SQL query
   * We select the cell to see if there is any result
   */
  size_t sql_length = strlen(fmt_sql_update_set_where) + strlen(table) + strlen(column) + bufsize + strlen("ROWID") + strlen(row);
  char *sql = mdbfs_malloc(sql_length);
  snprintf(sql, sql_length, fmt_sql_update_set_where, table, column, buf, "ROWID", row);

  mdbfs_debug("To be prepared: %s", sql);

  r = sqlite3_prepare_v2(_db, sql, -1, &stmt, NULL);
  if (r != SQLITE_OK) {
    mdbfs_error("write: sqlite3 cannot prepare a SQL statement for us: %s", sqlite3_errmsg(_db));
    retval = -EINTR;
    goto quit_free;
  }

  mdbfs_free(sql);

  /* Execute the SQL query */
  r = sqlite3_step(stmt);
  if (r != SQLITE_DONE) {
    mdbfs_warning("write: sqlite3 cannot finish the write request: %s", sqlite3_errmsg(_db));
    retval = -EINTR;
    goto quit_finalize;
  }

  mdbfs_info("write: cell update succeeded");

  /* Bytes written is the length of buffer */
  retval = bufsize;

quit_finalize:
  /* Destruct statement */
  r = sqlite3_finalize(stmt);
  if (r != SQLITE_OK) {
    mdbfs_warning("write: sqlite3 cannot finalize the SQL statement: %s", sqlite3_errmsg(_db));
    mdbfs_warning("write: dropping the statement object anyway, but memory leak happens...");
  }

quit_free:
  mdbfs_free(column);
  mdbfs_free(row);
  mdbfs_free(table);

quit:
  return retval;
}

/**
 * Get file attributes.
 *
 * All files will have the same set of attributes, we don't really care. It's
 * the same as directories.
 *
 * Some of the attributes are set globally in _init (e.g. owners).
 *
 * @param path     [in]  Path to the file.
 * @param stat     [out] The `struct stat` to pass back to the caller.
 * @param fileinfo [in]  Information about an open file.
 * @return 0 if succeed, -errno if fail.
 * @see _init
 */
static int _getattr(const char *path, struct stat *stat, struct fuse_file_info *fileinfo)
{
  /* "The 'st_dev' and 'st_blksize' fields are ignored. The 'st_ino' field is
   * ignored except if the 'use_ino' mount option is given."
   * -- https://libfuse.github.io/doxygen/structfuse__operations.html
   */
  char *table  = NULL;
  char *row    = NULL;
  char *column = NULL;
  sqlite3_stmt *stmt = NULL;
  int retval = 0; /* Value to be returned by this function */
  int r      = 0; /* Used to receive return values of calling functions */

  bool is_file = is_path_file(path);
  mdbfs_debug("%s should be a %s", path, is_file ? "file (0644)" : "directory (0755)");

  if (!path_exists(path)) {
    mdbfs_debug("%s should not exist", path);
    retval = -ENOENT;
    goto quit;
  }

  r = extract_path(&table, &row, &column, path);
  if (!r) {
    retval = -ENOENT;
    goto quit;
  }

  mdbfs_debug("%s exists", path);

  if (is_file) {

    /* Regular file, 0644 */
    stat->st_mode = S_IFREG |
                    S_IRUSR | S_IWUSR |
                    S_IRGRP |
                    S_IROTH;

    /* Select the cell to see the size of it */
    size_t sql_length = strlen(fmt_sql_select_from_where) + strlen(column) + strlen(table) + strlen("ROWID") + strlen(row);
    char *sql = mdbfs_malloc(sql_length);
    snprintf(sql, sql_length, fmt_sql_select_from_where, column, table, "ROWID", row);

    mdbfs_debug("To be prepared: %s", sql);

    r = sqlite3_prepare_v2(_db, sql, -1, &stmt, NULL);
    if (r != SQLITE_OK) {
      mdbfs_error("getattr: sqlite3 cannot prepare a SQL statement for us: %s", sqlite3_errmsg(_db));
      retval = -EFAULT;
      goto quit_free;
    }

    mdbfs_free(sql);

    /* Execute the SQL query */
    r = sqlite3_step(stmt);
    if (r != SQLITE_ROW) {
      mdbfs_warning("getattr: sqlite3 execution error: %s", sqlite3_errmsg(_db));
      retval = -EFAULT;
      goto quit_finalize;
    }

    /* Size to return to the caller */
    stat->st_size = sqlite3_column_bytes(stmt, 0);

    /* Extra check on more results */
    r = sqlite3_step(stmt);
    if (r != SQLITE_DONE) {
      mdbfs_warning("getattr: more than 1 result for the query, ignoring...");
    }

quit_finalize:
    /* Destruct statement */
    r = sqlite3_finalize(stmt);
    if (r != SQLITE_OK) {
      mdbfs_warning("getattr: sqlite3 cannot finalize the SQL statement: %s", sqlite3_errmsg(_db));
      mdbfs_warning("getattr: dropping the statement object anyway, but memory leak happens...");
    }

quit_free:
    mdbfs_free(column);
    mdbfs_free(row);
    mdbfs_free(table);

  } else {

    /* Directory file, 0755 */
    stat->st_mode = S_IFDIR |
                    S_IRUSR | S_IWUSR | S_IXUSR |
                    S_IRGRP | S_IXGRP |
                    S_IROTH | S_IXOTH;

    /* XXX: What is the size of directory? */
    stat->st_size = 1;

  }

quit:
  return retval;
}

/**
 *
 */
static int _readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileinfo, enum fuse_readdir_flags flags)
{
  char *table  = NULL;
  char *row    = NULL;
  char *column = NULL;
  sqlite3_stmt *stmt = NULL;
  int retval = 0; /* Value to be returned by this function */
  int r      = 0; /* Used to receive return values of calling functions */

  r = extract_path(&table, &row, &column, path);
  if (!r) {
    retval = -ENOENT;
    goto quit;
  }

  mdbfs_debug("%s -> /%s/%s/%s", path, table, row, column);

  if (table && row && column) {
    /* File access */
    retval = -ENOENT;
    goto quit_free;
  }

  if (!table && !row && !column) {

    /* Listing root; show table names */
    r = sqlite3_prepare_v2(_db, sql_get_tables, -1, &stmt, NULL);
    if (r != SQLITE_OK) {
      mdbfs_error("readdir: sqlite3 cannot prepare a SQL statement for us: %s", sqlite3_errmsg(_db));
      retval = -ENOENT;
      goto quit_free;
    }

    /* Execute the SQL query
     * The result should be one single column with all the table names
     */
    for (;;) {
      r = sqlite3_step(stmt);
      if (r != SQLITE_ROW)
        break;

      const char const *table_name = sqlite3_column_text(stmt, 0);
      if (!table_name) {
        mdbfs_warning("readdir: sqlite3 returned a NULL for table name, this is not expected");
        continue;
      }

      /* Use filler provided by FUSE (passed by parameter) to add an entry in
       * the file system
       */
      struct stat attr = {0};
      _getattr(path, &attr, fileinfo);
      filler(buf, table_name, &attr, 0, 0);
    }

    /* If the condition causing a break in for loop is not SQLITE_DONE, then
     * something went wrong, but we don't yield an error.
     */
    if (r != SQLITE_DONE) {
      mdbfs_warning("readdir: sqlite3 execution error: %s", sqlite3_errmsg(_db));
      retval = -ENOENT;
      goto quit_finalize;
    }

  } else if (table && !row && !column) {

    /* Listing table: show all rows (with ROWID as dirname) */
    size_t sql_length = strlen(fmt_sql_select_from) + strlen("ROWID") + strlen(table);
    char *sql = mdbfs_malloc(sql_length);
    snprintf(sql, sql_length, fmt_sql_select_from, "ROWID", table);

    mdbfs_debug("To be prepared: %s", sql);

    r = sqlite3_prepare_v2(_db, sql, -1, &stmt, NULL);
    if (r != SQLITE_OK) {
      mdbfs_error("readdir: sqlite3 cannot prepare a SQL statement for us: %s", sqlite3_errmsg(_db));
      retval = -ENOENT;
      goto quit_free;
    }

    mdbfs_free(sql);

    /* Execute the SQL query
     * The result should be a single column with all ROWIDs in the table
     */
    for (;;) {
      r = sqlite3_step(stmt);
      if (r != SQLITE_ROW)
        break;

      const char const *row_id = sqlite3_column_text(stmt, 0);
      if (!row_id) {
        mdbfs_warning("readdir: sqlite3 returned a NULL for table name, this is not expected");
        continue;
      }

      /* Use filler provided by FUSE (passed by parameter) to add an entry in
       * the file system
       */
      struct stat attr = {0};
      _getattr(path, &attr, fileinfo);
      filler(buf, row_id, &attr, 0, 0);
    }

    /* If the condition causing a break in for loop is not SQLITE_DONE, then
     * something went wrong.
     */
    if (r != SQLITE_DONE) {
      mdbfs_warning("readdir: sqlite3 execution error: %s", sqlite3_errmsg(_db));
      retval = -EINTR;
      goto quit_finalize;
    }

  } else if (table && row && !column) {

    /* Listing row: show all columns */
    size_t sql_length = strlen(fmt_sql_select_from_where) + strlen("*") + strlen(table) + strlen("ROWID") + strlen(row);
    char *sql = mdbfs_malloc(sql_length);
    snprintf(sql, sql_length, fmt_sql_select_from_where, "*", table, "ROWID", row);

    mdbfs_debug("To be prepared: %s", sql);

    r = sqlite3_prepare_v2(_db, sql, -1, &stmt, NULL);
    if (r != SQLITE_OK) {
      mdbfs_error("readdir: sqlite3 cannot prepare a SQL statement for us: %s", sqlite3_errmsg(_db));
      retval = -ENOENT;
      goto quit_free;
    }

    /* Execute the SQL query
     * The result should be one single row with column contents, but since we
     * only want to list the column names, we get metadata from SQLite instead
     */
    r = sqlite3_step(stmt);
    if (r != SQLITE_ROW) {
      mdbfs_warning("readdir: sqlite3 execution error: %s", sqlite3_errmsg(_db));
      retval = -EINTR;
      goto quit_finalize;
    }

    /* The result should be one single row with column contents
     * Since we only want to list the column names, we get metadata from SQLite
     */
    int ncol = sqlite3_column_count(stmt);
    for (int icol = 0; icol < ncol; icol++) {
      const char const *column_name = sqlite3_column_origin_name(stmt, icol);

      struct stat attr = {0};
      _getattr(path, &attr, fileinfo);
      filler(buf, column_name, &attr, 0, 0);
    }

    /* Extra check */
    r = sqlite3_step(stmt);
    if (r != SQLITE_DONE) {
      mdbfs_warning("readdir: more than 1 row returned by sqlite, ignoring");
    }

  }

quit_finalize:
  /* Destruct statement */
  r = sqlite3_finalize(stmt);
  if (r != SQLITE_OK) {
    mdbfs_warning("readdir: sqlite3 cannot finalize the SQL statement: %s", sqlite3_errmsg(_db));
    mdbfs_warning("readdir: dropping the statement object anyway, but memory leak happens...");
  }

quit_free:
  mdbfs_free(column);
  mdbfs_free(row);
  mdbfs_free(table);

quit:
  return retval;
}

/**
 *
 */
static int _symlink(const char *path1, const char *path2)
{
  /* So far we only implement a read-only file system */
  return -EROFS;
}

/********** Public APIs **********/

struct mdbfs_operations *mdbfs_backend_sqlite_register(void)
{
  struct mdbfs_operations *ret = mdbfs_malloc0(sizeof (struct mdbfs_operations));

  ret->load     = _load;
  ret->init     = _init;
  ret->destroy  = _destroy;
  ret->create   = _create;
  ret->rename   = _rename;
  ret->unlink   = _unlink;
  ret->mkdir    = _mkdir;
  ret->rmdir    = _rmdir;
  ret->read     = _read;
  ret->write    = _write;
  ret->readdir  = _readdir;
  ret->getattr  = _getattr;
  ret->symlink  = _symlink;

  return ret;
}
