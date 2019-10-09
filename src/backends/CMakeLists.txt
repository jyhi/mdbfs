# Sub-directories (backends) to build
add_subdirectory(sqlite)

# Target
add_library(mdbfs-backends INTERFACE)

# Link with backends
target_link_libraries(
  mdbfs-backends
  INTERFACE mdbfs-sqlite
)
