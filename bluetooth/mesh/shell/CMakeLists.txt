# SPDX-License-Identifier: Apache-2.0

stack_library()

stack_library_sources(
	shell.c
	utils.c
	)

stack_library_sources_ifdef(CONFIG_BT_MESH_SHELL_HEALTH_CLI health.c)

stack_library_sources_ifdef(CONFIG_BT_MESH_SHELL_CFG_CLI cfg.c)

stack_library_sources_ifdef(CONFIG_BT_MESH_SHELL_LARGE_COMP_DATA_CLI large_comp_data.c)

stack_library_sources_ifdef(CONFIG_BT_MESH_SHELL_OP_AGG_CLI op_agg.c)

stack_library_sources_ifdef(CONFIG_BT_MESH_SHELL_PRIV_BEACON_CLI priv_beacon.c)

stack_library_sources_ifdef(CONFIG_BT_MESH_SHELL_RPR_CLI rpr.c)

stack_library_sources_ifdef(CONFIG_BT_MESH_SHELL_SAR_CFG_CLI sar.c)

if(CONFIG_BT_MESH_SHELL_BLOB_CLI OR CONFIG_BT_MESH_SHELL_BLOB_SRV OR
	CONFIG_BT_MESH_SHELL_BLOB_IO_FLASH)
	stack_library_sources(blob.c)
endif()

if(CONFIG_BT_MESH_SHELL_DFU_SLOT OR CONFIG_BT_MESH_SHELL_DFU_METADATA OR
	CONFIG_BT_MESH_SHELL_DFU_CLI OR CONFIG_BT_MESH_SHELL_DFU_SRV)
	stack_library_sources(dfu.c)
endif()

stack_library_sources_ifdef(CONFIG_BT_MESH_SHELL_DFD_SRV dfd.c)

stack_library_sources_ifdef(CONFIG_BT_MESH_OD_PRIV_PROXY_CLI od_priv_proxy.c)

stack_library_sources_ifdef(CONFIG_BT_MESH_SOL_PDU_RPL_CLI sol_pdu_rpl.c)

stack_library_sources_ifdef(CONFIG_BT_MESH_SHELL_BRG_CFG_CLI brg_cfg.c)
