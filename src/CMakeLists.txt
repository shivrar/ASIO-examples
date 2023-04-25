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

# Add an executable and link with the Boost libraries
add_executable(gzip_eg gzip_eg.cpp)
target_link_libraries(gzip_eg ${Boost_LIBRARIES})

add_executable(asio_eg.o asio_datetime_str_server.cpp)
target_link_libraries(asio_eg.o ${Boost_LIBRARIES})

add_executable(asio_gzip_eg.o asio_gzip_eg.cpp)
target_link_libraries(asio_gzip_eg.o ${Boost_LIBRARIES})