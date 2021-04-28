cmake_minimum_required(VERSION 3.3)

project(rtt_dot_service)
find_package(ament_cmake REQUIRED)

set (ROS_ROOT $ENV{ROS_ROOT} )

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rtt_ros2 REQUIRED)

ament_package()

orocos_service(rtt_dot_service src/rtt_dot_service.cpp)
orocos_generate_package(
  DEPENDS_TARGETS rtt
)

