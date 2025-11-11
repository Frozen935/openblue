# SPDX-License-Identifier: Apache-2.0

add_library(subsys__bluetooth INTERFACE)

target_include_directories(subsys__bluetooth INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(common)
stack_add_subdirectory_ifdef(CONFIG_BT_HCI host)
stack_add_subdirectory_ifdef(CONFIG_BT_CONN services)
stack_add_subdirectory_ifdef(CONFIG_BT_MESH mesh)
stack_add_subdirectory_ifdef(CONFIG_BT_AUDIO audio)
stack_add_subdirectory_ifdef(CONFIG_BT_CRYPTO crypto)

if(CONFIG_BT_LL_SW_SPLIT)
  add_subdirectory(controller)
endif()

# stack_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(lib)
