cmake_minimum_required(VERSION 3.18)
project(hPartioEmitter CXX)

set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "Limited configurations" FORCE)
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Set the build type to Release by default" FORCE)
mark_as_advanced(CMAKE_BUILD_TYPE CMAKE_CONFIGURATION_TYPES)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Optionally add filesystem library linkage for GNU and Clang
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        link_libraries(stdc++fs)
    elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        link_libraries(c++fs)
    endif()
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake" ${CMAKE_MODULE_PATH})
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Paths for Houdini and Partio libraries
set(HOUDINI_INSTALL_PATH "C:/Program Files/Side Effects Software/Houdini20.0.625")
set(HOUDINI_LIB_PATH "${HOUDINI_INSTALL_PATH}/custom/houdini/dsolib")
set(Partio_LIBRARIES "${CMAKE_SOURCE_DIR}/lib/partio.lib") 

if (NOT EXISTS "${HOUDINI_INSTALL_PATH}")
    message(FATAL_ERROR "Houdini install path not found: ${HOUDINI_INSTALL_PATH}")
endif()

if (NOT EXISTS "${HOUDINI_LIB_PATH}")
    message(FATAL_ERROR "Houdini lib path not found: ${HOUDINI_LIB_PATH}")
endif()

include_directories(
    "${HOUDINI_INSTALL_PATH}/toolkit/include"
    "${CMAKE_SOURCE_DIR}/extern/partio/src/lib"
    "."
)

include(Common)
set(ExternalInstallDir "${CMAKE_BINARY_DIR}/extern/install" CACHE INTERNAL "")
set(EXT_CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE INTERNAL "")
if (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(EXT_CMAKE_BUILD_TYPE "Release" CACHE INTERNAL "")
endif()
set(TOPLEVEL_INCLUDE_DIR ${PROJECT_SOURCE_DIR})

# Ensure all necessary external libraries for SPlisHSPlasH are available
add_subdirectory("extern/zlib")
add_subdirectory("extern/partio")
add_subdirectory("extern/md5")
add_subdirectory("extern/tinyexpr")
add_subdirectory("extern/glfw")
add_subdirectory("extern/nfd")

find_package(Eigen3 REQUIRED)
add_definitions(-DEIGEN_DISABLE_UNALIGNED_ARRAY_ASSERT)
include(SetUpExternalProjects)

# Now add the SPlisHSPlasH project directory
add_subdirectory(SPlisHSPlasH)
add_subdirectory(Utilities)
# add_subdirectory(FoamGenerator)

# Define source and header files
set(SOURCE_FILES
    src/Plugin.C
    src/Simulator.cpp
    src/Visualizer.cpp
    src/main.cpp
    src/vec.cpp
    src/FluidSimulator.cpp
    src/FoamGeneratorNode.cpp
    src/FoamGenerationMain.cpp
    src/FoamKernel.cpp
    src/FluidConfiguration.cpp
    src/RigidBody.cpp
)

set(HEADER_FILES
    src/Plugin.h
    src/Simulator.h
    src/Visualizer.h
    src/matrix.h
    src/vec.h
    src/FluidSimulator.h
    src/FoamGeneratorNode.h
    src/FoamGenerationMain.h
    src/FoamKernel.h
    src/FluidConfiguration.h
    src/RigidBody.h
)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE "/std:c++17")
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    VERSION="${HOUDINI_VERSION}"
    _USE_MATH_DEFINES
    SESI_LITTLE_ENDIAN
    SIZEOF_VOID_P=8
    MAKING_DSO
    HBOOST_ALL_NO_LIB
)

link_directories(${HOUDINI_LIB_PATH})

file(GLOB LIB_FILES "${HOUDINI_LIB_PATH}/*.lib")
target_link_libraries(${PROJECT_NAME} PRIVATE "${LIB_FILES}" "${Partio_LIBRARIES}" 
    SPlisHSPlasH # Link to SPlisHSPlasH library
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Check if the custom DSO directory environment variable is set, if not use a fallback
set(CUSTOM_DSO_DIR $ENV{CUSTOM_DSO_PATH})
if(NOT CUSTOM_DSO_DIR)
    set(CUSTOM_DSO_DIR "${CMAKE_BINARY_DIR}/custom_dso")  # Fallback directory
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:${PROJECT_NAME}>"
    "${CUSTOM_DSO_DIR}"
    COMMENT "Copying ${PROJECT_NAME}.dll to CUSTOM_DSO_PATH directory"
)