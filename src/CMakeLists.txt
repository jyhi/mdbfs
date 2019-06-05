# Source code to be built
set(
  SRCS
  main.c
)

# Targets
add_executable(mdbfs ${SRCS})

# Targets for installation
install(TARGETS mdbfs RUNTIME)

# API Documentation
if(BUILD_DOCUMENTATION)
  set(DOXYGEN_JAVADOC_AUTOBRIEF true)
  set(DOXYGEN_QT_AUTOBRIEF true)
  set(DOXYGEN_TAB_SIZE "2")
  set(DOXYGEN_OPTIMIZE_OUTPUT_FOR_C true)
  set(DOXYGEN_VERBATIM_HEADERS false)

  set(DOXYGEN_GENERATE_HTML YES)

  doxygen_add_docs(
    docs
    COMMENT "Build API documentation using Doxygen"
  )
endif(BUILD_DOCUMENTATION)
