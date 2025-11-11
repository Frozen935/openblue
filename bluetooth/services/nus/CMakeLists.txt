# Copyright (c) 2024 Croxel, Inc.
# SPDX-License-Identifier: Apache-2.0

stack_library()

stack_library_include_directories(.)
stack_library_sources(
	nus.c
	nus_inst.c
)

stack_library_sources_ifdef(CONFIG_BT_NORDIC_NUS_AUTO_START_BLUETOOTH
	bt_nus_auto_start_bt.c
)
