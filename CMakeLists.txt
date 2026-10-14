cmake_minimum_required(VERSION 3.16)
project(R-Type LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

find_package(Threads REQUIRED)

# Coverage
option(COVERAGE "Compiler avec la mesure de couverture (gcov)" OFF)
if(COVERAGE)
    add_compile_options(--coverage -O0 -fno-inline)
    add_link_options(--coverage)
endif()

set(SRC ${CMAKE_SOURCE_DIR}/server/src)
set(MAIN_SOURCE ${SRC}/main.cpp)

file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS ${SRC}/*.cpp ${SRC}/*.cc ${SRC}/*.cxx)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    ${SRC}/*.hpp ${SRC}/*.h ${SRC}/*.hh ${SRC}/*.tpp ${SRC}/*.ipp)

# Tests
set(TEST_SOURCES ${ALL_SOURCES})
list(FILTER TEST_SOURCES INCLUDE REGEX "\\.(spec|test)\\.(cpp|cc|cxx)$")
list(FILTER ALL_SOURCES EXCLUDE REGEX "\\.(spec|test)\\.(cpp|cc|cxx)$")

# Tout le code sauf main pour les tests
set(CORE_SOURCES ${ALL_SOURCES})
list(REMOVE_ITEM CORE_SOURCES ${MAIN_SOURCE})

if(CORE_SOURCES)
    add_library(core OBJECT ${CORE_SOURCES} ${HEADERS})
    set(CORE_SCOPE PUBLIC)
else()
    add_library(core INTERFACE)
    set(CORE_SCOPE INTERFACE)
endif()
target_include_directories(core ${CORE_SCOPE} ${SRC})
target_compile_options(core ${CORE_SCOPE} -Wall -Wextra)
target_link_libraries(core ${CORE_SCOPE} Threads::Threads)

# Executable
add_executable(r-type_server ${MAIN_SOURCE})
target_link_libraries(r-type_server PRIVATE core)

find_package(GTest QUIET)
if(TEST_SOURCES AND GTest_FOUND)
    enable_testing()
    add_executable(tests ${TEST_SOURCES})
    target_link_libraries(tests PRIVATE core GTest::gtest GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(tests)
elseif(TEST_SOURCES)
    message(WARNING "Des fichiers de test existent mais GoogleTest est introuvable")
endif()