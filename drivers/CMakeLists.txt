# SPDX-License-Identifier: Apache-2.0

# Bluetooth drivers selection
openblue_library()

# Common Linux socket HCI support (for native/userchan)
openblue_library_sources_ifdef(CONFIG_OPENBLUE_BT_DRIVER hci_sock.c)

# User Channel driver
openblue_library_sources_ifdef(CONFIG_OPENBLUE_BT_DRIVER_TYPE_USERCHAN userchan.c)

# H4 driver (typically for NuttX/serial transport)
openblue_library_sources_ifdef(CONFIG_OPENBLUE_BT_DRIVER_TYPE_H4 h4.c)
