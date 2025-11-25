# SPDX-License-Identifier: Apache-2.0

openblue_add_subdirectory_ifdef(CONFIG_BT_SHELL shell)

openblue_library()
openblue_library_link_libraries(openblue__bluetooth)

openblue_library_sources_ifdef(CONFIG_BT_A2DP             a2dp.c a2dp_codec_sbc.c)
openblue_library_sources_ifdef(CONFIG_BT_AVDTP            avdtp.c)
openblue_library_sources_ifdef(CONFIG_BT_AVRCP            avrcp.c)
openblue_library_sources_ifdef(CONFIG_BT_AVCTP            avctp.c)
openblue_library_sources_ifdef(CONFIG_BT_RFCOMM           rfcomm.c)

openblue_library_sources_ifdef(
  CONFIG_BT_CLASSIC
  br.c
  conn_br.c
  keys_br.c
  l2cap_br.c
  sdp.c
  ssp.c
  sco.c
  )

openblue_library_sources_ifdef(
  CONFIG_BT_HFP_HF
  hfp_hf.c
  at.c
  )

openblue_library_sources_ifdef(
  CONFIG_BT_HFP_AG
  hfp_ag.c
  )

openblue_library_sources_ifdef(
  CONFIG_BT_GOEP
  obex.c
  goep.c
  )

openblue_library_sources_ifdef(
  CONFIG_BT_DID
  did.c
  )
