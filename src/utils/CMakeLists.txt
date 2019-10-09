# Source code to be built
set(
  SRCS
  memory.c
  path.cxx
  print.c
)

add_library(mdbfs-utils ${SRCS})

# The CXX libraries can be statically compiled to reduce dependencies
if(STATIC_LIBGCC)
  target_link_options(mdbfs-utils INTERFACE -static-libgcc)
endif()

if(STATIC_LIBSTDCXX)
  target_link_options(mdbfs-utils INTERFACE -static-libstdc++)
endif()
