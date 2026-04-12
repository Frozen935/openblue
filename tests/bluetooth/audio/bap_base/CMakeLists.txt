# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

openblue_add_native_test(test_bt_audio_bap_base
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
                         "bluetooth/audio/bap_base")
