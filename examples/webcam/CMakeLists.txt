cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

find_package(Boost 1.60.0 REQUIRED)
find_package(OpenCV 3.0.0 REQUIRED)
find_package (Threads)

find_library(Rabbitmqc_LIBRARIES
    NAMES rabbitmq
    HINTS "/usr/local/lib/x86_64-linux-gnu/"
)

find_library(SAC_LIBRARIES
    NAMES SimpleAmqpClient
    HINTS "/usr/local/lib"
)

find_library(Msgpack_LIBRARIES
    NAMES msgpackc
    HINTS "/usr/local/lib"
)

find_library(IS_LIBRARIES
    NAMES is
    HINTS "/usr/local/lib"
)

add_executable(Webcam webcam.cpp)
target_link_libraries(Webcam 
  ${IS_LIBRARIES} 
  ${OpenCV_LIBS} 
  ${Boost_LIBRARIES} 
  ${Rabbitmqc_LIBRARIES} 
  ${SAC_LIBRARIES} 
  ${CMAKE_THREAD_LIBS_INIT} 
  ${Msgpack_LIBRARIES}
)