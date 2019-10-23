/**
 * @file fuseops.c
 *
 * Implementation of the MDBFS Berkeley DB database backend.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include "utils/memory.h"
#include "utils/path.h"
#include "utils/print.h"
#include "dbmgr.h"
#include "fuseops.h"

/********** Private APIs **********/

/**
 * Extract record name from a legitimate path string.
 *
 * @param path [in] Path string.
 * @return The key name of record if the path is legitimate. If the path should
 *         not exist in the file system, the function will return NULL.
 */
static char *key_from_path(const char *path)
{
  char *ret = NULL;
  size_t ret_length = 0;

  if (!path) {
    mdbfs_error("berkeleydb: key_from_path: path is missing");
    return NULL;
  }

  char *normalized_path = mdbfs_path_lexically_normal(path);
  if (!mdbfs_path_is_absolute(normalized_path)) {
    mdbfs_warning("berkeleydb: key_from_path: not an absolute path");
    mdbfs_free(normalized_path);
    return NULL;
  }

  /* Special case: root */
  if (strcmp("/", normalized_path) == 0) {
    ret = mdbfs_malloc0(sizeof(char)); /* NUL */
    return ret;
  }

  /* Find the first slash and the second slash, between which should be the
   * record key.
   */
  const char *p_start = normalized_path + 1;
  const char *p_end   = p_start;

  while (*p_end && *p_end != '/')
    ++p_end;

  ret_length = p_end - p_start;
  ret = mdbfs_malloc0(ret_length + 1);
  memcpy(ret, p_start, ret_length);

  /* If there is still anything, the path is illegal */
  if (*p_end) {
    mdbfs_warning("berkeleydb: the path \"%s\" contains more than 1 component, which is illegal", path);
    mdbfs_free(ret);
    mdbfs_free(normalized_path);
    return NULL;
  }

  mdbfs_debug("berkeleydb: legitimate path %s", path);

  mdbfs_free(normalized_path);
  return ret;
}

/********** FUSE APIs **********/

static void *_init(struct fuse_conn_info *conn, struct fuse_config *cfg)
{
  (void)conn;

  cfg->use_ino   = 0;
  cfg->direct_io = 1;

  return NULL;
}

static void _destroy(void *private_data)
{
  (void)private_data;

  mdbfs_backend_berkeleydb_close_database();
}

static int _mknod(const char *path, mode_t mode, dev_t device)
{
  char *key = NULL;
  int ret = 0; /* Value to be returned by the function */
  int r = 0;   /* Value returned by other functions */

  /* XXX: Mode is fixed, see getattr. */
  (void)mode;
  (void)device;

  key = key_from_path(path);
  if (!key) {
    ret = -EINVAL;
    goto quit;
  }

  r = mdbfs_backend_berkeleydb_create_record(key);
  if (!r) {
    ret = -EINVAL;
    goto quit;
  }

quit:
  mdbfs_free(key);
  return ret;
}

static int _rename(const char *path1, const char *path2, unsigned int flags)
{
  char *key_old = NULL;
  char *key_new = NULL;
  int ret = 0; /* Value to be returned by the function */
  int r = 0;   /* Value returned by other functions */

  /* FIXME: flags should be respected */
  (void)flags;

  key_old = key_from_path(path1);
  if (!key_old) {
    ret = -EINVAL;
    goto quit;
  }

  key_new = key_from_path(path2);
  if (!key_new) {
    ret = -EINVAL;
    goto quit;
  }

  r = mdbfs_backend_berkeleydb_rename_record(key_old, key_new);
  if (!r) {
    ret = -EINVAL;
    goto quit;
  }

quit:
  mdbfs_free(key_old);
  mdbfs_free(key_new);
  return ret;
}

static int _unlink(const char *path)
{
  char *key = NULL;
  int ret = 0; /* Value to be returned by the function */
  int r = 0;   /* Value returned by other functions */

  key = key_from_path(path);
  if (!key) {
    ret = -EINVAL;
    goto quit;
  }

  r = mdbfs_backend_berkeleydb_remove_record(key);
  if (!r) {
    ret = -EINVAL;
    goto quit;
  }

quit:
  mdbfs_free(key);
  return ret;
}

static int _read(const char *path, char *buf, size_t bufsize, off_t offset, struct fuse_file_info *fileinfo)
{
  char *key = NULL;
  uint8_t *content = NULL;
  size_t content_size = 0;
  int ret = 0; /* Value to be returned by the function */
  int r = 0;   /* Value returned by other functions */

  /* XXX: Ignoring fileinfo from FUSE */
  (void)fileinfo;

  key = key_from_path(path);
  if (!key) {
    ret = -EINVAL;
    goto quit;
  }

  content = mdbfs_backend_berkeleydb_get_record_value(&content_size, key);
  if (!content) {
    ret = -EINVAL;
    goto quit;
  }

  /* "This memory cannot be read" */
  if (offset >= content_size) {
    ret = 0;
    goto quit;
  }

  /* Return the read result and "file size" */
  /* If offset is given, read from there */
  const uint8_t *p_content = content + offset;

  /* Either copy all the content from database or occupy all the buffer */
  size_t copy_size = content_size <= bufsize ? content_size : bufsize;

  memcpy(buf, p_content, copy_size);
  ret = copy_size;

quit:
  mdbfs_free(content);
  mdbfs_free(key);
  return ret;
}

static int _write(const char *path, const char *buf, size_t bufsize, off_t offset, struct fuse_file_info *fileinfo)
{
  char *key = NULL;
  int ret = 0; /* Value to be returned by the function */
  int r = 0;   /* Value returned by other functions */

  /* XXX: No offset support */
  if (offset > 0)
    return 0;

  /* XXX: Ignoring fileinfo from FUSE */
  (void)fileinfo;

  key = key_from_path(path);
  if (!key) {
    ret = -EINVAL;
    goto quit;
  }

  r = mdbfs_backend_berkeleydb_set_record_value(key, buf, bufsize);
  if (!r) {
    ret = -EINVAL;
    goto quit;
  }

  /* Bytes written is the length of buffer */
  ret = bufsize;

quit:
  mdbfs_free(key);
  return ret;
}

static int _getattr(const char *path, struct stat *stat, struct fuse_file_info *fileinfo)
{
  char *key = NULL;
  uint8_t *content = NULL;
  size_t content_size = 0;
  int ret = 0; /* Value to be returned by the function */
  int r = 0;   /* Value returned by other functions */

  key = key_from_path(path);
  if (!key) {
    ret = -ENOENT;
    goto quit;
  }

  /* Only the root is a directory */
  if (strcmp(key, "") == 0) {

    /* Directory file, 0755 */
    stat->st_mode = S_IFDIR |
                    S_IRUSR | S_IWUSR | S_IXUSR |
                    S_IRGRP | S_IXGRP |
                    S_IROTH | S_IXOTH;

    /* XXX: Directories do not have a size */
    stat->st_size = 0;

  } else {

    /* To know attributes we have to fetch the whole record */
    content = mdbfs_backend_berkeleydb_get_record_value(&content_size, key);
    if (!content) {
      ret = -ENOENT;
      goto quit;
    }

    /* Regular file, 0644 */
    stat->st_mode = S_IFREG |
                    S_IRUSR | S_IWUSR |
                    S_IRGRP |
                    S_IROTH;

    /* Size to return to the caller */
    stat->st_size = content_size;

  }

quit:
  mdbfs_free(content);
  mdbfs_free(key);
  return ret;
}

static int _readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fileinfo, enum fuse_readdir_flags flags)
{
  char *key = NULL;
  char **record_keys = NULL;
  int ret = 0; /* Value to be returned by the function */
  int r = 0;   /* Value returned by other functions */

  /* XXX: No offset support */
  if (offset > 0)
    return 0;

  /* FIXME: These should be useful */
  (void)fileinfo;
  (void)flags;

  key = key_from_path(path);
  if (!key) {
    ret = -ENOENT;
    goto quit;
  }

  /* There is only one directory: root */
  if (strcmp(key, "") != 0) {
    ret = -ENOENT;
    goto quit;
  }

  record_keys = mdbfs_backend_berkeleydb_get_record_keys();
  if (!record_keys) {
    ret = -EINVAL;
    goto quit;
  }

  for (int i = 0; record_keys[i]; i++) {
    /* XXX: We don't know, but empty filename should not appear, as it seems to
     * be legitimate in the database.
     */
    if (strcmp(record_keys[i], "") == 0)
      continue;

    /* Construct a path to the record */
    char *path_record = mdbfs_malloc0(strlen("/") + strlen(record_keys[i]) + 1);
    strcat(path_record, "/");
    strcat(path_record, record_keys[i]);

    /* Attribution information is required */
    struct stat attr = {0};
    _getattr(path_record, &attr, fileinfo);

    /* Send elements back to FUSE */
    filler(buf, record_keys[i], &attr, 0, 0);

    /* Free unused memory */
    mdbfs_free(path_record);
  }

  /* Free unused memory */
  for (int i = 0; record_keys[i]; i++)
    mdbfs_free(record_keys[i]);
  mdbfs_free(record_keys);

quit:
  mdbfs_free(key);
  return ret;
}

/********** Public APIs **********/

struct mdbfs_backend_berkeleydb_operations mdbfs_backend_berkeleydb_get_operations(void)
{
  return (struct mdbfs_backend_berkeleydb_operations) {
    .init     = _init,
    .destroy  = _destroy,

    .mknod    = _mknod,
    .rename   = _rename,
    .unlink   = _unlink,

    .read     = _read,
    .write    = _write,
    .readdir  = _readdir,

    .getattr  = _getattr,
  };
}
