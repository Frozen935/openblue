cmake_minimum_required(VERSION 3.16)
project(openblue C)

# ==============================================================================
# Add GNU_SOURCE compile option
add_compile_definitions(_GNU_SOURCE)

# ==============================================================================
# Option to enable AddressSanitizer (ASan)
option(ENABLE_ASAN "Enable AddressSanitizer" ON)

if(ENABLE_ASAN)
    message(STATUS "AddressSanitizer (ASan) is enabled.")
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

# ==============================================================================
# Helper macros
include(${CMAKE_SOURCE_DIR}/cmake/helpers.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/kconfig.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/libs.cmake)

# ==============================================================================
# C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)


# Helper macros and modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(helpers)
include(libs)

# ==============================================================================
# Include directories commonly used by sources
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/bluetooth)
include_directories(${CMAKE_SOURCE_DIR}/shim/include)

# ==============================================================================
# Dependencies
# ==============================================================================

# Python3 for Kconfig helpers (menuconfig/genconfig)
find_package(Python3 COMPONENTS Interpreter QUIET)
if(NOT Python3_FOUND)
  message(WARNING "Python3 interpreter not found; Kconfig menuconfig/genconfig helpers will be unavailable. Normal build will continue.")
endif()

# Prepare Kconfig (and install kconfiglib if needed)
openblue_prepare_kconfig_dependency()

# Load Kconfig configuration
openblue_load_kconfig(${CMAKE_SOURCE_DIR}/.config)

# Prepare mbedtls
#openblue_prepare_mbedtls()

if(DEFINED CONFIG_OPENBLUE_CRYPTO_USE_MBEDTLS AND CONFIG_OPENBLUE_CRYPTO_USE_MBEDTLS)
  set(CONFIG_MBEDTLS 1)
  add_compile_definitions(CONFIG_MBEDTLS=1)
endif()

# mbed TLS discovery and vendoring (provides INTERFACE target mbedTLS)
if(DEFINED CONFIG_MBEDTLS AND CONFIG_MBEDTLS)
  find_package(PkgConfig QUIET)
  set(_mbed_found 0)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(MBEDTLS QUIET mbedcrypto)
    if(NOT MBEDTLS_FOUND)
      pkg_check_modules(MBEDTLS QUIET mbedtls)
    endif()
    if(MBEDTLS_FOUND)
      add_library(mbedTLS INTERFACE)
      target_include_directories(mbedTLS INTERFACE ${MBEDTLS_INCLUDE_DIRS})
      target_link_libraries(mbedTLS INTERFACE ${MBEDTLS_LINK_LIBRARIES})
      set(_mbed_found 1)
      message(STATUS "Stack: Using system mbed TLS via pkg-config -> ${MBEDTLS_LINK_LIBRARIES}")
    endif()
  endif()

  if(NOT _mbed_found)
    include(FetchContent)
    message(STATUS "Stack: vendoring mbed TLS v2.28.8 (static libmbedcrypto.a)")
    FetchContent_Declare(mbedtls
      URL https://github.com/Mbed-TLS/mbedtls/archive/refs/tags/v2.28.8.tar.gz
      SOURCE_SUBDIR .
    )
    # Prefer static build, disable tests/programs for faster build
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared mbed TLS libs" FORCE)
    set(ENABLE_TESTING OFF CACHE BOOL "Enable mbed TLS tests" FORCE)
    set(ENABLE_PROGRAMS OFF CACHE BOOL "Enable mbed TLS programs" FORCE)
    FetchContent_MakeAvailable(mbedtls)

    # Create compatibility INTERFACE target for bluetooth tree
    add_library(mbedTLS INTERFACE)
    # mbedcrypto target is provided by vendored mbedtls build
    target_link_libraries(mbedTLS INTERFACE mbedcrypto)
    # Expose headers (FetchContent provides mbedtls_SOURCE_DIR variable)
    # Use generator expression to avoid evaluating before it's defined
    target_include_directories(mbedTLS INTERFACE $<IF:$<BOOL:${mbedtls_SOURCE_DIR}>,${mbedtls_SOURCE_DIR}/include,${CMAKE_BINARY_DIR}/_mbedtls_missing_include>)
  endif()
endif()


# ==============================================================================
# Kconfig support targets
# ==============================================================================

# menuconfig: open interactive UI to edit .config
if(Python3_FOUND)
  add_custom_target(menuconfig
    DEPENDS kconfig-deps
    COMMAND ${CMAKE_COMMAND} -E echo "[Kconfig] Launching menuconfig UI..."
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/kconfig/run_menuconfig.py ${CMAKE_SOURCE_DIR}/Kconfig ${CMAKE_SOURCE_DIR}/.config
    COMMAND ${CMAKE_COMMAND} -E echo "[Kconfig] menuconfig completed. Please re-run cmake -S . -B build to apply the configuration, or directly run cmake --build build --target genconfig."
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "OpenBlue Kconfig menuconfig"
    VERBATIM
  )
else()
  add_custom_target(menuconfig
    COMMAND ${CMAKE_COMMAND} -E echo "[Kconfig] Python3 not found. Cannot run menuconfig UI."
    VERBATIM
  )
endif()

# genconfig: generate ${CMAKE_BINARY_DIR}/include/generated/autoconf.h from .config
if(Python3_FOUND)
  add_custom_target(genconfig
    DEPENDS kconfig-deps
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/include/generated
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/kconfig/genconfig.py --header-output ${CMAKE_BINARY_DIR}/include/generated/autoconf.h ${CMAKE_SOURCE_DIR}/.config
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generate autoconf.h from .config -> ${CMAKE_BINARY_DIR}/include/generated/autoconf.h"
    VERBATIM
  )
else()
  add_custom_target(genconfig
    COMMAND ${CMAKE_COMMAND} -E echo "[Kconfig] Python3 not found. genconfig cannot run."
    VERBATIM
  )
endif()

# ==============================================================================
# Subdirectories and main library
# ==============================================================================

# Add subdirectories (Zephyr-style hierarchical organization)
add_subdirectory(bluetooth)
add_subdirectory(shim)
add_subdirectory(base)
add_subdirectory(core)
add_subdirectory(drivers)
add_subdirectory(osdep/posix)

# Aggregate into a real static library (compile consumer of all INTERFACE sources)
add_library(openblue STATIC)

# Force-include shim umbrella across all compilation units in this target only
# (avoid affecting external vendored libs)
target_compile_options(openblue PRIVATE -include ${CMAKE_SOURCE_DIR}/shim/include/preinclude.h)

# Include dirs for this target
target_include_directories(openblue PRIVATE
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/include
  ${CMAKE_SOURCE_DIR}/bluetooth
  ${CMAKE_SOURCE_DIR}/base
  ${CMAKE_SOURCE_DIR}/osdep/posix
  ${CMAKE_SOURCE_DIR}/shim/include
)

# Link all per-dir interface targets and Bluetooth subsystem into openblue
foreach(_t IN ITEMS
  openblue__bluetooth
  openblue_obj_base
  openblue_obj_core
  openblue_obj_drivers
  openblue_obj_osdep_posix
)
  if(TARGET ${_t})
    target_link_libraries(openblue PRIVATE ${_t})
  endif()
endforeach()

# ==============================================================================
# Samples (executables)
# ==============================================================================
if(DEFINED CONFIG_OPENBLUE_SAMPLES AND CONFIG_OPENBLUE_SAMPLES)
  add_subdirectory(samples/demo)
endif()

if(DEFINED CONFIG_OPENBLUE_BT_CMD AND CONFIG_OPENBLUE_BT_CMD)
  add_subdirectory(samples/cmds)
endif()
