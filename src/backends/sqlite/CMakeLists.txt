# Source code to be built
set(
  SRCS
  mdbfs-sqlite.c
)

# Targets
add_library(mdbfs-sqlite SHARED ${SRCS})
install(TARGETS mdbfs-sqlite)

# Additional include paths for common headers
target_include_directories(mdbfs-sqlite PRIVATE ..)

# Link against FUSE
target_compile_definitions(mdbfs-sqlite PRIVATE ${FUSE_DEFINITIONS})
target_include_directories(mdbfs-sqlite PRIVATE ${FUSE_INCLUDE_DIR})
target_link_libraries(mdbfs-sqlite PRIVATE ${FUSE_LIBRARY})
