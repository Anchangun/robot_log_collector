#include "robot_log_collector/infrastructure/rosout_collector_node.hpp"

#include <rclcpp/rclcpp.hpp>

#include <memory>

int main(int argc, char** argv) {
  rclcpp::init(argc, argv);

  rclcpp::NodeOptions options;
  options.enable_rosout(false);

  auto node =
      std::make_shared<robot_log_collector::infrastructure::RosoutCollectorNode>(
          options);
  rclcpp::spin(node);

  node.reset();
  rclcpp::shutdown();
  return 0;
}
