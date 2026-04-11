# ##############################################################################
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

if(CONFIG_LIB_OPENBLUE)

  set(OPENBLUE_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/openblue)
  set(OPENBLUE_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/openblue)
  set(OPENBLUE_INCLUDE_DIRS
      ${OPENBLUE_SOURCE_DIR}
      ${OPENBLUE_SOURCE_DIR}/include
      ${OPENBLUE_SOURCE_DIR}/bluetooth
      ${OPENBLUE_SOURCE_DIR}/base
      ${OPENBLUE_SOURCE_DIR}/osdep/posix
      ${OPENBLUE_SOURCE_DIR}/shim/include)
  set(OPENBLUE_COMMON_FLAGS -include ${OPENBLUE_SOURCE_DIR}/shim/include/preinclude.h)

  list(APPEND CMAKE_MODULE_PATH ${OPENBLUE_SOURCE_DIR}/cmake)
  include(${OPENBLUE_SOURCE_DIR}/cmake/helpers.cmake)

  if(CONFIG_OPENBLUE_CRYPTO_USE_MBEDTLS)
    if(NOT TARGET mbedtls)
      message(FATAL_ERROR
              "CONFIG_LIB_OPENBLUE requires the in-tree mbedtls target. Enable apps/crypto/mbedtls first.")
    endif()

    add_library(mbedTLS INTERFACE)
    target_link_libraries(mbedTLS INTERFACE mbedtls)
    target_include_directories(
      mbedTLS INTERFACE ${NUTTX_APPS_DIR}/crypto/mbedtls/include
                         ${NUTTX_APPS_DIR}/crypto/mbedtls/mbedtls/include)
  endif()

  add_subdirectory(${OPENBLUE_SOURCE_DIR}/shim ${OPENBLUE_BINARY_DIR}/shim)
  add_subdirectory(${OPENBLUE_SOURCE_DIR}/bluetooth ${OPENBLUE_BINARY_DIR}/bluetooth)
  add_subdirectory(${OPENBLUE_SOURCE_DIR}/base ${OPENBLUE_BINARY_DIR}/base)
  add_subdirectory(${OPENBLUE_SOURCE_DIR}/core ${OPENBLUE_BINARY_DIR}/core)
  add_subdirectory(${OPENBLUE_SOURCE_DIR}/drivers ${OPENBLUE_BINARY_DIR}/drivers)
  add_subdirectory(${OPENBLUE_SOURCE_DIR}/osdep/posix ${OPENBLUE_BINARY_DIR}/osdep/posix)

  nuttx_add_library(openblue STATIC)
  target_compile_definitions(openblue PRIVATE _GNU_SOURCE)
  target_compile_options(openblue PRIVATE ${OPENBLUE_COMMON_FLAGS})
  target_include_directories(openblue PRIVATE ${OPENBLUE_INCLUDE_DIRS})

  foreach(_t IN ITEMS openblue__bluetooth openblue_obj_base openblue_obj_core
                      openblue_obj_drivers openblue_obj_osdep_posix)
    if(TARGET ${_t})
      target_link_libraries(openblue PRIVATE ${_t})
    endif()
  endforeach()

  if(CONFIG_OPENBLUE_APP_BTCMD)
    nuttx_add_application(
      MODULE
      ${CONFIG_OPENBLUE_APP_BTCMD}
      NAME
      btcmd
      STACKSIZE
      ${CONFIG_OPENBLUE_APP_BTCMD_STACKSIZE}
      PRIORITY
      100
      SRCS
      ${OPENBLUE_SOURCE_DIR}/samples/cmds/bt_cmd_main.c
      INCLUDE_DIRECTORIES
      ${OPENBLUE_INCLUDE_DIRS}
      COMPILE_FLAGS
      ${OPENBLUE_COMMON_FLAGS}
      DEPENDS
      openblue)
  endif()

  if(CONFIG_OPENBLUE_APP_DEMO)
    nuttx_add_application(
      MODULE
      ${CONFIG_OPENBLUE_APP_DEMO}
      NAME
      demo
      STACKSIZE
      ${CONFIG_OPENBLUE_APP_DEMO_STACKSIZE}
      PRIORITY
      100
      SRCS
      ${OPENBLUE_SOURCE_DIR}/samples/demo/main.c
      INCLUDE_DIRECTORIES
      ${OPENBLUE_INCLUDE_DIRS}
      COMPILE_FLAGS
      ${OPENBLUE_COMMON_FLAGS}
      DEPENDS
      openblue)
  endif()

  if(CONFIG_OPENBLUE_APP_TEST)
    if(NOT TARGET cmocka)
      message(FATAL_ERROR
              "CONFIG_OPENBLUE_APP_TEST requires the in-tree cmocka target. Enable apps/testing/cmocka first.")
    endif()

    nuttx_add_application(
      MODULE
      ${CONFIG_OPENBLUE_APP_TEST}
      NAME
      test
      STACKSIZE
      ${CONFIG_OPENBLUE_APP_TEST_STACKSIZE}
      PRIORITY
      100
      SRCS
      ${OPENBLUE_SOURCE_DIR}/tests/base/test_bt_poll.c
      INCLUDE_DIRECTORIES
      ${OPENBLUE_INCLUDE_DIRS}
      ${NUTTX_APPS_DIR}/testing/cmocka/cmocka/include
      COMPILE_FLAGS
      ${OPENBLUE_COMMON_FLAGS}
      DEPENDS
      openblue
      cmocka)
  endif()

endif()
