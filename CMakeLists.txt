# cmake version
cmake_minimum_required(VERSION 3.15)

# project name
PROJECT(rppi_get)

# build config
#set(CMAKE_BUILD_TYPE "Release")
#set(CMAKE_BUILD_TYPE "Debug")

# for libgit2
if(MSVC)
	set(BUILD_SHARED_LIBS ON CACHE BOOL "Build Shared Library (OFF for Static)" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_TESTS OFF CACHE BOOL "Build Tests using the Clar suite" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Build the example tree.Build library usage example apps" FORCE)
set(BUILD_CLI OFF CACHE BOOL "Build the command-line interface." FORCE)

# for libgit2 ends

add_subdirectory(deps/yaml-cpp)
add_subdirectory(deps/libgit2)
add_subdirectory(deps/cxxopts)
add_subdirectory(deps/json)

set(LIBGIT2_LIB_DIR "${CMAKE_CURRENT_BINARY_DIR}/deps/libgit2")
link_directories(${LIBGIT2_LIB_DIR})

#header path
INCLUDE_DIRECTORIES (
	include 
	deps/cxxopts/include
	deps/json/include
	deps/libgit2/include
)

# source directory
aux_source_directory(./src DIR_SRC)

# set rppi_get target srcs
set(rppi_get ${DIR_SRC})

# add executable
add_executable(${PROJECT_NAME} ${rppi_get})

# add link library
if(WIN32)
	target_link_libraries( ${PROJECT_NAME} PRIVATE git2 yaml-cpp shlwapi user32 Rpcrt4)
else()
	target_link_libraries( ${PROJECT_NAME} PRIVATE git2 yaml-cpp)
endif()

set(TARGET_FILES
	${CMAKE_SOURCE_DIR}/rppi_config.yaml
	)

set(INSTALL_DIR ${CMAKE_SOURCE_DIR}/dist)

install(
	TARGETS ${PROJECT_NAME}
	DESTINATION ${INSTALL_DIR}
	)
install(
	FILES ${TARGET_FILES}
	DESTINATION ${INSTALL_DIR}
	)
