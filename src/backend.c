/**
 * @file backend.c
 *
 * Implementation of the MDBFS backend dispatcher.
 */

#include <string.h>
#include "utils/memory.h"
#include "utils/print.h"
#include "backends/list.h"
#include "backend.h"

struct mdbfs_backend *mdbfs_backend_get(const char *name)
{
  struct mdbfs_backend *ret = NULL;

  for (int i = 0; mdbfs_backends[i].name && mdbfs_backends[i].get_mdbfs_backend; i++) {
    if (strcmp(name, mdbfs_backends[i].name) == 0) {
      ret = mdbfs_backends[i].get_mdbfs_backend();
      break;
    }
  }

  return ret;
}

char *mdbfs_backends_get_help(void)
{
  char  *ret = NULL;
  size_t ret_length = 0;

  /* First iterate and get buffer size */
  for (int i = 0; mdbfs_backends[i].name && mdbfs_backends[i].get_mdbfs_backend; i++) {
    struct mdbfs_backend *backend = mdbfs_backends[i].get_mdbfs_backend();

    const char *backend_name = backend->get_name();
    const char *backend_desc = backend->get_description();
    const char *backend_help = backend->get_help();

    /* To support backend alias, we check if the backend has the same name as
     * the one we found in the list.
     */
    if (strcmp(mdbfs_backends[i].name, backend_name) != 0)
      continue;

    ret_length += strlen(backend_name ? backend_name : "unknown");
    ret_length += strlen(" - ");
    ret_length += strlen(backend_desc ? backend_desc : "unknown");
    ret_length += strlen("\n\n");
    ret_length += strlen(backend_help ? backend_help : "There is no help for this backend.");
    ret_length += strlen("\n\n");

    mdbfs_free(backend);
  }

  /* Then fill the buffer */
  ret = mdbfs_malloc0(ret_length + 1); // + 1 NUL

  for (int i = 0; mdbfs_backends[i].name && mdbfs_backends[i].get_mdbfs_backend; i++) {
    struct mdbfs_backend *backend = mdbfs_backends[i].get_mdbfs_backend();

    const char *backend_name = backend->get_name();
    const char *backend_desc = backend->get_description();
    const char *backend_help = backend->get_help();

    /* To support backend alias, we check if the backend has the same name as
     * the one we found in the list.
     */
    if (strcmp(mdbfs_backends[i].name, backend_name) != 0)
      continue;

    strcat(ret, backend_name ? backend_name : "unknown");
    strcat(ret, " - ");
    strcat(ret, backend_desc ? backend_desc : "unknown");
    strcat(ret, "\n\n");
    strcat(ret, backend_help ? backend_help : "There is no help for this backend.");
    strcat(ret, "\n\n");

    mdbfs_free(backend);
  }

  return ret;
}

char *mdbfs_backends_get_version(void)
{
  char  *ret = NULL;
  size_t ret_length = 0;

  /* First iterate and get buffer size */
  for (int i = 0; mdbfs_backends[i].name && mdbfs_backends[i].get_mdbfs_backend; i++) {
    struct mdbfs_backend *backend = mdbfs_backends[i].get_mdbfs_backend();

    const char *backend_name = backend->get_name();
    const char *backend_ver  = backend->get_version();

    /* To support backend alias, we check if the backend has the same name as
     * the one we found in the list.
     */
    if (strcmp(mdbfs_backends[i].name, backend_name) != 0)
      continue;

    ret_length += strlen("Backend ");
    ret_length += strlen(backend_name ? backend_name : "unknown");
    ret_length += strlen(" version ");
    ret_length += strlen(backend_ver ? backend_ver : "unknown");
    ret_length += strlen("\n");

    mdbfs_free(backend);
  }

  /* Then fill the buffer */
  ret = mdbfs_malloc0(ret_length + 1); // + 1 NUL

  for (int i = 0; mdbfs_backends[i].name && mdbfs_backends[i].get_mdbfs_backend; i++) {
    struct mdbfs_backend *backend = mdbfs_backends[i].get_mdbfs_backend();

    const char *backend_name = backend->get_name();
    const char *backend_ver  = backend->get_version();

    /* To support backend alias, we check if the backend has the same name as
     * the one we found in the list.
     */
    if (strcmp(mdbfs_backends[i].name, backend_name) != 0)
      continue;

    strcat(ret, "Backend ");
    strcat(ret, backend_name ? backend_name : "unknown");
    strcat(ret, " version ");
    strcat(ret, backend_ver ? backend_ver : "unknown");
    strcat(ret, "\n");

    mdbfs_free(backend);
  }

  return ret;
}
