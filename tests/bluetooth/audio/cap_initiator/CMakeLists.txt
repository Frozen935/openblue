# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

openblue_add_native_test(test_bt_audio_cap_initiator
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
                         "bluetooth/audio/cap_initiator"
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_common.c"
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_unicast_group.c"
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_unicast_start.c"
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_unicast_stop.c"
                         "${CMAKE_CURRENT_SOURCE_DIR}/uut/bap_unicast_client.c"
                         "${CMAKE_CURRENT_SOURCE_DIR}/uut/cap_initiator.c"
                         "${CMAKE_CURRENT_SOURCE_DIR}/uut/csip.c"
                         "${CMAKE_SOURCE_DIR}/tests/bluetooth/audio/mocks/src/conn.c"
                         "${CMAKE_SOURCE_DIR}/tests/bluetooth/audio/mocks/src/gatt.c"
                         "${CMAKE_SOURCE_DIR}/tests/bluetooth/audio/mocks/src/iso.c")

target_include_directories(test_bt_audio_cap_initiator PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/tests/bluetooth/audio/mocks/include
)
