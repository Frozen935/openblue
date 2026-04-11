# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

openblue_add_native_test(test_bt_addr_le_eq
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_bt_addr_le_eq.c"
                         "bluetooth/addr")
openblue_add_native_test(test_bt_addr_from_str
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_bt_addr_from_str.c"
                         "bluetooth/addr")
