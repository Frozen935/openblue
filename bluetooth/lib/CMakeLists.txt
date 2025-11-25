# SPDX-License-Identifier: Apache-2.0

openblue_library()

openblue_library_sources_ifdef(CONFIG_BT_EAD ead.c)
