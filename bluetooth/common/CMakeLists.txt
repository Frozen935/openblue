# SPDX-License-Identifier: Apache-2.0

stack_library()

stack_library_sources(addr.c)
stack_library_sources(dummy.c)
stack_library_sources(bt_str.c)

stack_library_sources_ifdef(CONFIG_BT_RPA   rpa.c)
stack_library_sources_ifdef(CONFIG_BT_PRIVATE_SHELL bt_shell_private.c)

stack_library_link_libraries(subsys__bluetooth)
