# SPDX-License-Identifier: Apache-2.0

stack_library()
stack_library_sources_ifdef(CONFIG_BT_IAS ias.c)
stack_library_sources_ifdef(CONFIG_BT_IAS_CLIENT ias_client.c)
