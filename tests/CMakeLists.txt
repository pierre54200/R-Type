CPMAddPackage(
    NAME googletest
    GITHUB_REPOSITORY google/googletest
    GIT_TAG v1.15.2
    OPTIONS
        "INSTALL_GTEST OFF"
        "BUILD_GMOCK OFF"
        "gtest_force_shared_crt ON"
)

add_executable(r-type_tests
    test_protocol.cpp
    test_entity_manager.cpp
    test_registery.cpp
    test_vec3.cpp
    test_clock.cpp
    test_string.cpp
    test_systems.cpp

    ${CMAKE_SOURCE_DIR}/client/src/ECS/Clock/Clock.cpp
    ${CMAKE_SOURCE_DIR}/client/src/ECS/String/String.cpp
)

target_include_directories(r-type_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/client/src
)

target_link_libraries(r-type_tests PRIVATE GTest::gtest_main sfml-graphics)
if(WIN32)
    target_link_libraries(r-type_tests PRIVATE ws2_32)
endif()

if(MSVC)
    target_compile_options(r-type_tests PRIVATE /W4)
else()
    target_compile_options(r-type_tests PRIVATE -Wall -Wextra)
endif()

include(GoogleTest)
gtest_discover_tests(r-type_tests DISCOVERY_TIMEOUT 30)
