# SPDX-License-Identifier: Apache-2.0

# Sample demo executable
add_executable(demo
  ${CMAKE_SOURCE_DIR}/samples/demo/main.c
)

# Link against the aggregated static library and pthread/rt
target_link_libraries(demo PRIVATE openblue pthread rt)

# Include dirs similar to openblue target
target_include_directories(demo PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/bluetooth
  ${CMAKE_SOURCE_DIR}/base
  ${CMAKE_SOURCE_DIR}/osdep/posix
  ${CMAKE_SOURCE_DIR}/shim/include
)

# Place output under build/samples/demo/
set_target_properties(demo PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/samples/demo
)
# Also force-include shim umbrella for sample
target_compile_options(demo PRIVATE -include ${CMAKE_SOURCE_DIR}/shim/include/preinclude.h)
