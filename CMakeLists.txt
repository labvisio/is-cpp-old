cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++14")

project(is_cpp)

SET(IS_LIB_SRCS
  src/connection.cpp
  src/service-client.cpp
  src/service-provider.cpp  
)

add_library(is SHARED ${IS_LIB_SRCS})
target_link_libraries(is)

install(
  TARGETS is
  DESTINATION "${CMAKE_INSTALL_PREFIX}/lib"
)

install(
  FILES
    src/is/is.hpp
    src/is/connection.hpp
    src/is/service-client.hpp
    src/is/service-provider.hpp
    src/is/logger.hpp
    src/is/packer.hpp
    src/is/helpers.hpp
    src/is/msgs.hpp
    src/is/msgs/common.hpp
    src/is/msgs/geometry.hpp
    src/is/msgs/camera.hpp
    src/is/msgs/robot.hpp

  DESTINATION 
    include/is
)