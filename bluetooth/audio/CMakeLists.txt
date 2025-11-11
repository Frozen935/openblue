# SPDX-License-Identifier: Apache-2.0

stack_add_subdirectory_ifdef(CONFIG_BT_SHELL shell)

stack_library()

stack_library_sources(audio.c)

if (CONFIG_BT_VOCS OR CONFIG_BT_VOCS_CLIENT)
	stack_library_sources(vocs.c)
endif()
stack_library_sources_ifdef(CONFIG_BT_VOCS_CLIENT vocs_client.c)

if (CONFIG_BT_AICS OR CONFIG_BT_AICS_CLIENT)
	stack_library_sources(aics.c)
endif()
stack_library_sources_ifdef(CONFIG_BT_AICS_CLIENT aics_client.c)

stack_library_sources_ifdef(CONFIG_BT_VCP_VOL_REND vcp_vol_rend.c)
stack_library_sources_ifdef(CONFIG_BT_VCP_VOL_CTLR vcp_vol_ctlr.c)

if (CONFIG_BT_MICP_MIC_DEV)
	stack_library_sources(micp_mic_dev.c)
endif()
stack_library_sources_ifdef(CONFIG_BT_MICP_MIC_CTLR micp_mic_ctlr.c)

# When CONFIG_BT_AUDIO and CONFIG_BT_CONN is enabled we pull in ccid. It is based on CONFIG_BT_CONN
# as CCID depends on GATT, and GATT is enabled when CONFIG_BT_CONN=y.
stack_library_sources_ifdef(CONFIG_BT_CONN ccid.c)

stack_library_link_libraries(subsys__bluetooth)

stack_library_sources_ifdef(CONFIG_BT_CSIP_SET_MEMBER csip_set_member.c)
stack_library_sources_ifdef(CONFIG_BT_CSIP_SET_COORDINATOR csip_set_coordinator.c)
if (CONFIG_BT_CSIP_SET_MEMBER OR CONFIG_BT_CSIP_SET_COORDINATOR)
	stack_library_sources(csip_crypto.c)
endif()

stack_library_sources_ifdef(CONFIG_BT_TBS tbs.c)

stack_library_sources_ifdef(CONFIG_BT_TBS_CLIENT tbs_client.c)

stack_library_sources_ifdef(CONFIG_BT_MCC mcc.c)

stack_library_sources_ifdef(CONFIG_BT_MCS mcs.c)

stack_library_sources_ifdef(CONFIG_BT_MPL mpl.c)

stack_library_sources_ifdef(CONFIG_MCTL media_proxy.c)

stack_library_sources_ifdef(CONFIG_BT_ASCS ascs.c)
stack_library_sources_ifdef(CONFIG_BT_PACS pacs.c)
stack_library_sources_ifdef(CONFIG_BT_BAP_STREAM bap_stream.c codec.c bap_iso.c)
stack_library_sources_ifdef(CONFIG_BT_BAP_BASE bap_base.c)
stack_library_sources_ifdef(CONFIG_BT_BAP_UNICAST_SERVER bap_unicast_server.c)
stack_library_sources_ifdef(CONFIG_BT_BAP_UNICAST_CLIENT bap_unicast_client.c)
stack_library_sources_ifdef(CONFIG_BT_BAP_BROADCAST_SOURCE bap_broadcast_source.c)
stack_library_sources_ifdef(CONFIG_BT_BAP_BROADCAST_SINK bap_broadcast_sink.c)
stack_library_sources_ifdef(CONFIG_BT_BAP_SCAN_DELEGATOR bap_scan_delegator.c)
stack_library_sources_ifdef(CONFIG_BT_BAP_BROADCAST_ASSISTANT bap_broadcast_assistant.c)
stack_library_sources_ifdef(CONFIG_BT_CCP_CALL_CONTROL_CLIENT ccp_call_control_client.c)
stack_library_sources_ifdef(CONFIG_BT_CCP_CALL_CONTROL_SERVER ccp_call_control_server.c)
stack_library_sources_ifdef(CONFIG_BT_HAS has.c)
stack_library_sources_ifdef(CONFIG_BT_HAS_CLIENT has_client.c)
stack_library_sources_ifdef(CONFIG_BT_CAP cap_stream.c)
stack_library_sources_ifdef(CONFIG_BT_CAP_ACCEPTOR cap_acceptor.c)
stack_library_sources_ifdef(CONFIG_BT_CAP_INITIATOR cap_initiator.c)
stack_library_sources_ifdef(CONFIG_BT_CAP_COMMANDER cap_commander.c)
stack_library_sources_ifdef(CONFIG_BT_CAP_HANDOVER cap_handover.c)
if (CONFIG_BT_CAP_INITIATOR_UNICAST OR CONFIG_BT_CAP_COMMANDER)
	stack_library_sources(cap_common.c)
endif()
stack_library_sources_ifdef(CONFIG_BT_TMAP tmap.c)
stack_library_sources_ifdef(CONFIG_BT_GMAP gmap_client.c gmap_server.c)
stack_library_sources_ifdef(CONFIG_BT_PBP pbp.c)
