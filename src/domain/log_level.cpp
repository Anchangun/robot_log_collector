#include "robot_log_collector/domain/log_level.hpp"

#include <cctype>

namespace robot_log_collector::domain {

std::string ToUpper(std::string value) {
  for (auto& ch : value) {
    ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
  }

  return value;
}

LogLevel LogLevelFromName(const std::string& level_name) {
  if (level_name == "DEBUG") {
    return LogLevel::kDebug;
  }

  if (level_name == "INFO") {
    return LogLevel::kInfo;
  }

  if (level_name == "WARN" || level_name == "WARNING") {
    return LogLevel::kWarn;
  }

  if (level_name == "ERROR") {
    return LogLevel::kError;
  }

  if (level_name == "FATAL") {
    return LogLevel::kFatal;
  }

  return LogLevel::kInfo;
}

std::string LogLevelName(LogLevel level) {
  switch (level) {
    case LogLevel::kDebug:
      return "DEBUG";
    case LogLevel::kInfo:
      return "INFO";
    case LogLevel::kWarn:
      return "WARN";
    case LogLevel::kError:
      return "ERROR";
    case LogLevel::kFatal:
      return "FATAL";
    default:
      return "UNKNOWN";
  }
}

std::string SyslogSeverityName(LogLevel level) {
  switch (level) {
    case LogLevel::kDebug:
      return "debug";
    case LogLevel::kInfo:
      return "info";
    case LogLevel::kWarn:
      return "warning";
    case LogLevel::kError:
      return "err";
    case LogLevel::kFatal:
      return "crit";
    default:
      return "notice";
  }
}

}  // namespace robot_log_collector::domain
