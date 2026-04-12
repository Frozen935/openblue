# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

openblue_add_native_test(test_bt_mesh_net
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
                         "bluetooth/mesh/net")

target_include_directories(test_bt_mesh_net PRIVATE
  ${CMAKE_SOURCE_DIR}/bluetooth/mesh
)
