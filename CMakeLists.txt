cmake_minimum_required (VERSION 3.1)
project (chemist)

set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(vendor/verbly)
add_subdirectory(vendor/yaml-cpp EXCLUDE_FROM_ALL)
add_subdirectory(vendor/libtwittercpp)

include_directories(vendor/verbly/lib vendor/yaml-cpp/include vendor/libtwittercpp/src)
add_executable(chemist chemist.cpp)
set_property(TARGET chemist PROPERTY CXX_STANDARD 11)
set_property(TARGET chemist PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(chemist verbly twitter++ yaml-cpp)
