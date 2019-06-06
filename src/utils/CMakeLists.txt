# Source code to be built
set(
  SRCS
  mdbfs-memory.c
)

# Utilities are statically compiled
add_library(mdbfs-utils STATIC ${SRCS})

# ... but with -fPIC
set_property(TARGET mdbfs-utils PROPERTY POSITION_INDEPENDENT_CODE ON)
