# SPDX-License-Identifier: Apache-2.0

openblue_library()
openblue_library_sources(bt.c)
openblue_library_sources_ifdef(CONFIG_BT_CONN gatt.c)
openblue_library_sources_ifdef(CONFIG_BT_L2CAP_DYNAMIC_CHANNEL l2cap.c)
openblue_library_sources_ifdef(CONFIG_BT_ISO iso.c)
openblue_library_sources_ifdef(CONFIG_BT_CHANNEL_SOUNDING cs.c)
