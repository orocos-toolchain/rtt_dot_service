cmake_minimum_required(VERSION 2.8.3)

project(rtt_dot_service)


set (ROS_ROOT $ENV{ROS_ROOT} )

if (ROS_ROOT)
    find_package(catkin REQUIRED)
    include_directories(${catkin_INCLUDE_DIRS})  
    include_directories(${USE_OROCOS_INCLUDE_DIRS})
    set( RTT_HINTS HINTS ${rtt_PACKAGE_PATH}/install )
endif()

set(CMAKE_BUILD_TYPE MinSizeRel)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

find_package(OROCOS-RTT REQUIRED ${RTT_HINTS})
include(${OROCOS-RTT_USE_FILE_PATH}/UseOROCOS-RTT.cmake)

orocos_service(rtt_dot_service src/rtt_dot_service.cpp)
orocos_generate_package(
  DEPENDS_TARGETS rtt
)

