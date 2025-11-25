# SPDX-License-Identifier: Apache-2.0

add_library(openblue__bluetooth INTERFACE)

target_include_directories(openblue__bluetooth INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(common)
openblue_add_subdirectory_ifdef(CONFIG_BT_HCI host)
openblue_add_subdirectory_ifdef(CONFIG_BT_CONN services)
openblue_add_subdirectory_ifdef(CONFIG_BT_MESH mesh)
openblue_add_subdirectory_ifdef(CONFIG_BT_AUDIO audio)
openblue_add_subdirectory_ifdef(CONFIG_BT_CRYPTO crypto)

if(CONFIG_BT_LL_SW_SPLIT)
  add_subdirectory(controller)
endif()

# openblue_include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_subdirectory(lib)

# Aggregate all per-subdir interface targets into openblue__bluetooth
foreach(_t IN ITEMS
  openblue_obj_bluetooth_common
  openblue_obj_bluetooth_host
  openblue_obj_bluetooth_services
  openblue_obj_bluetooth_mesh
  openblue_obj_bluetooth_audio
  openblue_obj_bluetooth_crypto
  openblue_obj_bluetooth_lib
  openblue_obj_bluetooth_host_classic
  openblue_obj_bluetooth_host_shell
  openblue_obj_bluetooth_host_classic_shell
)
  if(TARGET ${_t})
    target_link_libraries(openblue__bluetooth INTERFACE ${_t})
  endif()
endforeach()
