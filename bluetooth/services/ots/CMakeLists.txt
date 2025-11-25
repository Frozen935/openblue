# SPDX-License-Identifier: Apache-2.0

openblue_library()

openblue_library_sources_ifdef(
  CONFIG_BT_OTS
  ots.c
  ots_l2cap.c
  ots_obj_manager.c
  ots_oacp.c
  ots_olcp.c)

  openblue_library_sources_ifdef(CONFIG_BT_OTS_DIR_LIST_OBJ ots_dir_list.c)

openblue_library_sources_ifdef(
  CONFIG_BT_OTS_CLIENT
  ots_client.c
  ots_l2cap.c)
