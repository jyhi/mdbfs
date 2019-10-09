/**
 * @file backend.h
 *
 * Definition of the MDBFS backend dispatcher.
 */

#ifndef MDBFS_BACKEND_H
#define MDBFS_BACKEND_H

#include "mdbfs-config.h"
#include <fuse.h>

/**
 * Structure representing a mdbfs_backend.
 */
struct mdbfs_backend {
  /**
   * Get name of the backend.
   */
  const char *(*get_name)(void);

  /**
   * Get description of the backend.
   */
  const char *(*get_description)(void);

  /**
   * Get help message from the backend.
   */
  const char *(*get_help)(void);

  /**
   * Get version string from the backend.
   */
  const char *(*get_version)(void);

  /**
   * Initialize the backend.
   *
   * Command line arguments are supplied for backends to support their own
   * command line options.
   *
   * @param argc [in] Argument count from command line.
   * @param argv [in] Argument vector from command line.
   * @return 1 if the operation succeeded, 0 otherwise.
   */
  int (*init)(int argc, char **argv);

  /**
   * De-initialize the backend and free related resources.
   *
   * Normally this should be done when FUSE signals `destroy`.
   */
  void (*deinit)(void);

  /**
   * Tell the backend to open the database located in the given path.
   *
   * @param path [in] The path to database. The format of path depends on the
   *                  implementation of backend.
   * @return 1 if the operation succeeded. On failure, the error code should be
   *         returned in a negated form (e.g. if the database cannot be found
   *         and 2 should be returned to the operating system, then this
   *         function should return -2).
   */
  int (*open)(const char *path);

  /**
   * Close the database and free related resources.
   *
   * Normally this should be done when FUSE signals `destroy`.
   */
  void (*close)(void);

  /**
   * Get the `fuse_operations` structure for FUSE use.
   */
  struct fuse_operations (*get_fuse_operations)(void);
};

/**
 * Get the MDBFS backend with the given name.
 *
 * Possible values of backends may vary depending on the compile-time options.
 * See `src/backends/list.h` for a full list of supported backends.
 *
 * @param name [in] A string representing the backend to choose.
 * @return A pointer to a `struct mdbfs_backend`. On failure (e.g. the named
 *         backend cannot be found) NULL will be returned.
 */
struct mdbfs_backend *mdbfs_backend_get(const char *name);

/**
 * Get help messages of backends and return them in a single string.
 *
 * @return A help message string. The caller is responsible for freeing the
 *         memory.
 */
char *mdbfs_backends_get_help(void);

/**
 * Get version strings of backends and return them in a single string.
 *
 * @return A help message string. The caller is responsible for freeing the
 *         memory.
 */
char *mdbfs_backends_get_version(void);

#endif
