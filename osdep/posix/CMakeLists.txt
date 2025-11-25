# SPDX-License-Identifier: Apache-2.0

# POSIX OS adaptation (Linux)
openblue_library()

openblue_library_sources(
  os.c
)

# Expose local include directory for os.h
openblue_library_include_directories(${CMAKE_CURRENT_SOURCE_DIR})
