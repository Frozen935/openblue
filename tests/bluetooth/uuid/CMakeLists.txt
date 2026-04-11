# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

openblue_add_native_test(test_bt_uuid_cmp
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_bt_uuid_cmp.c"
                         "bluetooth/uuid")
openblue_add_native_test(test_bt_uuid_create
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_bt_uuid_create.c"
                         "bluetooth/uuid")
openblue_add_native_test(test_bt_uuid_from_str
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_bt_uuid_from_str.c"
                         "bluetooth/uuid")
openblue_add_native_test(test_bt_uuid_to_str
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_bt_uuid_to_str.c"
                         "bluetooth/uuid")
