# SPDX-License-Identifier: Apache-2.0

include(CTest)
enable_testing()

find_package(Threads REQUIRED)
find_path(CMOCKA_INCLUDE_DIR NAMES cmocka.h)
find_library(CMOCKA_LIBRARY NAMES cmocka)

if(NOT CMOCKA_INCLUDE_DIR OR NOT CMOCKA_LIBRARY)
  message(WARNING "cmocka not found; skipping native tests under tests/base and tests/osdep")
  return()
endif()

function(openblue_add_native_test target source rel_output_dir)
  add_executable(${target} ${source})
  target_link_libraries(${target} PRIVATE openblue Threads::Threads ${CMOCKA_LIBRARY} rt)
  target_include_directories(${target} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/bluetooth
    ${CMAKE_SOURCE_DIR}/base
    ${CMAKE_SOURCE_DIR}/osdep/posix
    ${CMAKE_SOURCE_DIR}/shim/include
    ${CMOCKA_INCLUDE_DIR}
  )
  target_compile_options(${target} PRIVATE -include ${CMAKE_SOURCE_DIR}/shim/include/preinclude.h)
  set_target_properties(${target} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests/${rel_output_dir}
  )
  add_test(NAME ${target} COMMAND $<TARGET_FILE:${target}>)
endfunction()

add_subdirectory(base)
add_subdirectory(osdep)

# `tests/bluetooth` 暂不纳入 openblue 原生 CMake 层级。
