# SPDX-License-Identifier: Apache-2.0
stack_library()
stack_library_sources_ifdef(CONFIG_BT_BAS bas.c)
stack_library_sources_ifdef(CONFIG_BT_BAS_BLS bas_bls.c)
stack_library_sources_ifdef(CONFIG_BT_BAS_BCS bas_bcs.c)
