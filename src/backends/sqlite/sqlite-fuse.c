/**
 * @file sqlite-fuse.c
 *
 * Implementation of the MDBFS SQLite database backend.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "utils/memory.h"
#include "utils/path.h"
#include "utils/print.h"
#include "dbmgr.h"
#include "sqlite-fuse.h"

/********** Private APIs **********/

/**
 * Type of a path in this backend, used in `struct mdbfs_sqlite_path`.
 */
enum mdbfs_sqlite_path_type {
  MDBFS_SQLITE_PATH_TYPE_DATABASE, ///< The path is pointing at database level.
  MDBFS_SQLITE_PATH_TYPE_TABLE,    ///< The path is pointing at table level.
  MDBFS_SQLITE_PATH_TYPE_ROW,      ///< The path is pointing at row level.
  MDBFS_SQLITE_PATH_TYPE_COLUMN,   ///< The path is pointing to column level.
};

/**
 * Private structure representing segments in a legitimate path in this backend.
 */
struct mdbfs_sqlite_path {
  enum mdbfs_sqlite_path_type type; ///< Where the path is pointing to
  char *table;  ///< Table name (1st component in the path)
  char *row;    ///< Row name (2nd component in the path)
  char *column; ///< Column name (3rd component in the path)
};

/**
 * Free a `struct mdbfs_sqlite_path`.
 *
 * Note that this only frees the content of the structure, not the structure
 * itself. Use mdbfs_free to free the structure itself.
 *
 * @param sqlite_path [in] `struct mdbfs_sqlite_path` to free.
 */
static void mdbfs_sqlite_path_free(struct mdbfs_sqlite_path *sqlite_path)
{
  mdbfs_free(sqlite_path->table);
  mdbfs_free(sqlite_path->row);
  mdbfs_free(sqlite_path->column);
}

/**
 * Convert a legitimate path string into `struct mdbfs_sqlite_path`.
 *
 * @param path [in] Path string.
 * @return A `struct mdbfs_sqlite_path` if the path is legitimate. If the path
 *         should not exist in the file system, the function will return NULL.
 */
static struct mdbfs_sqlite_path *mdbfs_sqlite_path_from_string(const char *path)
{
  if (!path) {
    mdbfs_error("sqlite: path_from_string: path is missing");
    return NULL;
  }

  char *normalized_path = mdbfs_path_lexically_normal(path);
  if (!mdbfs_path_is_absolute(normalized_path)) {
    mdbfs_warning("sqlite: not an absolute path");
    mdbfs_free(normalized_path);
    return NULL;
  }

  /* Fill the structure with NULL to make it valid */
  struct mdbfs_sqlite_path *ret = mdbfs_malloc0(sizeof(struct mdbfs_sqlite_path));

  const char *p_table  = NULL; /* Pointer to the table component */
  const char *p_row    = NULL; /* Pointer to the row component */
  const char *p_column = NULL; /* Pointer to the column component */
  size_t table_length  = 0;    /* Length of the table component */
  size_t row_length    = 0;    /* Length of the row component */
  size_t column_length = 0;    /* Length of the column component */

  /* Special case: root */
  if (strcmp("/", normalized_path) == 0) {
    ret->type = MDBFS_SQLITE_PATH_TYPE_DATABASE;
    goto finish;
  }

  /* Walk the path:
   *
   *     /path/to/cell
   *      ^   ^
   *      |   +-- p_end
   *      +-- p_start
   */
  const char *p_start = normalized_path + 1;
  const char *p_end   = p_start;

  /* Table */
  while (*p_end && *p_end != '/')
    ++p_end;

  table_length = p_end - p_start;
  ret->table = mdbfs_malloc0(table_length + 1);
  memcpy(ret->table, p_start, table_length);

  if (!*p_end) {
    ret->type = MDBFS_SQLITE_PATH_TYPE_TABLE;
    goto finish;
  }

  p_start = p_end + 1;
  p_end = p_start;

  /* Row */
  while (*p_end && *p_end != '/')
    ++p_end;

  table_length = p_end - p_start;
  ret->row = mdbfs_malloc0(table_length + 1);
  memcpy(ret->row, p_start, table_length);

  if (!*p_end) {
    ret->type = MDBFS_SQLITE_PATH_TYPE_ROW;
    goto finish;
  }

  p_start = p_end + 1;
  p_end = p_start;

  /* Column */
  while (*p_end && *p_end != '/')
    ++p_end;

  table_length = p_end - p_start;
  ret->column = mdbfs_malloc0(table_length + 1);
  memcpy(ret->column, p_start, table_length);

  if (!*p_end) {
    ret->type = MDBFS_SQLITE_PATH_TYPE_COLUMN;
    goto finish;
  }

  p_start = p_end + 1;
  p_end = p_start;

  /* If there is still anything, the path is illegal */
  mdbfs_warning("sqlite: the path \"%s\" contains more than 3 components, which is illegal", path);
  mdbfs_free(ret->table);
  mdbfs_free(ret->row);
  mdbfs_free(ret->column)
  mdbfs_free(ret);
  mdbfs_free(normalized_path);
  return NULL;

finish:
  mdbfs_debug("sqlite: legitimate path %s", path);

  mdbfs_free(normalized_path);
  return ret;
}

/********** FUSE APIs **********/

/**
 * Initialize global (FUSE) configurations of the file system.
 */
static void *_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
  (void)conn;

  cfg->use_ino   = 0;
  cfg->direct_io = 1;

  return NULL;
}

/**
 * Close the file system gracefully.
 */
static void _destroy(void *private_data)
{
  (void)private_data;

  mdbfs_backend_sqlite_close_database();
}

/**
 * Create a file.
 *
 * @param path [in] The path to the file to be created.
 * @param mode [in] Mode to be applied to the new file.
 * @param device [inout] Something.
 * @return 0 on success, any negative error code on failure.
 */
static int _mknod(const char *path, mode_t mode, dev_t device)
{
  struct mdbfs_sqlite_path *sqlite_path = NULL;
  int ret = 0; /* Value to be returned by the function */
  int r = 0;   /* Value returned by other functions */

  /* XXX: Mode is fixed, see getattr. */
  (void)mode;
  (void)device;

  sqlite_path = mdbfs_sqlite_path_from_string(path);
  if (!sqlite_path) {
    ret = -EINTR;
    goto quit;
  }

  /* One cannot create a file on directory (db, table, row) levels */
  if (sqlite_path->type != MDBFS_SQLITE_PATH_TYPE_COLUMN) {
    ret = -EROFS;
    goto quit;
  }

  r = mdbfs_backend_sqlite_create_column(sqlite_path->table, sqlite_path->column);
  if (!r) {
    ret = -EINTR;
    goto quit;
  }

quit:
  mdbfs_sqlite_path_free(sqlite_path);
  mdbfs_free(sqlite_path);
  return ret;
}

/**
 * Rename, cpoy, or move a directory or a file.
 *
 * @param path1 [in] The original path.
 * @param path2 [in] The target path.
 * @parma flags [in] A flag given by FUSE representing a renaming scheme, which
 *                   may be either `RENAME_EXCHANGE` (move) or
 *                   `RENAME_NOREPLACE` (copy).
 * @return 0 on success, any negative error code on failure.
 */
static int _rename(const char *path1, const char *path2, unsigned int flags)
{
  struct mdbfs_sqlite_path *sqlite_path_old = NULL;
  struct mdbfs_sqlite_path *sqlite_path_new = NULL;
  int ret = 0; /* Value to be returned by the function */
  int r = 0;   /* Value returned by other functions */

  /* FIXME: flags should be respected */
  (void)flags;

  sqlite_path_old = mdbfs_sqlite_path_from_string(path1);
  if (!sqlite_path_old) {
    mdbfs_warning("sqlite: rename: illegal original path %s", path1);
    ret = -EINTR;
    goto quit;
  }

  sqlite_path_new = mdbfs_sqlite_path_from_string(path2);
  if (!sqlite_path_new) {
    mdbfs_warning("sqlite: rename: illegal new path %s", path2);
    ret = -EINTR;
    goto quit;
  }

  /* It's just impossible to move things around */
  if (sqlite_path_old->type != sqlite_path_new->type) {
    ret = -ENOSPC;
    goto quit;
  }

  if (sqlite_path_old->type == MDBFS_SQLITE_PATH_TYPE_DATABASE) {

    /* There is no way to rename a root */
    mdbfs_warning("sqlite: rename: cannot rename the root");
    ret = -EROFS;
    goto quit;

  } else if (sqlite_path_old->type == MDBFS_SQLITE_PATH_TYPE_TABLE) {

    /* Renaming a table */
    r = mdbfs_backend_sqlite_rename_table(sqlite_path_old->table, sqlite_path_new->table);
    if (!r) {
      ret = -ENOSPC;
      goto quit;
    }

  } else if (sqlite_path_old->type == MDBFS_SQLITE_PATH_TYPE_ROW) {

    /* Renaming a row */
    r = mdbfs_backend_sqlite_rename_row(sqlite_path_old->table, sqlite_path_old->row, sqlite_path_new->row);
    if (!r) {
      ret = -ENOSPC;
      goto quit;
    }

  } else if (sqlite_path_old->type == MDBFS_SQLITE_PATH_TYPE_COLUMN) {

    /* Renaming a column */
    r = mdbfs_backend_sqlite_rename_column(sqlite_path_old->table, sqlite_path_old->column, sqlite_path_new->column);
    if (!r) {
      ret = -ENOSPC;
      goto quit;
    }

  } else {

    /* Something happened */
    ret = -EINTR;
    goto quit;

  }

quit:
  mdbfs_sqlite_path_free(sqlite_path_old);
  mdbfs_sqlite_path_free(sqlite_path_new);
  mdbfs_free(sqlite_path_old);
  mdbfs_free(sqlite_path_new);
  return ret;
}

/**
 * Remove a file at path.
 *
 * This always fails, as SQLite does not support dropping columns (which are
 * mapped by files in this driver).
 *
 * @param path [in] The file to be removed.
 * @return -EROFS.
 */
static int _unlink(const char *path)
{
  (void)path;

  return -EROFS;
}

/**
 * Create a directory at path.
 *
 * This always fails, since in this backend we do not allow directory creation:
 *
 * - Creating a directory mapping to a table creates a table without any column,
 *   which is illegal.
 * - Creating a directory mapping to a row creates a row, but the name of the
 *   directory is hard to validate.
 *
 * Thus, this function always return -EROFS (read-only file system), although in
 * the future this may be implemented in an acceptable way.
 *
 * @param path [in] The path to create.
 * @param mode [in] The file mode to be applied to the new directory.
 * @return 0 on success, any negative error code on failure.
 */
static int _mkdir(const char *path, mode_t mode)
{
  (void)path;
  (void)mode;

  return -EROFS;
}

/**
 * Remove a directory at path.
 *
 * @param path [in] The path to directory to be removed.
 * @return 0 on success, any negative error code on failure.
 */
static int _rmdir(const char *path)
{
  struct mdbfs_sqlite_path *sqlite_path = NULL;
  int ret = 0; /* Value to be returned by the function */
  int r = 0;   /* Value returned by other functions */

  sqlite_path = mdbfs_sqlite_path_from_string(path);
  if (!sqlite_path) {
    ret = -EINTR;
    goto quit;
  }

  /* There should not be any directory in the file level (column) */
  if (sqlite_path->column) {
    ret = -EINTR;
    goto quit;
  }

  if (sqlite_path->type == MDBFS_SQLITE_PATH_TYPE_DATABASE) {

    /* Removing (dropping) the database */
    ret = -EACCES;
    goto quit;

  } else if (sqlite_path->type == MDBFS_SQLITE_PATH_TYPE_TABLE) {

    /* Removing (dropping) a table */
    r = mdbfs_backend_sqlite_remove_table(sqlite_path->table);
    if (!r) {
      ret = -EINTR;
      goto quit;
    }

  } else if (sqlite_path->type == MDBFS_SQLITE_PATH_TYPE_ROW) {

    /* Removing a row */
    r = mdbfs_backend_sqlite_remove_row(sqlite_path->table, sqlite_path->row);
    if (!r) {
      ret = -EINTR;
      goto quit;
    }

  } else {

    /* This should never happen */
    mdbfs_warning("sqlite: rmdir: unexpected directory on file (column) level: %s", path);
    ret = -EINTR;
    goto quit;

  }

quit:
  mdbfs_sqlite_path_free(sqlite_path);
  mdbfs_free(sqlite_path);
  return ret;
}

/**
 * Read content of a file.
 *
 * @param path     [in]  Path to the file to be read.
 * @param buf      [out] The buffer to put file content in.
 * @param bufsize  [in]  Size of the provided buffer.
 * @param offset   [in]  Offset from which the file should be read.
 * @param fileinfo [in,out] FUSE file information structure.
 * @return bytes read, or negated error code on failure.
 */
static int _read(const char *path, char *buf, size_t bufsize, off_t offset, struct fuse_file_info *fileinfo)
{
  struct mdbfs_sqlite_path *sqlite_path = NULL;
  uint8_t *cell = NULL;
  size_t cell_size = 0;
  int ret = 0; /* Value to be returned by the function */

  /* XXX: Ignoring fileinfo from FUSE */
  (void)fileinfo;

  sqlite_path = mdbfs_sqlite_path_from_string(path);
  if (!sqlite_path) {
    ret = -EINTR;
    goto quit;
  }

  if (sqlite_path->type != MDBFS_SQLITE_PATH_TYPE_COLUMN) {
    /* read(3p) is for files (columns), not directories */
    ret = -EISDIR;
    goto quit;
  }

  cell = mdbfs_backend_sqlite_get_cell(&cell_size, sqlite_path->table, sqlite_path->row, sqlite_path->column);
  if (!cell) {
    ret = -ENOENT;
    goto quit;
  }

  /* "This memory cannot be read" */
  if (offset >= cell_size) {
    ret = 0;
    goto quit;
  }

  /* Return the read result and "file size" */
  /* If offset is given, read from there */
  const uint8_t *p_cell = cell + offset;

  /* Either copy all the content from database or occupy all the buffer */
  size_t copy_size = cell_size <= bufsize ? cell_size : bufsize;

  memcpy(buf, p_cell, copy_size);
  ret = copy_size;

quit:
  mdbfs_free(cell);
  mdbfs_sqlite_path_free(sqlite_path);
  mdbfs_free(sqlite_path);
  return ret;
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
  struct mdbfs_sqlite_path *sqlite_path = NULL;
  int ret = 0; /* Value to be returned by the function */
  int r = 0;   /* Value returned by other functions */

  /* XXX: No offset support */
  if (offset > 0)
    return 0;

  /* XXX: Ignoring fileinfo from FUSE */
  (void)fileinfo;

  sqlite_path = mdbfs_sqlite_path_from_string(path);
  if (!sqlite_path) {
    ret = -EINTR;
    goto quit;
  }

  r = mdbfs_backend_sqlite_set_cell(buf, bufsize, sqlite_path->table, sqlite_path->row, sqlite_path->column);
  if (!r) {
    ret = -EINTR;
    goto quit;
  }

  /* Bytes written is the length of buffer */
  ret = bufsize;

quit:
  mdbfs_sqlite_path_free(sqlite_path);
  mdbfs_free(sqlite_path);
  return ret;
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
  struct mdbfs_sqlite_path *sqlite_path = NULL;
  size_t file_size = 0;
  int ret = 0; /* Value to be returned by the function */
  int r = 0;   /* Value returned by other functions */

  sqlite_path = mdbfs_sqlite_path_from_string(path);
  if (!sqlite_path) {
    ret = -ENOENT;
    goto quit;
  }

  /* We have to get data from the database because we don't know if it exists */
  if (sqlite_path->type == MDBFS_SQLITE_PATH_TYPE_DATABASE){

    char **tables = mdbfs_backend_sqlite_get_table_names();

    if (!tables) {
      ret = -ENOENT;
      goto quit;
    }

    for (int i = 0; tables[i]; i++)
      mdbfs_free(tables[i]);
    mdbfs_free(tables);

  } else if (sqlite_path->type == MDBFS_SQLITE_PATH_TYPE_TABLE) {

    char **rows = mdbfs_backend_sqlite_get_row_names(sqlite_path->table);

    if (!rows) {
      ret = -ENOENT;
      goto quit;
    }

    for (int i = 0; rows[i]; i++)
      mdbfs_free(rows[i]);
    mdbfs_free(rows);

  } else if (sqlite_path->type == MDBFS_SQLITE_PATH_TYPE_ROW) {

    char **columns = mdbfs_backend_sqlite_get_column_names(sqlite_path->table, sqlite_path->row);

    if (!columns) {
      ret = -ENOENT;
      goto quit;
    }

    for (int i = 0; columns[i]; i++)
      mdbfs_free(columns[i]);
    mdbfs_free(columns);

  } else if (sqlite_path->type == MDBFS_SQLITE_PATH_TYPE_COLUMN) {

    char *cell = mdbfs_backend_sqlite_get_cell(&file_size, sqlite_path->table, sqlite_path->row, sqlite_path->column);

    if (!cell) {
      ret = -ENOENT;
      goto quit;
    }

    mdbfs_free(cell);

  }

  if (sqlite_path->type == MDBFS_SQLITE_PATH_TYPE_COLUMN) {

    /* Regular file, 0644 */
    stat->st_mode = S_IFREG |
                    S_IRUSR | S_IWUSR |
                    S_IRGRP |
                    S_IROTH;

    /* Size to return to the caller */
    stat->st_size = file_size;

  } else if (sqlite_path->type == MDBFS_SQLITE_PATH_TYPE_DATABASE ||
             sqlite_path->type == MDBFS_SQLITE_PATH_TYPE_TABLE ||
             sqlite_path->type == MDBFS_SQLITE_PATH_TYPE_ROW) {

    /* Directory file, 0755 */
    stat->st_mode = S_IFDIR |
                    S_IRUSR | S_IWUSR | S_IXUSR |
                    S_IRGRP | S_IXGRP |
                    S_IROTH | S_IXOTH;

    /* XXX: Directories do not have a size */
    stat->st_size = 0;

  }

quit:
  mdbfs_sqlite_path_free(sqlite_path);
  mdbfs_free(sqlite_path);
  return ret;
}

/**
 * List content of a directory.
 *
 * @param path [in] Path to the directory.
 * @param buf  [out] The buffer to receive directory information.
 * @param filler [in] A function provided by FUSE to add entries to buf.
 * @param offset [in] Offset for multi-page loading (perhaps).
 * @param fileinfo [in,out] FUSE file information structure.
 * @param flags [in] FUSE flags for readdir.
 * @return 0 if succeeded, negated error codes otherwise.
 */
static int _readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileinfo, enum fuse_readdir_flags flags)
{
  struct mdbfs_sqlite_path *sqlite_path = NULL;
  int ret = 0; /* Value to be returned by the function */
  int r = 0;   /* Value returned by other functions */

  /* XXX: No offset support */
  if (offset > 0)
    return 0;

  /* FIXME: These should be useful */
  (void)fileinfo;
  (void)flags;

  sqlite_path = mdbfs_sqlite_path_from_string(path);
  if (!sqlite_path) {
    ret = -EINTR;
    goto quit;
  }

  /* File access? How can this happen? */
  if (sqlite_path->type == MDBFS_SQLITE_PATH_TYPE_COLUMN) {
    ret = -ENOENT;
    goto quit;
  }

  if (sqlite_path->type == MDBFS_SQLITE_PATH_TYPE_DATABASE) {

    /* Listing root; show table names */
    char **table_names = mdbfs_backend_sqlite_get_table_names();
    if (!table_names) {
      ret = -ENOENT;
      goto quit;
    }

    for (int i = 0; table_names[i]; i++) {
      /* Attribution information is required */
      struct stat attr = {0};
      _getattr(path, &attr, fileinfo);

      /* Send elements back to FUSE */
      filler(buf, table_names[i], &attr, 0, 0);
    }

    /* Free unused memory */
    for (int i = 0; table_names[i]; i++)
      mdbfs_free(table_names[i]);
    mdbfs_free(table_names);

  } else if (sqlite_path->type == MDBFS_SQLITE_PATH_TYPE_TABLE) {

    /* Listing table; show all rows */
    char **row_names = mdbfs_backend_sqlite_get_row_names(sqlite_path->table);
    if (!row_names) {
      ret = -ENOENT;
      goto quit;
    }

    for (int i = 0; row_names[i]; i++) {
      /* Attribution information is required */
      struct stat attr = {0};
      r = _getattr(path, &attr, fileinfo);
      if (r < 0) {
        ret = r;
        goto quit;
      }

      /* Send elements back to FUSE */
      filler(buf, row_names[i], &attr, 0, 0);
    }

    /* Free unused memory */
    for (int i = 0; row_names[i]; i++)
      mdbfs_free(row_names[i]);
    mdbfs_free(row_names);

  } else if (sqlite_path->type == MDBFS_SQLITE_PATH_TYPE_ROW) {

    /* Listing row; show all columns */
    char **column_names = mdbfs_backend_sqlite_get_column_names(sqlite_path->table, sqlite_path->row);
    if (!column_names) {
      ret = -ENOENT;
      goto quit;
    }

    for (int i = 0; column_names[i]; i++) {
      /* Attribution information is required */
      struct stat attr = {0};
      r = _getattr(path, &attr, fileinfo);
      if (r < 0) {
        ret = r;
        goto quit;
      }

      /* Send elements back to FUSE */
      filler(buf, column_names[i], &attr, 0, 0);
    }

    /* Free unused memory */
    for (int i = 0; column_names[i]; i++)
      mdbfs_free(column_names[i]);
    mdbfs_free(column_names);

  }

quit:
  mdbfs_sqlite_path_free(sqlite_path);
  mdbfs_free(sqlite_path);
  return ret;
}

/********** Public APIs **********/

struct mdbfs_backend_sqlite_operations mdbfs_backend_sqlite_get_operations(void)
{
  return (struct mdbfs_backend_sqlite_operations) {
    .init     = _init,
    .destroy  = _destroy,

    .mknod    = _mknod,
    .rename   = _rename,
    .unlink   = _unlink,
    .mkdir    = _mkdir,
    .rmdir    = _rmdir,

    .read     = _read,
    .write    = _write,
    .readdir  = _readdir,

    .getattr  = _getattr,
  };
}
