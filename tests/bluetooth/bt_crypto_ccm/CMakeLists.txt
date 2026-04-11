# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

openblue_add_native_test(test_bt_crypto_ccm
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_bt_crypto_ccm.c"
                         "bluetooth/bt_crypto_ccm")
