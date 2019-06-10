/**
 * @file mdbfs-print.h
 *
 * MDBFS specific console printing functions.
 */

#ifndef MDBFS_UTIL_PRINT_H
#define MDBFS_UTIL_PRINT_H

/**
 * Information level for printing.
 *
 * Different levels are printed differently, among which mostly differs by the
 * header. Some levels of printing may be supressed by environment variables,
 * see _mdbfs_println for more information.
 */
enum MdbfsPrintLevel {
  MDBFS_PRINT_LEVEL_DEBUG   = 0x01, /**< A debug message. (0b00000001) */
  MDBFS_PRINT_LEVEL_INFO    = 0x02, /**< An informative message. (0b00000010) */
  MDBFS_PRINT_LEVEL_WARNING = 0x06, /**< A warning message. (0b00000110) */
  MDBFS_PRINT_LEVEL_ERROR   = 0x0E, /**< An erro messsage. (0b00001110) */

  MDBFS_PRINT_LEVEL_STOP    = 0x10, /**< A special bit causing an immediate
                                         abortion of the program. Used in
                                         combination with other levels.
                                         (0b00010000) */
};

/**
 * Print a formatted string with the given level indicator.
 *
 * Use the convenient functions instead.
 *
 * @param level [in] Printing level of the message.
 * @param fmt   [in] String format, followed by the content. The format follows
 *                   what is used in printf family functions.
 * @see mdbfs_debug, mdbfs_info, mdbfs_warning, mdbfs_error, mdbfs_fatal
 */
void mdbfs_println(enum MdbfsPrintLevel level, const char * const fmt, ...);

/**
 * Print a debug message.
 *
 * @param fmt [in] String format, followed by the content.
 */
#define mdbfs_debug(fmt, ...) mdbfs_println(MDBFS_PRINT_LEVEL_DEBUG, fmt, ##__VA_ARGS__)

/**
 * Print an informative message.
 *
 * @param fmt [in] String format, followed by the content.
 */
#define mdbfs_info(fmt, ...) mdbfs_println(MDBFS_PRINT_LEVEL_INFO, fmt, ##__VA_ARGS__)

/**
 * Print a warning message.
 *
 * @param fmt [in] String format, followed by the content.
 */
#define mdbfs_warning(fmt, ...) mdbfs_println(MDBFS_PRINT_LEVEL_WARNING, fmt, ##__VA_ARGS__)

/**
 * Print an error message.
 *
 * @param fmt [in] String format, followed by the content.
 */
#define mdbfs_error(fmt, ...) mdbfs_println(MDBFS_PRINT_LEVEL_ERROR, fmt, ##__VA_ARGS__)

/**
 * Print a fatal error message, and abort the program.
 *
 * @param fmt [in] String format, followed by the content.
 */
#define mdbfs_fatal(fmt, ...) mdbfs_println(MDBFS_PRINT_LEVEL_ERROR | MDBFS_PRINT_LEVEL_STOP, fmt, ##__VA_ARGS)

#endif
