/**
 * @file backends.h
 *
 * Private header file containing a list of supported backends.
 */

#ifndef MDBFS_BACKENDS_H
#define MDBFS_BACKENDS_H

#include "sqlite/mdbfs-sqlite.h"

/**
 * Private structure representing a backend and its relative metadata.
 */
struct backend {
  const char const *name; ///< Name of the backend.
  struct mdbfs_operations *(*registerer)(void); ///< Registration function of the database.
};

/**
 * A list of supported backends.
 */
static const struct backend mdbfs_backends[] = {
  {"sqlite", mdbfs_backend_sqlite_register},
  {NULL, NULL},
};

#endif
