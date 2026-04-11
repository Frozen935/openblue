# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

set(_bt_rand_mock_sources
  ${CMAKE_CURRENT_SOURCE_DIR}/../mocks/hci_core.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../mocks/hci_core_expects.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../mocks/prng.c
  ${CMAKE_CURRENT_SOURCE_DIR}/../mocks/prng_expects.c
)

openblue_add_native_test(test_bt_rand
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
                         "bluetooth/host/crypto")
target_sources(test_bt_rand PRIVATE ${_bt_rand_mock_sources})
target_include_directories(test_bt_rand PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

openblue_add_native_test(test_bt_rand_invalid_inputs
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_suite_invalid_inputs.c"
                         "bluetooth/host/crypto")
target_sources(test_bt_rand_invalid_inputs PRIVATE ${_bt_rand_mock_sources})
target_include_directories(test_bt_rand_invalid_inputs PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)
