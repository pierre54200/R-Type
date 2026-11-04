cmake_minimum_required(VERSION 3.12)
project(RType CXX)

set(CMAKE_CXX_STANDARD 17)
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


# Choose flag (Windows or linux)
if(MSVC)
    set(RTYPE_WARNINGS /W4)
else()
    set(RTYPE_WARNINGS -Wall -Wextra)
endif()

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

CPMAddPackage(
    NAME asio
    GITHUB_REPOSITORY chriskohlhoff/asio
    GIT_TAG asio-1-30-2
    DOWNLOAD_ONLY YES
)
add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)
target_compile_definitions(asio INTERFACE ASIO_STANDALONE)
if(WIN32)
    target_compile_definitions(asio INTERFACE _WIN32_WINNT=0x0A00)
    target_link_libraries(asio INTERFACE ws2_32 wsock32)
endif()

# Serveur
file(GLOB_RECURSE SERVER_SOURCES CONFIGURE_DEPENDS
    server/src/*.cpp
    server/src/*.hpp
)

add_executable(r-type_server ${SERVER_SOURCES})
target_include_directories(r-type_server PRIVATE server/src common)
target_link_libraries(r-type_server PRIVATE asio)
target_compile_options(r-type_server PRIVATE ${RTYPE_WARNINGS})

# Client
file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS
    client/src/*.cpp
    client/src/*.hpp
)

add_executable(r-type_client ${CLIENT_SOURCES})
target_include_directories(r-type_client PRIVATE client/src common)
target_link_libraries(r-type_client PRIVATE sfml-graphics sfml-window sfml-system asio)
target_compile_options(r-type_client PRIVATE ${RTYPE_WARNINGS})

# Les deux exécutables à la racine du projet
set_target_properties(r-type_server r-type_client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)
