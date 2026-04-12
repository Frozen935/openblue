# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

openblue_add_native_test(test_bt_classic_a2dp_sink
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
                         "bluetooth/classic/a2dp_sink"
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/test_a2dp_sink.c")
