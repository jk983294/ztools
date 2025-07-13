cmake_minimum_required(VERSION 3.15)

project(zerg)

set(CMAKE_INSTALL_PREFIX "/opt/version/latest/zerg/")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/)
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/lib)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#SET(CMAKE_CXX_FLAGS "-std=c++17 -O3 -s -Wall -Werror")
SET(CMAKE_CXX_FLAGS "-std=c++17 -g -Wall -Werror")
# SET(CMAKE_CXX_FLAGS "-std=c++17 -g -Wall -Werror -fsanitize=address")

ADD_COMPILE_OPTIONS(-Wall -Wextra -pedantic -Wno-unused-parameter)

message ("cxx Flags: " ${CMAKE_CXX_FLAGS})
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

FIND_PACKAGE(OpenMP REQUIRED)
FIND_LIBRARY(ARROW_LIBRARY libarrow.a HINTS /opt/3rd/arrow/lib/ REQUIRED)
FIND_LIBRARY(ARROW_DEPS_LIBRARY libarrow_bundled_dependencies.a HINTS /opt/3rd/arrow/lib/ REQUIRED)
FIND_LIBRARY(CRYPTOPP_LIB libcryptopp.a HINTS /home/kun/system/lib REQUIRED)

# Source code
include_directories(SYSTEM /opt/3rd/arrow/include/)
include_directories(SYSTEM /opt/3rd/common/include/)
include_directories(SYSTEM /home/kun/system/include/)
include_directories(include/)

add_subdirectory(src)

install(DIRECTORY include/ DESTINATION include)

