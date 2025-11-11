# SPDX-License-Identifier: Apache-2.0

stack_library()

stack_library_sources_ifdef(CONFIG_BT_EAD ead.c)
