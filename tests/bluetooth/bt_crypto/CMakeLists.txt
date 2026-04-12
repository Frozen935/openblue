# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

openblue_add_native_test(test_bt_crypto
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_bt_crypto.c"
                         "bluetooth/bt_crypto")
target_include_directories(test_bt_crypto PRIVATE
  ${CMAKE_SOURCE_DIR}/bluetooth/crypto
)
