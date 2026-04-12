# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

openblue_add_native_test(test_bt_l2cap
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
                         "bluetooth/l2cap")
