cmake_minimum_required(VERSION 3.20)
project(BoilEffect VERSION 1.0.5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

if(NOT OPENFX_ROOT)
  set(OPENFX_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/openfx")
endif()

if(NOT EXISTS "${OPENFX_ROOT}/Support/include")
  message(FATAL_ERROR "OpenFX SDK not found. Clone it:\n  git clone https://github.com/AcademySoftwareFoundation/openfx.git ${OPENFX_ROOT}")
endif()

include_directories(
  ${OPENFX_ROOT}/include
  ${OPENFX_ROOT}/Support/include
  ${OPENFX_ROOT}/Support/Plugins/include
)

set(SOURCES Boilify.cpp)

if(APPLE)
  set(PLUGIN_DIR "MacOS")
  set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
elseif(WIN32)
  set(PLUGIN_DIR "Win64")
else()
  set(PLUGIN_DIR "Linux-x86-64")
endif()

set(PLUGIN_OUTPUT_DIR "${CMAKE_BINARY_DIR}/${PLUGIN_DIR}")

add_library(BoilEffect MODULE ${SOURCES})

set_target_properties(BoilEffect PROPERTIES
  OUTPUT_NAME "Boilify.ofx"
  PREFIX ""
  SUFFIX ""
  ARCHIVE_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIR}"
  LIBRARY_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${PLUGIN_OUTPUT_DIR}"
)

# Multi-config generators (Visual Studio, Xcode) ignore the non-config output dirs.
if(CMAKE_CONFIGURATION_TYPES)
  foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER "${cfg}" cfg_uc)
    set_target_properties(BoilEffect PROPERTIES
      ARCHIVE_OUTPUT_DIRECTORY_${cfg_uc} "${PLUGIN_OUTPUT_DIR}"
      LIBRARY_OUTPUT_DIRECTORY_${cfg_uc} "${PLUGIN_OUTPUT_DIR}"
      RUNTIME_OUTPUT_DIRECTORY_${cfg_uc} "${PLUGIN_OUTPUT_DIR}"
    )
  endforeach()
endif()

if(APPLE)
  target_link_libraries(BoilEffect "-framework OpenGL")
elseif(WIN32)
  # Static runtime to avoid VC++ redistributable dependency
  set_property(TARGET BoilEffect PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
  target_link_libraries(BoilEffect GL dl pthread)
endif()

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/BoilEffect.ofx.bundle/
  DESTINATION BoilEffect.ofx.bundle
)

install(TARGETS BoilEffect
  LIBRARY DESTINATION BoilEffect.ofx.bundle/${PLUGIN_DIR}
)

message(STATUS "")
message(STATUS "Build: cmake --build build")
message(STATUS "Install: cmake --install build --prefix <path>")
message(STATUS "")
