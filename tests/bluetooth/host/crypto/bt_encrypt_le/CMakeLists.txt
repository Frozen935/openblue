# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(_bt_encrypt_le_mock_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/../mocks/aes.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../mocks/aes_expects.c
)

openblue_add_native_test(test_bt_encrypt_le
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
                         "bluetooth/host/crypto")
target_sources(test_bt_encrypt_le PRIVATE ${_bt_encrypt_le_mock_sources})
target_include_directories(test_bt_encrypt_le PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

openblue_add_native_test(test_bt_encrypt_le_invalid_inputs
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_suite_invalid_inputs.c"
                         "bluetooth/host/crypto")
target_sources(test_bt_encrypt_le_invalid_inputs PRIVATE ${_bt_encrypt_le_mock_sources})
target_include_directories(test_bt_encrypt_le_invalid_inputs PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)
