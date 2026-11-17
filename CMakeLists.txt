cmake_minimum_required(VERSION 3.20)

project(NexusPS5 VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

add_library(nexus_core
    src/core/config.cpp
    src/core/logger.cpp
)
target_include_directories(nexus_core PUBLIC include)
target_compile_features(nexus_core PUBLIC cxx_std_20)

add_executable(nexus src/frontend/main.cpp)
target_link_libraries(nexus PRIVATE nexus_core)

if(BUILD_TESTING)
    add_executable(nexus_core_tests tests/core_tests.cpp)
    target_link_libraries(nexus_core_tests PRIVATE nexus_core)
    add_test(NAME nexus_core_tests COMMAND nexus_core_tests)
endif()
