/**
 * @file dbmgr.h
 *
 * Definition for public interfaces of the Berkeley DB database manager for
 * MDBFS Berkeley DB backend.
 */

#ifndef MDBFS_BACKENDS_BERKELEYDB_DBMGR_H
#define MDBFS_BACKENDS_BERKELEYDB_DBMGR_H

#include <stdint.h>

/* TODO: Documentation */

int mdbfs_backend_berkeleydb_open_database_from_file(const char *path);
void mdbfs_backend_berkeleydb_close_database(void);

char *mdbfs_backend_berkeleydb_get_database_name(void);

char **mdbfs_backend_berkeleydb_get_record_keys(void);
uint8_t *mdbfs_backend_berkeleydb_get_record_value(size_t *value_length, const char *key);

int mdbfs_backend_berkeleydb_set_record_value(const char *key, const uint8_t *value, const size_t value_length);

int mdbfs_backend_berkeleydb_rename_record(const char *key_old, const char *key_new);
int mdbfs_backend_berkeleydb_create_record(const char *key_new);
int mdbfs_backend_berkeleydb_remove_record(const char *key);

#endif
