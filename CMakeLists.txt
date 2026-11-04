cmake_minimum_required(VERSION 3.14)
project(RType CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CPM_DOWNLOAD_VERSION 0.40.2)
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
if(NOT EXISTS "${CPM_DOWNLOAD_LOCATION}")
    message(STATUS "Telechargement de CPM.cmake ${CPM_DOWNLOAD_VERSION}")
    file(DOWNLOAD
        "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
        "${CPM_DOWNLOAD_LOCATION}"
        STATUS CPM_DOWNLOAD_STATUS)
    list(GET CPM_DOWNLOAD_STATUS 0 CPM_DOWNLOAD_CODE)
    if(NOT CPM_DOWNLOAD_CODE EQUAL 0)
        file(REMOVE "${CPM_DOWNLOAD_LOCATION}")
        message(FATAL_ERROR "Impossible de telecharger CPM.cmake : ${CPM_DOWNLOAD_STATUS}")
    endif()
endif()
include("${CPM_DOWNLOAD_LOCATION}")

# Flags de warnings selon le compilateur
if(MSVC)
    set(RTYPE_WARNINGS /W4)
else()
    set(RTYPE_WARNINGS -Wall -Wextra)
endif()

find_package(Threads REQUIRED)

CPMAddPackage(
    NAME SFML
    GITHUB_REPOSITORY SFML/SFML
    GIT_TAG 2.6.1
    OPTIONS
        "BUILD_SHARED_LIBS OFF"
        "SFML_BUILD_AUDIO OFF"
        "SFML_BUILD_NETWORK OFF"
        "SFML_BUILD_EXAMPLES OFF"
        "SFML_BUILD_DOC OFF"
)

# Bibliotheque reseau systeme (Winsock sous Windows, rien a ajouter sous Linux)
add_library(rtype_net INTERFACE)
target_link_libraries(rtype_net INTERFACE Threads::Threads)
if(WIN32)
    target_link_libraries(rtype_net INTERFACE ws2_32)
endif()

# Serveur
file(GLOB_RECURSE SERVER_SOURCES CONFIGURE_DEPENDS
    server/src/*.cpp
    server/src/*.hpp
)

add_executable(r-type_server ${SERVER_SOURCES})
target_include_directories(r-type_server PRIVATE server/src common)
target_link_libraries(r-type_server PRIVATE rtype_net)
target_compile_options(r-type_server PRIVATE ${RTYPE_WARNINGS})

# Client
file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS
    client/src/*.cpp
    client/src/*.hpp
)

add_executable(r-type_client ${CLIENT_SOURCES})
target_include_directories(r-type_client PRIVATE client/src common)
target_link_libraries(r-type_client PRIVATE sfml-graphics sfml-window sfml-system rtype_net)
target_compile_options(r-type_client PRIVATE ${RTYPE_WARNINGS})

# Les deux executables a la racine du projet
set_target_properties(r-type_server r-type_client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Tests unitaires (desactives par defaut : cmake -B build -DRTYPE_BUILD_TESTS=ON)
option(RTYPE_BUILD_TESTS "Build the unit tests" OFF)
if(RTYPE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
