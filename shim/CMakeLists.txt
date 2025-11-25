# SPDX-License-Identifier: Apache-2.0

# Shim glue layer as an INTERFACE target that provides include path
add_library(shim INTERFACE)

target_include_directories(shim INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
