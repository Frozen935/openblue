# SPDX-License-Identifier: Apache-2.0

stack_library()
stack_library_sources(bt.c)
stack_library_sources_ifdef(CONFIG_BT_CONN gatt.c)
stack_library_sources_ifdef(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL l2cap.c)
stack_library_sources_ifdef(CONFIG_BT_ISO iso.c)
stack_library_sources_ifdef(CONFIG_BT_CHANNEL_SOUNDING cs.c)
