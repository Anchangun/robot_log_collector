cmake_minimum_required(VERSION 3.8)
project(robot_log_collector)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rcl_interfaces REQUIRED)

add_executable(robot_log_collector
  src/main.cpp
  src/application/log_collector_service.cpp
  src/domain/log_level.cpp
  src/domain/syslog_log_formatter.cpp
  src/infrastructure/file_log_repository.cpp
  src/infrastructure/rosout_collector_node.cpp
)

target_include_directories(robot_log_collector PRIVATE
  include
)

ament_target_dependencies(robot_log_collector
  rclcpp
  rcl_interfaces
)

install(TARGETS robot_log_collector
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY config
  DESTINATION share/${PROJECT_NAME}
)

ament_package()
