option(BUILD_SQLITE3 "Enable the SQLite3 database backend" ON)
option(BUILD_BERKELEY_DB "Enable the Berkeley DB database backend" ON)

# Target
add_library(mdbfs-backends INTERFACE)

# Backends
if(BUILD_SQLITE3)
  add_subdirectory(sqlite)
  target_compile_definitions(mdbfs-backends INTERFACE BUILD_SQLITE3)
  target_link_libraries(mdbfs-backends INTERFACE mdbfs-sqlite)
endif()

if(BUILD_BERKELEY_DB)
  add_subdirectory(berkeleydb)
  target_compile_definitions(mdbfs-backends INTERFACE BUILD_BERKELEY_DB)
  target_link_libraries(mdbfs-backends INTERFACE mdbfs-berkeleydb)
endif()
