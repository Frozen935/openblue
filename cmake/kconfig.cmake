# ==============================================================================
# CMake helper macros/functions for Kconfig
# ==============================================================================

# Prepare Python dependency for Kconfig (kconfiglib)
function(openblue_prepare_kconfig_dependency)
  if(Python3_FOUND)
    set(CHECK_KCONFIG_PY "${CMAKE_CURRENT_BINARY_DIR}/check_kconfig.py")

    file(WRITE "${CHECK_KCONFIG_PY}" [=[import importlib.util
import subprocess
import sys

if importlib.util.find_spec("kconfiglib") is None:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "--user", "kconfiglib"])
]=])

    add_custom_target(kconfig-deps
      COMMAND ${Python3_EXECUTABLE} ${CHECK_KCONFIG_PY}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Prepare Python dependency: kconfiglib"
      VERBATIM
    )
  endif()
endfunction()
