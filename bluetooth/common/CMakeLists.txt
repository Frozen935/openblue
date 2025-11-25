# SPDX-License-Identifier: Apache-2.0

openblue_library()

openblue_library_sources(addr.c)
openblue_library_sources(dummy.c)
openblue_library_sources(bt_str.c)

openblue_library_sources_ifdef(CONFIG_BT_RPA   rpa.c)
openblue_library_sources_ifdef(CONFIG_BT_PRIVATE_SHELL bt_shell_private.c)
openblue_library_sources_ifdef(CONFIG_BT_SHELL bt_shell_private.c)

openblue_library_link_libraries(openblue__bluetooth)
