cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "-std=c++11 -Wall -Werror -Wextra")

project(is_cpp)

SET(IS_LIB_SRCS 
  src/connection.cpp
  src/service-client.cpp
  src/service-provider.cpp
  src/theora/ogg-encoder.cpp  
  src/theora/ogg-decoder.cpp  
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

  DESTINATION 
    include/is
)

install(
  FILES
    src/is/msgs/common.hpp
    src/is/msgs/geometry.hpp
    src/is/msgs/camera.hpp
    src/is/msgs/robot.hpp

  DESTINATION 
    include/is/msgs
)

install(
  FILES
    src/is/theora/ogg-encoder.hpp
    src/is/theora/ogg-decoder.hpp

  DESTINATION 
    include/is/theora
)