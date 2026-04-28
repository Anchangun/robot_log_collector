#include "robot_log_collector/domain/syslog_log_formatter.hpp"

#include "robot_log_collector/domain/log_level.hpp"

#include <ctime>
#include <iomanip>
#include <regex>
#include <sstream>
#include <utility>

namespace robot_log_collector::domain {

namespace {

std::string StripExistingLogPrefix(const std::string& message) {
  static const std::regex kSyslogPrefixPattern(
      R"(^\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}\.\d{3} [^ ]+ local1\.[a-z]+: [^[]+\[\d+\]:\s*(.*)$)");
  static const std::regex kRosConsolePrefixPattern(
      R"(^\[[A-Z]+\] \[[^\]]+\] \[[^\]]+\]:\s*(.*)$)");

  std::smatch match;
  if (std::regex_match(message, match, kSyslogPrefixPattern)) {
    return match[1].str();
  }

  if (std::regex_match(message, match, kRosConsolePrefixPattern)) {
    return match[1].str();
  }

  return message;
}

std::string FormatHumanReadableTimestamp(const LogTimestamp& timestamp) {
  const auto time_t_value = static_cast<time_t>(timestamp.seconds);
  std::tm tm = {};
  localtime_r(&time_t_value, &tm);

  const uint32_t milliseconds = timestamp.nanoseconds / 1000000U;

  std::ostringstream stream;
  stream << std::put_time(&tm, "%Y-%m-%d %H:%M:%S")
         << "."
         << std::setw(3)
         << std::setfill('0')
         << milliseconds;

  return stream.str();
}

}  // namespace

SyslogLogFormatter::SyslogLogFormatter(std::string hostname)
    : hostname_(std::move(hostname)) {}

std::string SyslogLogFormatter::Format(const LogRecord& record,
                                       int process_id) const {
  static_cast<void>(hostname_);
  static_cast<void>(process_id);
  const std::string formatted_timestamp =
      FormatHumanReadableTimestamp(record.stamp);
  const std::string normalized_message = StripExistingLogPrefix(record.message);

  std::ostringstream oss;
  oss << "["
      << LogLevelName(record.level)
      << "] ["
      << formatted_timestamp
      << "] ["
      << record.logger
      << "]: "
      << normalized_message;

  return oss.str();
}

}  // namespace robot_log_collector::domain
