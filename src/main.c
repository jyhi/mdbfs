/**
 * @file main.c
 *
 * Main entry of the file system driver.
 */

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
  (void) argc;
  (void) argv;

  return 0;
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
