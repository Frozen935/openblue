# ==============================================================================
# CMake helper macros/functions for Kconfig
# ==============================================================================

# Prepare Python dependency for Kconfig (kconfiglib)
function(openblue_prepare_kconfig_dependency)
  if(Python3_FOUND)
    set(CHECK_KCONFIG_PY "${CMAKE_CURRENT_BINARY_DIR}/check_kconfig.py")

    add_custom_target(kconfig-deps
      COMMAND ${Python3_EXECUTABLE} ${CHECK_KCONFIG_PY}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Prepare Python dependency: kconfiglib"
      VERBATIM
    )
  endif()
endfunction()