# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

openblue_add_native_test(test_bt_classic_l2cap_c
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
                         "bluetooth/classic/l2cap_c"
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/l2cap_test.c")

target_sources(test_bt_classic_l2cap_c PRIVATE
  ${CMAKE_SOURCE_DIR}/bluetooth/common/bt_shell_private.c
  ${CMAKE_SOURCE_DIR}/tests/bluetooth/classic/classic_shell_host.c
)
