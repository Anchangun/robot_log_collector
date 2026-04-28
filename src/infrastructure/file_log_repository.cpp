#include "robot_log_collector/infrastructure/file_log_repository.hpp"

#include <unistd.h>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace robot_log_collector::infrastructure {

FileLogRepository::FileLogRepository(FileLogRepositoryConfig config)
    : config_(std::move(config)),
      formatter_(CurrentHostName()) {
  config_.output_dir = ExpandUserPath(config_.output_dir.string());
  std::filesystem::create_directories(config_.output_dir);
}

FileLogRepository::~FileLogRepository() {
  Flush();
  if (output_file_.is_open()) {
    output_file_.close();
  }
}

void FileLogRepository::Append(const domain::LogRecord& record) {
  const std::string formatted_line = formatter_.Format(record, getpid());
  AppendFormattedLine(formatted_line);
}

void FileLogRepository::AppendBatch(const std::vector<domain::LogRecord>& records) {
  for (const auto& record : records) {
    const std::string formatted_line = formatter_.Format(record, getpid());
    AppendFormattedLine(formatted_line);
  }
}

void FileLogRepository::Flush() {
  if (output_file_.is_open()) {
    output_file_.flush();
  }
}

void FileLogRepository::EnsureFileOpen() {
  if (!output_file_.is_open()) {
    OpenNewFile();
  }
}

void FileLogRepository::OpenNewFile() {
  std::filesystem::create_directories(config_.output_dir);

  current_file_path_ = config_.output_dir / MakeLogFileName();
  output_file_.open(current_file_path_, std::ios::out | std::ios::app);

  if (!output_file_.is_open()) {
    throw std::runtime_error("Failed to open log file: " + current_file_path_.string());
  }

  std::error_code error;
  current_file_size_bytes_ = std::filesystem::file_size(current_file_path_, error);
  if (error) {
    current_file_size_bytes_ = 0;
  }

  PruneOldFiles();
}

void FileLogRepository::RotateFile() {
  Flush();
  output_file_.close();
  current_file_size_bytes_ = 0;
  OpenNewFile();
}

void FileLogRepository::PruneOldFiles() {
  if (config_.max_files <= 0) {
    return;
  }

  std::vector<std::filesystem::directory_entry> files;
  std::error_code error;
  for (const auto& entry : std::filesystem::directory_iterator(config_.output_dir, error)) {
    if (error) {
      std::cerr << "[robot_log_collector] failed to iterate log directory for pruning: path="
                << config_.output_dir
                << ", error="
                << error.message()
                << std::endl;
      return;
    }

    if (!entry.is_regular_file()) {
      continue;
    }

    const std::string filename = entry.path().filename().string();
    const std::string prefix = config_.file_prefix + "_";
    const bool has_prefix = filename.rfind(prefix, 0) == 0;
    const bool has_log_extension = entry.path().extension() == ".log";

    if (has_prefix && has_log_extension) {
      files.push_back(entry);
    }
  }

  if (files.size() <= static_cast<size_t>(config_.max_files)) {
    return;
  }

  std::sort(files.begin(), files.end(), [](const auto& lhs, const auto& rhs) {
    return lhs.last_write_time() < rhs.last_write_time();
  });

  const size_t remove_count = files.size() - static_cast<size_t>(config_.max_files);
  for (size_t i = 0; i < remove_count; ++i) {
    std::error_code remove_error;
    std::filesystem::remove(files[i].path(), remove_error);
    if (remove_error) {
      std::cerr << "[robot_log_collector] failed to remove old log file: path="
                << files[i].path()
                << ", error="
                << remove_error.message()
                << std::endl;
    }
  }
}

void FileLogRepository::AppendFormattedLine(const std::string& formatted_line) {
  EnsureFileOpen();

  const size_t line_size_bytes = formatted_line.size() + 1;
  if (current_file_size_bytes_ > 0 &&
      current_file_size_bytes_ + line_size_bytes > config_.max_file_size_bytes) {
    RotateFile();
  }

  output_file_ << formatted_line << '\n';
  current_file_size_bytes_ += line_size_bytes;
}

std::string FileLogRepository::MakeLogFileName() {
  std::ostringstream oss;
  oss << config_.file_prefix
      << "_"
      << CurrentTimeForFileName()
      << "_pid"
      << getpid()
      << "_"
      << std::setw(3)
      << std::setfill('0')
      << file_index_++
      << ".log";
  return oss.str();
}

std::filesystem::path ExpandUserPath(const std::string& path) {
  if (path.empty() || path[0] != '~') {
    return std::filesystem::path(path);
  }

  const char* home = std::getenv("HOME");
  if (home == nullptr) {
    return std::filesystem::path(path);
  }

  if (path.size() == 1) {
    return std::filesystem::path(home);
  }

  if (path[1] == '/') {
    return std::filesystem::path(home) / path.substr(2);
  }

  return std::filesystem::path(path);
}

std::string CurrentTimeForFileName() {
  const auto now = std::chrono::system_clock::now();
  const auto now_time = std::chrono::system_clock::to_time_t(now);

  std::tm tm = {};
  localtime_r(&now_time, &tm);

  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
  return oss.str();
}

std::string CurrentHostName() {
  constexpr size_t kHostnameCapacity = 256;
  char hostname[kHostnameCapacity] = {};
  if (gethostname(hostname, sizeof(hostname)) != 0) {
    return "localhost";
  }

  hostname[kHostnameCapacity - 1] = '\0';
  return hostname;
}

}  // namespace robot_log_collector::infrastructure
