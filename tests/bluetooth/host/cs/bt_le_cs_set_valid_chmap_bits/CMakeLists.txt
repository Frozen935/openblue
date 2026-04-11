# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

openblue_add_native_test(test_bt_le_cs_set_valid_chmap_bits
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
                         "bluetooth/host/cs")
target_sources(test_bt_le_cs_set_valid_chmap_bits PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../native_stubs.c
)
