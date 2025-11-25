# SPDX-License-Identifier: Apache-2.0
openblue_library()
openblue_library_sources_ifdef(CONFIG_BT_BAS bas.c)
openblue_library_sources_ifdef(CONFIG_BT_BAS_BLS bas_bls.c)
openblue_library_sources_ifdef(CONFIG_BT_BAS_BCS bas_bcs.c)
