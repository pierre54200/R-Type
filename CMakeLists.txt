cmake_minimum_required(VERSION 3.12)
project(RType CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Serveur
file(GLOB_RECURSE SERVER_SOURCES CONFIGURE_DEPENDS
    server/src/*.cpp
    server/src/*.hpp
)

add_executable(r-type-server ${SERVER_SOURCES})
target_include_directories(r-type-server PRIVATE server/src common)
target_compile_options(r-type-server PRIVATE -Wall -Wextra)

# Client
file(GLOB_RECURSE CLIENT_SOURCES CONFIGURE_DEPENDS
    client/src/*.cpp
    client/src/*.hpp
)

add_executable(r-type-client ${CLIENT_SOURCES})
target_include_directories(r-type-client PRIVATE client/src common)
target_compile_options(r-type-client PRIVATE -Wall -Wextra)

# Les deux exécutables à la racine du projet
set_target_properties(r-type-server r-type-client PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
)