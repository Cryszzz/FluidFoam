cmake_minimum_required(VERSION 3.18)

project(hPartioEmitter CXX)

set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "Limited configurations" FORCE)
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Set the build type to Release by default" FORCE)

mark_as_advanced(CMAKE_BUILD_TYPE CMAKE_CONFIGURATION_TYPES)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(HOUDINI_INSTALL_PATH "C:/Program Files/Side Effects Software/Houdini 20.0.590")
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
    "extern/partio/src/lib"
    "."
)

set(SOURCE_FILES
    src/Plugin.C
    src/Simulator.cpp
    src/FoamGenerator.cpp
    src/Visualizer.cpp
    src/main.cpp
    src/vec.cpp
)

set(HEADER_FILES
    src/Plugin.h
    src/FoamGenerator.h
    src/Simulator.h
    src/Visualizer.h
    src/matrix.h
    src/vec.h
)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})

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
target_link_libraries(${PROJECT_NAME} PRIVATE "${LIB_FILES}" "${Partio_LIBRARIES}" )

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

set(CUSTOM_DSO_DIR $ENV{CUSTOM_DSO_PATH})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:${PROJECT_NAME}>"
    "${CUSTOM_DSO_DIR}"
    COMMENT "Copying ${PROJECT_NAME}.dll to CUSTOM_DSO_PATH directory"
)