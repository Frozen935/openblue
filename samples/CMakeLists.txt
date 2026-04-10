# SPDX-License-Identifier: Apache-2.0

if(DEFINED CONFIG_OPENBLUE_SAMPLES AND CONFIG_OPENBLUE_SAMPLES)
  add_subdirectory(demo)
endif()

if(DEFINED CONFIG_OPENBLUE_BT_CMD AND CONFIG_OPENBLUE_BT_CMD)
  add_subdirectory(cmds)
endif()
