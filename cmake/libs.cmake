# CMake helper macros/functions for libraries
# ==============================================================================
# Prepare Python dependency for mbedTLS
function(openblue_prepare_mbedtls)
# Map OpenBlue crypto selection to Zephyr-style CONFIG_MBEDTLS expected by bluetooth tree
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
endfunction()