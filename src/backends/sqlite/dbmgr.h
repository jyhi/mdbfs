/**
 * @file dbmgr.h
 *
 * Definition for public interfaces of the SQLite database manager for MDBFS
 * SQLite backend.
 */

#ifndef MDBFS_BACKENDS_SQLITE_DBMGR_H
#define MDBFS_BACKENDS_SQLITE_DBMGR_H

#include <stdint.h>

/* TODO: Documentation */

int mdbfs_backend_sqlite_open_database_from_file(const char *path);
void mdbfs_backend_sqlite_close_database(void);

char *mdbfs_backend_sqlite_get_database_name(void);
char **mdbfs_backend_sqlite_get_table_names(void);
char **mdbfs_backend_sqlite_get_column_names(const char *table_name, const char *row_name);
char **mdbfs_backend_sqlite_get_row_names(const char *table_name);

uint8_t *mdbfs_backend_sqlite_get_cell(size_t *cell_length, const char *table_name, const char *row_name, const char *col_name);
size_t mdbfs_backend_sqlite_get_cell_length(const char *table_name, const char *row_name, const char *col_name);
int mdbfs_backend_sqlite_set_cell(const uint8_t *content, const size_t content_length, const char *table_name, const char *row_name, const char *col_name);

int mdbfs_backend_sqlite_rename_table(const char *table_old, const char *table_new);
int mdbfs_backend_sqlite_rename_column(const char *table_name, const char *column_old, const char *column_new);
int mdbfs_backend_sqlite_rename_row(const char *table_name, const char *row_old, const char *row_new);

int mdbfs_backend_sqlite_create_table(const char *table_new);
int mdbfs_backend_sqlite_create_column(const char *table_name, const char *column_new);
int mdbfs_backend_sqlite_create_row(const char *table_name, const char *row_new);

int mdbfs_backend_sqlite_remove_table(const char *table_name);
int mdbfs_backend_sqlite_remove_column(const char *table_name, const char *column_name);
int mdbfs_backend_sqlite_remove_row(const char *table_name, const char *row_name);

#endif
