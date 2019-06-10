/**
 * @file configmgr_private.h
 *
 * Private config manager definitions.
 */

#ifndef MDBFS_CONFIGMGR_PRIVATE_H
#define MDBFS_CONFIGMGR_PRIVATE_H

/**
 * Define a key-value store in config manager.
 *
 * This macro is designed to be used in the config manager header for public
 * use. Two definitions of functions are defined:
 *
 * - `mdbfs_configmgr_<name>_load` for loading value from the config manager
 * - `mdbfs_configmgr_<name>_store` for storing value to the config manager
 *
 * It should be noted that the config manager only copies by value, so for
 * pointer values it does not own the data. The caller is responsible for
 * keeping the memory being pointed to valid (e.g. store the content into heap
 * first), and freeing the value when it is not needed.
 */
#define MDBFS_CONFIGMGR_DEFINE_STORE(name, type) \
  type mdbfs_configmgr_##name##_load(void); \
  void mdbfs_configmgr_##name##_store(const type name);

/**
 * Implement a key-value store in config manager.
 *
 * This macro is designed to be usd in the config manager code for
 * implementation of the store defined with MDBFS_CONFIGMGR_DEFINE_STORE. The
 * macro will expand to the following elements:
 *
 * - `static <type> _<name>` for local storage
 * - `<type> mdbfs_configmgr_<name>_load` implementation
 * - `<type> mdbfs_configmgr_<name>_store` implementation
 */
#define MDBFS_CONFIGMGR_IMPLEMENT_STORE(name, type) \
  static type _##name = NULL; \
  type mdbfs_configmgr_##name##_load(void) \
  { \
    return _##name; \
  } \
  void mdbfs_configmgr_##name##_store(type name) \
  { \
    _##name = name; \
  }

#endif
