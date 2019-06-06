/**
 * @file db_sqlite.c
 *
 * Implementation of the MDBFS SQLite database backend.
 */

#include <mdbfs-operations.h>
#include <utils/mdbfs-memory.h>

struct mdbfs_operations *mdbfs_register(void)
{
  struct mdbfs_operations *ret = mdbfs_malloc0(sizeof (struct mdbfs_operations));

  // TODO

  return ret;
}
