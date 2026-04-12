# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)
set(NO_QEMU_SERIAL_BT_SERVER 1)

openblue_add_native_test(test_bt_classic_smp_key_persist
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
                         "bluetooth/classic/smp_key_persist"
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/smp_reboot.c")

target_sources(test_bt_classic_smp_key_persist PRIVATE
  ${CMAKE_SOURCE_DIR}/bluetooth/common/bt_shell_private.c
  ${CMAKE_SOURCE_DIR}/tests/bluetooth/classic/classic_shell_host.c
)
