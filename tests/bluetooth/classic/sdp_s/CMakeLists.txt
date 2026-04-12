# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

openblue_add_native_test(test_bt_classic_sdp_s
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
                         "bluetooth/classic/sdp_s"
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/sdp_server.c")

target_sources(test_bt_classic_sdp_s PRIVATE
  ${CMAKE_SOURCE_DIR}/bluetooth/common/bt_shell_private.c
  ${CMAKE_SOURCE_DIR}/tests/bluetooth/classic/classic_shell_host.c
)
