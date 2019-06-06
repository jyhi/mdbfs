# Source code to be built
set(
  SRCS
  mdbfs-sqlite.c
)

# Targets
add_library(mdbfs-sqlite SHARED ${SRCS})
install(TARGETS mdbfs-sqlite)

# Link against utils
target_include_directories(mdbfs-sqlite PRIVATE mdbfs-utils)
target_link_libraries(mdbfs-sqlite PRIVATE mdbfs-utils)

# Link against FUSE
target_compile_definitions(mdbfs-sqlite PRIVATE ${FUSE_DEFINITIONS})
target_include_directories(mdbfs-sqlite PRIVATE ${FUSE_INCLUDE_DIR})
target_link_libraries(mdbfs-sqlite PRIVATE ${FUSE_LIBRARY})
