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

set(CMAKE_CXX_FLAGS "-std=c++17 -O3 -s -Wall -Werror")
# set(CMAKE_CXX_FLAGS "-std=c++17 -g -Wall -Werror")
# set(CMAKE_CXX_FLAGS "-std=c++17 -g -Wall -Werror -fsanitize=address")

add_compile_options(-Wall -Wextra -pedantic -Wno-unused-parameter)

message ("cxx Flags: " ${CMAKE_CXX_FLAGS})
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

find_package(OpenMP REQUIRED)
find_library(ARROW_LIBRARY libarrow.a HINTS /opt/3rd/arrow/lib/ REQUIRED)
find_library(ARROW_DEPS_LIBRARY libarrow_bundled_dependencies.a HINTS /opt/3rd/arrow/lib/ REQUIRED)
find_library(CRYPTOPP_LIB libcryptopp.a HINTS /home/kun/system/lib REQUIRED)

# Source code
include_directories(SYSTEM /opt/3rd/arrow/include/)
include_directories(SYSTEM /opt/3rd/common/include/)
include_directories(SYSTEM /home/kun/system/include/)
include_directories(include/)

add_subdirectory(src)

install(DIRECTORY include/ DESTINATION include)
install(DIRECTORY cmake/ DESTINATION cmake)
install(
    DIRECTORY script/
    DESTINATION script
    FILE_PERMISSIONS 
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
    DIRECTORY_PERMISSIONS 
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
        GROUP_READ GROUP_EXECUTE
        WORLD_READ WORLD_EXECUTE
)
