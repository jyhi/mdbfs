set(
  SRCS
  configmgr.c
)

add_library(mdbfs-configmgr STATIC ${SRCS})
set_property(TARGET mdbfs-configmgr PROPERTY POSITION_INDEPENDENT_CODE ON)
