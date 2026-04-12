# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

openblue_add_native_test(test_bt_audio_ascs
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
                         "bluetooth/audio/ascs"
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_ase_control_params.c"
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_ase_register.c"
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_ase_state_transition_invalid.c"
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_ase_state_transition.c"
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_common.c")

target_sources(test_bt_audio_ascs PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/uut/bap_unicast_client.c
  ${CMAKE_CURRENT_SOURCE_DIR}/uut/bap_unicast_server.c
  ${CMAKE_SOURCE_DIR}/tests/bluetooth/audio/mocks/src/bap_stream.c
  ${CMAKE_SOURCE_DIR}/tests/bluetooth/audio/mocks/src/conn.c
  ${CMAKE_SOURCE_DIR}/tests/bluetooth/audio/mocks/src/gatt.c
  ${CMAKE_SOURCE_DIR}/tests/bluetooth/audio/mocks/src/iso.c
  ${CMAKE_SOURCE_DIR}/tests/bluetooth/audio/mocks/src/pacs.c
)

target_include_directories(test_bt_audio_ascs PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/tests/bluetooth/audio/cap_initiator/include
  ${CMAKE_SOURCE_DIR}/tests/bluetooth/audio/mocks/include
)
