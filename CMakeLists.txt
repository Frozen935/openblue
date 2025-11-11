cmake_minimum_required(VERSION 3.16)
project(stack C)

# Include helper macros
include(${CMAKE_SOURCE_DIR}/cmake/stack_helpers.cmake)

# Load Kconfig selections from .config if present
stack_load_kconfig(${CMAKE_SOURCE_DIR}/.config)

# Top-level interface target to aggregate Bluetooth subsystem
add_library(stack_interface INTERFACE)

# Provide include dir for consumers
target_include_directories(stack_interface INTERFACE ${CMAKE_SOURCE_DIR}/bluetooth)
# Add shim include dir and pre-include unified header to minimize source changes
target_include_directories(stack_interface INTERFACE ${CMAKE_SOURCE_DIR}/shim/include)
# Pre-include shim umbrella header for all compilation units
add_compile_options(-include ${CMAKE_SOURCE_DIR}/shim/include/shim.h)
# Enable classic/BREDR path for syntax builds

# Add Bluetooth subsystem
add_subdirectory(bluetooth)
# Add shim
add_subdirectory(shim)
# Make shim visible to consumers of stack_interface
# (INTERFACE link to propagate include path; functions are no-op, but compile units see prototypes)
target_link_libraries(stack_interface INTERFACE shim)
