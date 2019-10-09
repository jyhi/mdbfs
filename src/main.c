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
#include "backend.h"
#include "utils/memory.h"
#include "utils/print.h"

/**
 * Internal global structure holding the accepted command line options fed by
 * the user.
 */
static struct _cmdline_options {
  char *type;      /**< Database type */
  char *path;      /**< Path to the database file */
  int   show_help; /**< Whether help message should be shown */
  int   show_version; /**< Whether version information should be shown */
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
  CMDLINE_OPTION("--help", show_help),
  CMDLINE_OPTION("-h", show_help),
  CMDLINE_OPTION("--version", show_version),
  CMDLINE_OPTION("-v", show_version),
  FUSE_OPT_END,
};

/**
 * Print MDBFS-specific help message to stdout.
 */
void show_help(const char const *progname) {
  char *backend_helps = mdbfs_backends_get_help();

  printf(
    "%s: %s, version %s\n"
    "\n"
    "usage: %s [options] <mountpoint>\n"
    "\n"
    "    --db=<s>      Path to the database to mount.\n"
    "                  Depending on the database backend type, this may vary.\n"
    "    --type=<s>    Specify the type of database (backend).\n"
    "\n"
    "Help messages from backends:\n"
    "\n"
    "%s",
    PROJECT_NAME, PROJECT_DESCRIPTION, PROJECT_VERSION, progname, backend_helps
  );

  mdbfs_free(backend_helps);
}

/**
 * Print MDBFS version with its backend versions to stdout.
 */
void show_version(void)
{
  char *backend_versions = mdbfs_backends_get_version();

  printf(
    "%s version %s\n%s", /* The returned versions have a blank line */
    PROJECT_NAME, PROJECT_VERSION, backend_versions
  );

  mdbfs_free(backend_versions);
}

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
  struct mdbfs_backend *backend = NULL;

  /* Deal with command line arguments first */
  r = fuse_opt_parse(&args, &cmdline_options, cmdline_option_spec, NULL);
  if (r != 0)
    return 1;

  if (cmdline_options.show_help) {
    /* From FUSE example/hello.c: first print our own file-system specific help
       text, then signal fuse_main to show additional help (by adding `--help` to
       the options again) without usage: line (by setting argv[0] to the empty
       string) */
    show_help(argv[0]);
    fuse_opt_add_arg(&args, "--help");
    args.argv[0][0] = '\0';

    goto fusemain;
  }

  if (cmdline_options.show_version) {
    show_version();
    goto quit;
  }

  if (!cmdline_options.path) {
    mdbfs_info("database path is missing; use --db= to specify a database.");
    r = 2;
    goto quit;
  }

  if (!cmdline_options.type) {
    mdbfs_info("you must specify a database backend type.");
    r = 1;
    goto quit;
  }

  backend = mdbfs_backend_get(cmdline_options.type);
  if (!backend) {
    mdbfs_error("type \"%s\" does not match any supported database backend.", cmdline_options.type);
    r = 1;
    goto quit;
  }

  r = backend->init(argc, argv);
  if (!r) {
    mdbfs_error("backend \"%s\" encounters an error during initialization.", cmdline_options.type);
    r = 1;
    goto quit;
  }

  r = backend->open(cmdline_options.path);
  if (r <= 0) {
    mdbfs_error("backend \"%s\" cannot open the database: %s", cmdline_options.type, strerror(-r));
    backend->deinit();
    r = -r;
    goto quit;
  }

fusemain:
  if (backend) {
    struct fuse_operations fuse_ops = backend->get_fuse_operations();
    r = fuse_main(args.argc, args.argv, &fuse_ops, NULL);
  } else {
    r = fuse_main(args.argc, args.argv, NULL, NULL);
  }

quit:
  /* Free unused memory (2nd wave) */
  fuse_opt_free_args(&args);
  mdbfs_free(backend);
  mdbfs_free(cmdline_options.type);
  mdbfs_free(cmdline_options.path);

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
 */
