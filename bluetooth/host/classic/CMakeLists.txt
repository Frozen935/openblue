# SPDX-License-Identifier: Apache-2.0

stack_add_subdirectory_ifdef(CONFIG_BT_SHELL shell)

stack_library()
stack_library_link_libraries(subsys__bluetooth)

stack_library_sources_ifdef(CONFIG_BT_A2DP             a2dp.c a2dp_codec_sbc.c)
stack_library_sources_ifdef(CONFIG_BT_AVDTP            avdtp.c)
stack_library_sources_ifdef(CONFIG_BT_AVRCP            avrcp.c)
stack_library_sources_ifdef(CONFIG_BT_AVCTP            avctp.c)
stack_library_sources_ifdef(CONFIG_BT_RFCOMM           rfcomm.c)

stack_library_sources_ifdef(
  CONFIG_BT_CLASSIC
  br.c
  conn_br.c
  keys_br.c
  l2cap_br.c
  sdp.c
  ssp.c
  sco.c
  )

stack_library_sources_ifdef(
  CONFIG_BT_HFP_HF
  hfp_hf.c
  at.c
  )

stack_library_sources_ifdef(
  CONFIG_BT_HFP_AG
  hfp_ag.c
  )

stack_library_sources_ifdef(
  CONFIG_BT_GOEP
  obex.c
  goep.c
  )

stack_library_sources_ifdef(
  CONFIG_BT_DID
  did.c
  )
