cmake_minimum_required(VERSION 2.8.3)
project(nocs_compression)

## Compile as C++11, supported in ROS Kinetic and newer
add_compile_options(-std=c++11 -Wall -Wextra -Wpedantic)

## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages
# Find the Boost libraries and include directories
find_package(Boost REQUIRED COMPONENTS filesystem iostreams system thread)
include_directories(${Boost_INCLUDE_DIRS})

add_definitions(-DBOOST_IOSTREAMS_NO_LIB)

set(LZMA_INCLUDE_DIR /usr/include/lzma)
set(LZMA_LIB /usr/lib/x86_64-linux-gnu/liblzma.so)

add_library(lzma lzma.cpp)
target_compile_definitions(lzma PRIVATE BOOST_IOSTREAMS_NO_LIB)
target_include_directories(lzma PRIVATE ${LZMA_INCLUDE_DIR})
target_link_libraries(lzma ${Boost_LIBRARIES} ${LZMA_LIB})

# Add an executable and link with the Boost libraries
add_executable(gzip_eg gzip_eg.cpp)
target_link_libraries(gzip_eg ${Boost_LIBRARIES})

add_executable(gzip_decomp_eg gzip_decompression_eg.cpp)
target_link_libraries(gzip_decomp_eg ${Boost_LIBRARIES})

add_executable(asio_eg.o asio_datetime_str_server.cpp)
target_link_libraries(asio_eg.o ${Boost_LIBRARIES})

add_executable(asio_gzip_eg.o asio_gzip_eg.cpp)
target_link_libraries(asio_gzip_eg.o ${Boost_LIBRARIES} lzma)