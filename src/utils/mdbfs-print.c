#include <stdio.h>
#include <stdarg.h>
#include "mdbfs-print.h"

/**
 * Extract only message level.
 */
#define MDBFS_PRINT_LEVEL_MESSAGE_LEVEL(m) ((m) & 0x0f)

/**
 * Extract only stop bit.
 */
#define MDBFS_PRINT_LEVEL_STOP_BIT(s) ((s) & 0x10)

/* Headers for output. */
static const char * const debug_header   = "** mdbfs: DEBUG: ";
static const char * const info_header    = "** mdbfs: INFO: ";
static const char * const warning_header = "** mdbfs: WARN: ";
static const char * const error_header   = "** mdbfs: FAIL: ";

void mdbfs_println(enum MdbfsPrintLevel level, const char * const fmt, ...)
{
  va_list args;
  va_start(args, fmt);

  int msg_level = MDBFS_PRINT_LEVEL_MESSAGE_LEVEL(level);
  int stop_bit  = MDBFS_PRINT_LEVEL_STOP_BIT(level);

  switch(msg_level) {
    case MDBFS_PRINT_LEVEL_DEBUG:
      fprintf(stderr, "%s", debug_header);
      break;
    case MDBFS_PRINT_LEVEL_INFO:
      fprintf(stderr, "%s", info_header);
      break;
    case MDBFS_PRINT_LEVEL_WARNING:
      fprintf(stderr, "%s", warning_header);
      break;
    case MDBFS_PRINT_LEVEL_ERROR:
      fprintf(stderr, "%s", error_header);
      break;
    default:
      break;
  }

  vfprintf(stderr, fmt, args);
  fprintf(stderr, "\n");

  va_end(args);

  if (stop_bit)
    abort();
}
