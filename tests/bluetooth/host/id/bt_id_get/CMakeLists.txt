openblue_add_native_test(test_bt_id_get
                         "${CMAKE_CURRENT_SOURCE_DIR}/src/main.c"
                         "bluetooth/host/id")
target_include_directories(test_bt_id_get PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)
