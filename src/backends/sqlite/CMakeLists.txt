# Dependencies; pthread and dl are required by SQLite3
find_package(SQLite3 3.28 REQUIRED)

# Source code to be built
set(
  SRCS
  mdbfs.c
  fuseops.c
  dbmgr.c
)

# Targets
add_library(mdbfs-sqlite ${SRCS})

# Link against utils
target_include_directories(mdbfs-sqlite PRIVATE mdbfs-utils)
target_link_libraries(mdbfs-sqlite PRIVATE mdbfs-utils)

# Link against SQLite
target_include_directories(mdbfs-sqlite PRIVATE ${SQLite3_INCLUDE_DIRS})
target_link_libraries(mdbfs-sqlite PRIVATE SQLite::SQLite3)

# Include FUSE development files (headers etc.)
target_compile_definitions(mdbfs-sqlite PRIVATE ${FUSE_DEFINITIONS})
target_include_directories(mdbfs-sqlite PRIVATE ${FUSE_INCLUDE_DIRS})
