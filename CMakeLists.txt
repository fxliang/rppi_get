cmake_minimum_required(VERSION 3.15)
project(rppi_get)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(GNUInstallDirs)

# options
option(USE_BUNDLED_CXXOPTS "Use built-in version of cxxopts" ${WIN32})
option(USE_BUNDLED_JSON "Use built-in version of nlohmann/json" ${WIN32})
option(USE_BUNDLED_LIBGIT2 "Use built-in version of libgit2" ${WIN32})
option(USE_BUNDLED_YAMLCPP "Use built-in version of yaml-cpp" ${WIN32})

# dependencies
if (USE_BUNDLED_CXXOPTS)
    add_subdirectory(deps/cxxopts)
else()
    find_package(cxxopts REQUIRED)
endif()

if (USE_BUNDLED_JSON)
    set(JSON_BuildTests OFF CACHE INTERNAL "")
    add_subdirectory(deps/json)
else()
    find_package(nlohmann_json REQUIRED)
endif()

if (USE_BUNDLED_LIBGIT2)
    set(BUILD_TESTS OFF CACHE INTERNAL "")
    set(BUILD_CLI OFF CACHE INTERNAL "")
    if (WIN32)
        set(BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
    endif()
    add_subdirectory(deps/libgit2)
    set(LIBGIT2_LIBRARIES libgit2package)
    set(LIBGIT2_INCLUDE_DIR deps/libgit2/include)
else()
    find_package(PkgConfig REQUIRED)
    pkg_search_module(LIBGIT2 REQUIRED libgit2)
endif()

if (USE_BUNDLED_YAMLCPP)
    set(YAML_CPP_BUILD_TOOLS OFF CACHE INTERNAL "")
    set(YAML_CPP_BUILD_TESTS OFF CACHE INTERNAL "")
    set(YAML_CPP_INSTALL OFF CACHE INTERNAL "")
    if (WIN32)
        set(YAML_BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
    endif()
    add_subdirectory(deps/yaml-cpp)
else()
    find_package(yaml-cpp REQUIRED)
endif()

if (WIN32)
    set(WIN32_LIBS shlwapi user32 Rpcrt4)
endif()

# source directory
aux_source_directory(./src DIR_SRC)

# set rppi_get target srcs
set(rppi_get ${DIR_SRC})

# add executable
add_executable(${PROJECT_NAME} ${rppi_get})

# add link libraries
target_link_libraries(${PROJECT_NAME}
    cxxopts::cxxopts
    ${LIBGIT2_LIBRARIES}
    nlohmann_json::nlohmann_json
    yaml-cpp
    ${WIN32_LIBS}
)

# add include directories
target_include_directories(${PROJECT_NAME} PUBLIC
    ${LIBGIT2_INCLUDE_DIR}
)

set(TARGET_FILES ${CMAKE_SOURCE_DIR}/rppi_config.yaml)

install(
    TARGETS ${PROJECT_NAME}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    FILES ${TARGET_FILES}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)
