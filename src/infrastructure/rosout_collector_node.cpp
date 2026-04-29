#include "robot_log_collector/infrastructure/rosout_collector_node.hpp"

#include "robot_log_collector/domain/log_level.hpp"
#include "robot_log_collector/domain/log_record.hpp"

#include <rclcpp/qos.hpp>

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <vector>

namespace robot_log_collector::infrastructure {

RosoutCollectorNode::RosoutCollectorNode(const rclcpp::NodeOptions& options)
    : Node("robot_log_collector", options) {
  DeclareParameters();
  LoadParameters();

  repository_ = std::make_unique<FileLogRepository>(repository_config_);
  service_ = std::make_unique<application::LogCollectorService>(
      collector_config_, repository_.get());

  subscription_ = this->create_subscription<rcl_interfaces::msg::Log>(
      "/rosout", rclcpp::RosoutQoS(),
      std::bind(&RosoutCollectorNode::OnLog, this, std::placeholders::_1));
  stats_timer_ = this->create_wall_timer(
      stats_report_period_, std::bind(&RosoutCollectorNode::ReportStats, this));

  RCLCPP_INFO(this->get_logger(), "robot log collector started. output_dir=%s",
              repository_config_.output_dir.string().c_str());
}

RosoutCollectorNode::~RosoutCollectorNode() {
  if (service_ == nullptr) {
    return;
  }

  const uint64_t dropped_count = service_->dropped_count();
  if (dropped_count > 0) {
    std::cerr << "[robot_log_collector] dropped logs: " << dropped_count
              << std::endl;
  }

  std::cerr << "[robot_log_collector] received=" << service_->received_count()
            << " written=" << service_->written_count()
            << " filtered=" << service_->filtered_count()
            << " dropped=" << dropped_count
            << " peak_queue=" << service_->peak_queue_size() << std::endl;
}

void RosoutCollectorNode::DeclareParameters() {
  this->declare_parameter<std::string>("log_output_dir", kDefaultLogOutputDir);
  this->declare_parameter<std::string>("output_dir", "");
  this->declare_parameter<std::string>("file_prefix", "rosout");
  this->declare_parameter<std::string>("min_level", "INFO");
  this->declare_parameter<int>("max_file_size_mb", 50);
  this->declare_parameter<int>("max_files", 20);
  this->declare_parameter<int>("flush_every_n", 1);
  this->declare_parameter<int>("flush_interval_ms", 1);
  this->declare_parameter<int>("queue_capacity", 100000);
  this->declare_parameter<int>("writer_batch_size", 2048);
  this->declare_parameter<int>("stats_report_period_sec", 5);
  this->declare_parameter<std::string>("include_logger_regex", ".*");
  this->declare_parameter<std::vector<std::string>>(
      "exclude_logger_names", std::vector<std::string>{"robot_log_collector"});
  this->declare_parameter<std::string>("exclude_logger_name", "");
}

void RosoutCollectorNode::LoadParameters() {
  const std::string log_output_dir =
      this->get_parameter("log_output_dir").as_string();
  const std::string legacy_output_dir =
      this->get_parameter("output_dir").as_string();

  if (!legacy_output_dir.empty()) {
    repository_config_.output_dir = ExpandUserPath(legacy_output_dir);
    RCLCPP_WARN(
        this->get_logger(),
        "parameter 'output_dir' is deprecated. Use 'log_output_dir' instead.");
  } else {
    repository_config_.output_dir = ExpandUserPath(log_output_dir);
  }
  repository_config_.file_prefix = this->get_parameter("file_prefix").as_string();

  const int max_file_size_mb = this->get_parameter("max_file_size_mb").as_int();
  repository_config_.max_file_size_bytes =
      static_cast<size_t>(std::max(1, max_file_size_mb)) * 1024U * 1024U;
  repository_config_.max_files =
      static_cast<int>(this->get_parameter("max_files").as_int());

  collector_config_.min_level = domain::LogLevelFromName(
      domain::ToUpper(this->get_parameter("min_level").as_string()));
  collector_config_.flush_every_n =
      static_cast<int>(this->get_parameter("flush_every_n").as_int());
  collector_config_.flush_interval_ms =
      static_cast<int>(this->get_parameter("flush_interval_ms").as_int());
  collector_config_.queue_capacity =
      static_cast<int>(this->get_parameter("queue_capacity").as_int());
  collector_config_.writer_batch_size =
      static_cast<int>(this->get_parameter("writer_batch_size").as_int());
  collector_config_.include_logger_regex =
      this->get_parameter("include_logger_regex").as_string();
  collector_config_.exclude_logger_names =
      this->get_parameter("exclude_logger_names").as_string_array();
  const std::string legacy_exclude_logger_name =
      this->get_parameter("exclude_logger_name").as_string();
  if (!legacy_exclude_logger_name.empty()) {
    collector_config_.exclude_logger_names.push_back(legacy_exclude_logger_name);
    RCLCPP_WARN(
        this->get_logger(),
        "parameter 'exclude_logger_name' is deprecated. Use 'exclude_logger_names' instead.");
  }
  stats_report_period_ = std::chrono::seconds(std::max(
      1, static_cast<int>(this->get_parameter("stats_report_period_sec").as_int())));
}

void RosoutCollectorNode::OnLog(const rcl_interfaces::msg::Log::SharedPtr msg) {
  domain::LogRecord record;
  record.stamp.seconds = msg->stamp.sec;
  record.stamp.nanoseconds = msg->stamp.nanosec;
  record.received_time_ns = NowNs();
  record.level = static_cast<domain::LogLevel>(msg->level);
  record.logger = msg->name;
  record.message = msg->msg;
  record.file = msg->file;
  record.function = msg->function;
  record.line = msg->line;
  service_->Enqueue(std::move(record));
}

void RosoutCollectorNode::ReportStats() {
  if (service_ == nullptr) {
    return;
  }

  const uint64_t dropped_count = service_->dropped_count();
  const size_t current_queue_size = service_->queue_size();
  if (dropped_count == 0 && current_queue_size == 0) {
    return;
  }

  RCLCPP_WARN(
      this->get_logger(),
      "collector stats: received=%llu written=%llu filtered=%llu dropped=%llu queue=%zu peak_queue=%zu",
      static_cast<unsigned long long>(service_->received_count()),
      static_cast<unsigned long long>(service_->written_count()),
      static_cast<unsigned long long>(service_->filtered_count()),
      static_cast<unsigned long long>(dropped_count), current_queue_size,
      service_->peak_queue_size());
}

uint64_t RosoutCollectorNode::NowNs() {
  const auto now = std::chrono::system_clock::now().time_since_epoch();
  return static_cast<uint64_t>(
      std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

}  // namespace robot_log_collector::infrastructure
