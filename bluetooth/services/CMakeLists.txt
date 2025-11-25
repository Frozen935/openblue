# SPDX-License-Identifier: Apache-2.0

openblue_library()

openblue_library_sources_ifdef(CONFIG_BT_DIS dis.c)
openblue_library_sources_ifdef(CONFIG_BT_CTS cts.c)
openblue_library_sources_ifdef(CONFIG_BT_HRS hrs.c)
openblue_library_sources_ifdef(CONFIG_BT_TPS tps.c)

if(CONFIG_BT_BAS)
  add_subdirectory(bas)
endif()

if(CONFIG_BT_OTS OR CONFIG_BT_OTS_CLIENT)
  add_subdirectory(ots)
endif()

if(CONFIG_BT_IAS OR CONFIG_BT_IAS_CLIENT)
  add_subdirectory(ias)
endif()

if(CONFIG_BT_NORDIC_NUS)
  add_subdirectory(nus)
endif()
