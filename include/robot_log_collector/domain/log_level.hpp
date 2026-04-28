#ifndef ROBOT_LOG_COLLECTOR_DOMAIN_LOG_LEVEL_HPP_
#define ROBOT_LOG_COLLECTOR_DOMAIN_LOG_LEVEL_HPP_

#include <cstdint>
#include <string>

namespace robot_log_collector::domain {

/**
 * @brief 내부 도메인에서 사용하는 로그 심각도 레벨.
 */
enum class LogLevel : uint8_t {
  kDebug = 10,
  kInfo = 20,
  kWarn = 30,
  kError = 40,
  kFatal = 50,
};

/**
 * @brief 문자열을 대문자로 정규화한다.
 *
 * @param value 정규화할 문자열.
 * @return 대문자로 변환된 문자열.
 */
std::string ToUpper(std::string value);

/**
 * @brief 문자열 레벨 이름을 도메인 로그 레벨로 변환한다.
 *
 * @param level_name 예: `INFO`, `WARN`.
 * @return 대응하는 `LogLevel`.
 */
LogLevel LogLevelFromName(const std::string& level_name);

/**
 * @brief 도메인 로그 레벨을 표시용 문자열로 변환한다.
 *
 * @param level 변환할 로그 레벨.
 * @return 예: `INFO`, `ERROR`.
 */
std::string LogLevelName(LogLevel level);

/**
 * @brief 도메인 로그 레벨을 syslog severity 이름으로 변환한다.
 *
 * @param level 변환할 로그 레벨.
 * @return 예: `info`, `warning`, `err`.
 */
std::string SyslogSeverityName(LogLevel level);

}  // namespace robot_log_collector::domain

#endif  // ROBOT_LOG_COLLECTOR_DOMAIN_LOG_LEVEL_HPP_
