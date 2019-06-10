/**
 * @file main.c
 *
 * Main entry of the file system driver.
 */

#include "mdbfs-config.h"

#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <fuse.h>
#include "backends/dispatcher.h"
#include "configmgr/configmgr.h"
#include "utils/mdbfs-utils.h"

/**
 * Internal global structure holding the accepted command line options fed by
 * the user.
 */
static struct _cmdline_options {
  char *type;      /**< Database type */
  char *path;      /**< Path to the database file */
  int   show_help; /**< Whether help message should be shown */
} cmdline_options;

/**
 * A convenient macro from FUSE example/hello.c for command line specification
 * definition.
 *
 * @see cmdline_option_spec
 */
#define CMDLINE_OPTION(t, p) \
  {t, offsetof(struct _cmdline_options, p), 1}

/**
 * Command line option specification to be provided to FUSE for option parsing.
 */
static const struct fuse_opt cmdline_option_spec[] = {
  CMDLINE_OPTION("--type=%s", type),
  CMDLINE_OPTION("--db=%s", path),
  FUSE_OPT_END,
};

/**
 * Main entry of the program.
 *
 * @param argc [in] Command-line argument counter.
 * @param argv [in] Command-line argument vector (array), which should be a
 *        NULL-terminated list.
 * @return 0 for success, non-zero for failures.
 */
int main(int argc, char **argv)
{
  int r = 0;
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

  /* Deal with command line arguments first */
  r = fuse_opt_parse(&args, &cmdline_options, cmdline_option_spec, NULL);
  if (r != 0)
    return 1;

  if (!cmdline_options.path) {
    mdbfs_info("database path is missing; use --db= to specify a database.");
    return 2;
  }

  if (!cmdline_options.type) {
    mdbfs_info("you must specify a database backend type.");
    return 1;
  }

  /* Store the path to database for future use */
  mdbfs_configmgr_path_store(strdup(cmdline_options.path));

  /* Get the right backend to use */
  struct mdbfs_operations *ops = mdbfs_backend_get(
    mdbfs_backend_str_to_enum(cmdline_options.type)
  );
  if (!ops) {
    mdbfs_error("type \"%s\" does not match any supported database backend.", cmdline_options.type);
    return 1;
  }

  /* Transform mdbfs_operations to fuse_operations */
  struct fuse_operations fuse_ops = mdbfs_operations_map_to_fuse_operations(*ops);

  /* Free unused memory (1st wave) */
  mdbfs_free(&ops);

  /* Nike -- Just Do It. */
  r = fuse_main(args.argc, args.argv, &fuse_ops, NULL);

  /* Free unused memory (2nd wave) */
  fuse_opt_free_args(&args);
  mdbfs_free(&cmdline_options.type);
  mdbfs_free(&cmdline_options.path);

  return r;
}

/**
 * @mainpage
 *
 * This project aims to provide a way to display any (supported) databases into
 * file systems by acting as a translator between database instructions (e.g.
 * [Structured Querying Language (SQL)][sql] in relational databases) and
 * input / output (I/O) system calls, and thus creating a file system interface
 * upon database management systems. In other words, we are converting databases
 * to a file system on-the-fly, one-to-one, which is also known as "mapping".
 * Thus, the project is titled "Mapping Databases into a File System" (mdbfs).
 *
 * This project is started for my Final Year Project I topic, so the project
 * should be considered highly experimental and unstable -- only as a proof-of-
 * concept (PoC). It is not advised to use it in production environment.
 *
 * [sql]: https://en.wikipedia.org/wiki/SQL
 *
 * @section overview Overview
 *
 * `mdbfs` consists of 3 parts:
 *
 * - a helper utility (`mdbfs`) for users' convenient mounting and unmounting
 * - a file system in userspace (FUSE) to display contents in file system
 *   hierarchy
 * - a database adapter acting as a backend to perform database-specific
 *   operations, and pass contents between the file system driver mentioned
 *   above and the database management systems.
 *
 * They don't have clear boundaries in the code; only for reference.
 *
 * @section hierarchy Repository Hierarchy
 *
 * - `src/` contains source code of this project
 */
