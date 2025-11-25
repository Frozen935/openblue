# SPDX-License-Identifier: Apache-2.0

openblue_library()
openblue_library_sources_ifdef(CONFIG_BT_IAS ias.c)
openblue_library_sources_ifdef(CONFIG_BT_IAS_CLIENT ias_client.c)
