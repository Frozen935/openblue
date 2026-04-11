# SPDX-License-Identifier: Apache-2.0

openblue_add_native_test(test_bt_audio_codec
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
                         "bluetooth/audio/codec")
