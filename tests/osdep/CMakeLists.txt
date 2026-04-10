# SPDX-License-Identifier: Apache-2.0

file(GLOB _openblue_osdep_tests CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/test_*.c")

foreach(_src IN LISTS _openblue_osdep_tests)
  get_filename_component(_name "${_src}" NAME_WE)
  openblue_add_native_test(${_name} "${_src}" "osdep")
endforeach()
