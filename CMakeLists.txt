####################################################################################
# START 1. Basic setup for cmake
####################################################################################
# basic setup for cmake
cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_INCLUDE_DIRECTORIES_PROJECT_BEFORE ON)
set(CMAKE_COLOR_MAKEFILE ON)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# Disable gnu exentions
set(CMAKE_CXX_EXTENSIONS ON)

# Define the project
project("xGPU" VERSION 1.0.0 LANGUAGES CXX CUDA C)

# XGPU may be built to run using CUDA. Future version may be
# written for HIP or SYCL, which we call the
# Target type. By default, the target is CUDA.
if(DEFINED ENV{XGPU_TARGET})
  set(DEFTARGET $ENV{XGPU_TARGET})
else()
  set(DEFTARGET "CUDA")
endif()

set(VALID_TARGET_TYPES CUDA) #HIP SYCL
set(XGPU_TARGET_TYPE
  "${DEFTARGET}"
  CACHE STRING "Choose the type of target, options are: ${VALID_TARGET_TYPES}")
set_property(CACHE XGPU_TARGET_TYPE PROPERTY STRINGS CUDA)

# CUDA specific part of CMakeLists
#set(CMAKE_CUDA_EXTENSIONS OFF)
find_package(CUDAToolkit REQUIRED)

if(DEFINED ENV{XGPU_GPU_ARCH})
  set(XGPU_DEFAULT_GPU_ARCH $ENV{XGPU_GPU_ARCH})
else()
  set(XGPU_DEFAULT_GPU_ARCH sm_70)
endif()
if(NOT XGPU_GPU_ARCH)
  message(STATUS "Building XGPU for GPU ARCH " "${XGPU_DEFAULT_GPU_ARCH}")
endif()

set(XGPU_GPU_ARCH
  ${XGPU_DEFAULT_GPU_ARCH}
  CACHE STRING "set the GPU architecture (sm_60, sm_70, sm_80 sm_90)")
set_property(CACHE XGPU_GPU_ARCH PROPERTY STRINGS sm_60 sm_70 sm_80 sm_90)
set(XGPU_GPU_ARCH_SUFFIX
  ""
  CACHE STRING "set the GPU architecture suffix (virtual, real). Leave empty for no suffix.")
set_property(CACHE XGPU_GPU_ARCH_SUFFIX PROPERTY STRINGS "real" "virtual" " ")
#set(CMAKE_CUDA_ARCHITECTURES ${XGPU_GPU_ARCH})
mark_as_advanced(XGPU_GPU_ARCH_SUFFIX)
mark_as_advanced(CMAKE_CUDA_ARCHITECTURES)

string(TOUPPER ${XGPU_TARGET_TYPE} CHECK_TARGET_TYPE)
list(FIND VALID_TARGET_TYPES ${CHECK_TARGET_TYPE} TARGET_TYPE_VALID)

if(TARGET_TYPE_VALID LESS 0)
  message(SEND_ERROR "Please specify a valid XGPU_TARGET_TYPE type! Valid target types are:" "${VALID_TARGET_TYPES}")
endif()

# Git
find_package(Git)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} show
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE IS_GIT_REPOSIITORY
    OUTPUT_QUIET ERROR_QUIET)
  if(${IS_GIT_REPOSIITORY} EQUAL 0)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} describe --abbrev=0
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GITTAG
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    # we use git rev-list and pipe that through wc here. Newer git versions support --count as option to rev-list but
    # that might not always be available
    execute_process(
      COMMAND ${GIT_EXECUTABLE} rev-list ${GITTAG}..HEAD
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMAND wc -l
      OUTPUT_VARIABLE GITCOUNT
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(
      COMMAND ${GIT_EXECUTABLE} describe --match 1 --always  --long --dirty
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      OUTPUT_VARIABLE GITVERSION
      OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
endif(GIT_FOUND)

# If FIXED_POINT is defined, the library is compiled to use 8-bit fixed
# point (i.e. integers), otherwise it is compiled to use 32-bit floating
# point (i.e. floats).
option(XGPU_FIXED_POINT "Compile with Fixed point (8-bit) integer arithmetic" OFF)
if(XGPU_FIXED_POINT)
  add_compile_definitions(FIXED_POINT)
endif()

option(XGPU_DP4A "Compile with DP4A mixec prec integer arithmetic" OFF)
if(XGPU_DP4A)
  add_compile_definitions(DP4A)
endif()

option(POWER_LOOP "Compile with POWER_LOOP on for power consumption data" OFF)
option(BENCHMARK "Compile with BENCHMARK on for compute speed data" OFF)


set(XGPU_NPOL "2" CACHE STRING "set the number of polarisations")
set_property(CACHE XGPU_NPOL PROPERTY STRINGS 2)
add_compile_definitions(NPOL=${XGPU_NPOL} XGPU_NPOL)

set(XGPU_NSTATION "32" CACHE STRING "set the number of dish stations")
add_compile_definitions(NSTATION=${XGPU_NSTATION} XGPU_NSTATION)

set(XGPU_NFREQUENCY "32" CACHE STRING "set the number of frequencies")
add_compile_definitions(NFREQUENCY=${XGPU_NFREQUENCY} XGPU_NFREQUENCY)

set(XGPU_NTIME "200" CACHE STRING "set the number of times")
add_compile_definitions(NTIME=${XGPU_NTIME} XGPU_NTIME)

set(XGPU_NTIME_PIPE "100" CACHE STRING "set the number of time pipes")
add_compile_definitions(NTIME_PIPE=${XGPU_NTIME_PIPE} XGPU_NTIME_PIPE)

set(XGPU_TEXTURE_DIM "1" CACHE STRING "set the number of texture_dimensions")
set_property(CACHE XGPU_TEXTURE_DIM PROPERTY STRINGS 1 2)
add_compile_definitions(TEXTURE_DIM=${XGPU_TEXTURE_DIM} XGPU_TEXTURE_DIM)

# Print the configuration details to stdout
message(STATUS "")
message(STATUS "${PROJECT_NAME} ${PROJECT_VERSION} (${GITVERSION}) **")
message(STATUS "cmake version: ${CMAKE_VERSION}")
message(STATUS "Source location: ${CMAKE_SOURCE_DIR}")
message(STATUS "Build location: ${CMAKE_BINARY_DIR}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "XGPU target: ${XGPU_TARGET_TYPE}")
message(STATUS "XGPU FIXED_POINT: ${XGPU_FIXED_POINT}")
message(STATUS "XGPU DP4A: ${XGPU_DP4A}")
message(STATUS "XGPU TEXTURE_DIM: ${XGPU_TEXTURE_DIM}")
message(STATUS "XGPU NPOL: ${XGPU_NPOL}")
message(STATUS "XGPU NSTATION: ${XGPU_NSTATION}")
message(STATUS "XGPU NFREQUENCY: ${XGPU_NFREQUENCY}")
message(STATUS "XGPU NTIME: ${XGPU_NTIME}")
message(STATUS "XGPU NTIME_PIPE: ${XGPU_NTIME_PIPE}")

# Add src, tests
add_subdirectory(src)
#add_subdirectory(tests)
