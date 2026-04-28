#ifndef ROBOT_LOG_COLLECTOR_INFRASTRUCTURE_ROSOUT_COLLECTOR_NODE_HPP_
#define ROBOT_LOG_COLLECTOR_INFRASTRUCTURE_ROSOUT_COLLECTOR_NODE_HPP_

#include "robot_log_collector/application/log_collector_service.hpp"
#include "robot_log_collector/infrastructure/file_log_repository.hpp"

#include <rcl_interfaces/msg/log.hpp>
#include <rclcpp/rclcpp.hpp>

#include <chrono>
#include <memory>

namespace robot_log_collector::infrastructure {

/**
 * @brief `/rosout` 토픽을 구독해 애플리케이션 서비스로 전달하는 ROS 어댑터 노드.
 *
 * 노드는 ROS parameter 를 읽어 애플리케이션 및 인프라 설정으로 변환하고,
 * 수신한 ROS 로그 메시지를 도메인 `LogRecord`로 매핑한다.
 */
class RosoutCollectorNode : public rclcpp::Node {
 private:
  // private member variables
  FileLogRepositoryConfig repository_config_;
  application::LogCollectorConfig collector_config_;
  std::unique_ptr<FileLogRepository> repository_;
  std::unique_ptr<application::LogCollectorService> service_;
  rclcpp::Subscription<rcl_interfaces::msg::Log>::SharedPtr subscription_;
  rclcpp::TimerBase::SharedPtr stats_timer_;
  std::chrono::seconds stats_report_period_{5};

  // private member functions
  void DeclareParameters();
  void LoadParameters();
  void OnLog(const rcl_interfaces::msg::Log::SharedPtr msg);
  void ReportStats();
  static uint64_t NowNs();

 public:
  /**
   * @brief ROS 노드를 생성하고 의존 객체를 조립한다.
   *
   * @param options ROS node options.
   */
  explicit RosoutCollectorNode(const rclcpp::NodeOptions& options);

  /**
   * @brief 종료 시 유실된 로그 수를 출력한다.
   */
  ~RosoutCollectorNode() override;
};

}  // namespace robot_log_collector::infrastructure

#endif  // ROBOT_LOG_COLLECTOR_INFRASTRUCTURE_ROSOUT_COLLECTOR_NODE_HPP_
