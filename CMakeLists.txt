cmake_minimum_required (VERSION 2.6)
project (chemist)

set(CMAKE_BUILD_TYPE Debug)

add_subdirectory(vendor/twitcurl/libtwitcurl)
add_subdirectory(vendor/verbly)

find_package(PkgConfig)
pkg_check_modules(YamlCpp yaml-cpp REQUIRED)
pkg_check_modules(sqlite3 sqlite3 REQUIRED)

include_directories(vendor/twitcurl/libtwitcurl ${sqlite3_INCLUDE_DIR} vendor/verbly/lib)
add_executable(chemist chemist.cpp)
set_property(TARGET chemist PROPERTY CXX_STANDARD 11)
set_property(TARGET chemist PROPERTY CXX_STANDARD_REQUIRED ON)
target_link_libraries(chemist verbly ${sqlite3_LIBRARIES} ${YamlCpp_LIBRARIES} twitcurl curl)
