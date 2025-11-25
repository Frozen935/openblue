# Copyright (c) 2024 Croxel, Inc.
# SPDX-License-Identifier: Apache-2.0

openblue_library()

openblue_library_include_directories(.)
openblue_library_sources(
	nus.c
	nus_inst.c
)

openblue_library_sources_ifdef(CONFIG_BT_NORDIC_NUS_AUTO_START_BLUETOOTH
	bt_nus_auto_start_bt.c
)
