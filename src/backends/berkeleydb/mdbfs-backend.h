/**
 * @file mdbfs-backend.h
 *
 * Definition of public interfaces exposed by the Berkeley DB backend for MDBFS.
 */

#ifndef MDBFS_BACKENDS_BERKELEYDB_MDBFS_BACKEND_H
#define MDBFS_BACKENDS_BERKELEYDB_MDBFS_BACKEND_H

#include "backend.h"

/**
 * Retrieve the `struct mdbfs_backend` representing the backend.
 *
 * @return The `struct mdbfs_backend` representing the backend. The caller is
 *         responsible for freeing the memory allocated.
 */
struct mdbfs_backend *mdbfs_backend_berkeleydb_get_mdbfs_backend(void);

#endif
