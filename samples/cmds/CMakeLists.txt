# SPDX-License-Identifier: Apache-2.0

# Bluetooth shell command sample
add_executable(btcmd
  ${CMAKE_SOURCE_DIR}/samples/cmds/bt_cmd_main.c
)

# Link against the aggregated static library and pthread/rt
target_link_libraries(btcmd PRIVATE openblue pthread rt)

# Include dirs similar to openblue target
target_include_directories(btcmd PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/bluetooth
  ${CMAKE_SOURCE_DIR}/base
  ${CMAKE_SOURCE_DIR}/osdep/posix
  ${CMAKE_SOURCE_DIR}/shim/include
)

# Place output under build/samples/cmds/
set_target_properties(btcmd PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/samples/cmds
)
# Also force-include shim umbrella for sample
target_compile_options(btcmd PRIVATE -include ${CMAKE_SOURCE_DIR}/shim/include/preinclude.h)
