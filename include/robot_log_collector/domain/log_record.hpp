#ifndef ROBOT_LOG_COLLECTOR_DOMAIN_LOG_RECORD_HPP_
#define ROBOT_LOG_COLLECTOR_DOMAIN_LOG_RECORD_HPP_

#include "robot_log_collector/domain/log_level.hpp"

#include <cstdint>
#include <string>

namespace robot_log_collector::domain {

/**
 * @brief 로그 시각을 초와 나노초 단위로 표현하는 값 객체.
 */
struct LogTimestamp {
  int32_t seconds;
  uint32_t nanoseconds;
};

/**
 * @brief 수집된 ROS 로그를 도메인 관점에서 표현한 레코드.
 */
struct LogRecord {
  LogTimestamp stamp;
  uint64_t received_time_ns;
  LogLevel level;
  std::string logger;
  std::string message;
  std::string file;
  std::string function;
  uint32_t line;
};

}  // namespace robot_log_collector::domain

#endif  // ROBOT_LOG_COLLECTOR_DOMAIN_LOG_RECORD_HPP_
