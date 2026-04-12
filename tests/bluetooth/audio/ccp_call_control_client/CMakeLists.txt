# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

openblue_add_native_test(test_bt_audio_ccp_call_control_client
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
                         "bluetooth/audio/ccp_call_control_client"
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_common.c"
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_procedures.c"
                         "${CMAKE_CURRENT_SOURCE_DIR}/uut/tbs_client.c")

target_include_directories(test_bt_audio_ccp_call_control_client PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)
