cmake_minimum_required(VERSION 3.16)
project(tcp_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(Threads REQUIRED)

# Toutes les sources et tous les headers de src/**, a n'importe quelle profondeur.
# CONFIGURE_DEPENDS : un fichier ajoute ou supprime est detecte au prochain build.
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.cpp
    ${CMAKE_SOURCE_DIR}/src/*.cc
    ${CMAKE_SOURCE_DIR}/src/*.cxx
)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.hpp
    ${CMAKE_SOURCE_DIR}/src/*.h
    ${CMAKE_SOURCE_DIR}/src/*.hh
    ${CMAKE_SOURCE_DIR}/src/*.tpp
    ${CMAKE_SOURCE_DIR}/src/*.ipp
)

add_executable(server ${SOURCES} ${HEADERS})

# src/ est la racine des includes : depuis n'importe quel fichier,
# on ecrit #include "net/Socket.hpp" pour src/net/Socket.hpp
target_include_directories(server PRIVATE ${CMAKE_SOURCE_DIR}/src)

target_compile_options(server PRIVATE -Wall -Wextra)
target_link_libraries(server PRIVATE Threads::Threads)