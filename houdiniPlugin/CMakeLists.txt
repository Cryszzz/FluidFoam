cmake_minimum_required(VERSION 3.18)
set(CMAKE_SUPPRESS_REGENERATION "1")

project(houdiniPlugin CXX)
#set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "Limited configurations" FORCE)
#set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Set the build type to Release by default" FORCE)

#mark_as_advanced(CMAKE_BUILD_TYPE CMAKE_CONFIGURATION_TYPES)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_GENERATOR_PLATFORM x64)
set(CMAKE_DEBUG_POSTFIX "_d")

include_directories(${PROJECT_SOURCE_DIR})
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" ${CMAKE_MODULE_PATH})

if(WIN32)
	#add_definitions(-DNT_PLUGIN -DGLEW_STATIC)
endif()
if(UNIX)
  add_definitions(-DREQUIRE_IOSTREAM)
  add_compile_options(-fPIC -m64)
  set(PLUGIN_SUFFIX ".so")
  set(CMAKE_SHARED_LIBRARY_SUFFIX ".so")
  set(SUFFIX ".so")
  set(STD_INCLUDES "." ".." "/usr/X11R6/include" "/usr/include" "/usr/include/CC")
  include_directories(${STD_INCLUDES})
  add_definitions(-DLINUX)
endif()


add_subdirectory(extern/glew)
add_subdirectory(extern/partio)
add_subdirectory(extern/zlib)

link_directories("${PROJECT_SOURCE_DIR}/lib/Release")


add_library(houdiniPlugin SHARED
	src/PluginMain.h
	src/PluginMain.c
	src/PartioEmitter.cpp
	src/PartioEmitter.h
)



# houdini paths
set(HOUDINI_INSTALL_PATH "E:/houdini/Houdini 20.0.625")
set(HOUDINI_LIB_PATH "${HOUDINI_INSTALL_PATH}/custom/houdini/dsolib")

if (NOT EXISTS "${HOUDINI_INSTALL_PATH}")
    message(FATAL_ERROR "Houdini install path not found: ${HOUDINI_INSTALL_PATH}")
endif()

if (NOT EXISTS "${HOUDINI_LIB_PATH}")
    message(FATAL_ERROR "Houdini lib path not found: ${HOUDINI_LIB_PATH}")
endif()

include_directories(
    "${HOUDINI_INSTALL_PATH}/toolkit/include"
    "."
)
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
#target_link_libraries(${PROJECT_NAME} PRIVATE "${LIB_FILES}")

# external files
if(WIN32)
  target_link_libraries(${PROJECT_NAME} PRIVATE opengl32.lib glu32.lib glew partio zlib "${LIB_FILES}")
else()
  target_link_libraries(${PROJECT_NAME} PRIVATE GL GLU partio zlib "${LIB_FILES}")
endif()
add_dependencies(${PROJECT_NAME} glew partio zlib)

include_directories(${CMAKE_SOURCE_DIR}/extern/glew/include)

set_target_properties(glew PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(zlib PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(partio PROPERTIES LINKER_LANGUAGE CXX)

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
#set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)


add_custom_command(TARGET houdiniPlugin PRE_BUILD
					COMMAND ${CMAKE_COMMAND} -E copy_directory
					${CMAKE_SOURCE_DIR}/scripts $<TARGET_FILE_DIR:houdiniPlugin>/scripts)

# cpoy .dll to the houdini plugin folder					
set(CUSTOM_DSO_DIR $ENV{CUSTOM_DSO_PATH})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "$<TARGET_FILE:${PROJECT_NAME}>"
    "${CUSTOM_DSO_DIR}"
    COMMENT "Copying ${PROJECT_NAME}.dll to CUSTOM_DSO_PATH directory"
)



