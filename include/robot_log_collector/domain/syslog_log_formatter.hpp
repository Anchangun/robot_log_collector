#ifndef ROBOT_LOG_COLLECTOR_DOMAIN_SYSLOG_LOG_FORMATTER_HPP_
#define ROBOT_LOG_COLLECTOR_DOMAIN_SYSLOG_LOG_FORMATTER_HPP_

#include "robot_log_collector/domain/log_record.hpp"

#include <string>

namespace robot_log_collector::domain {

/**
 * @brief `LogRecord`를 사람이 읽기 쉬운 한 줄 문자열로 변환하는 도메인 서비스.
 */
class SyslogLogFormatter {
 private:
  std::string hostname_;

 public:
  /**
   * @brief 포맷터를 생성한다.
   *
   * @param hostname 기존 인터페이스 호환을 위해 유지하는 호스트명.
   */
  explicit SyslogLogFormatter(std::string hostname);

  /**
   * @brief 도메인 로그 레코드를 한 줄 문자열로 변환한다.
   *
   * @param record 변환할 로그 레코드.
   * @param process_id 기존 인터페이스 호환을 위해 유지하는 프로세스 ID.
   * @return 사람이 읽기 쉬운 한 줄 문자열.
   */
  std::string Format(const LogRecord& record, int process_id) const;
};

}  // namespace robot_log_collector::domain

#endif  // ROBOT_LOG_COLLECTOR_DOMAIN_SYSLOG_LOG_FORMATTER_HPP_
