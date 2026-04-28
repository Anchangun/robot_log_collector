#include "robot_log_collector/application/log_collector_service.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <utility>
#include <vector>

namespace robot_log_collector::application {

LogCollectorService::LogCollectorService(LogCollectorConfig config,
                                         LogRepository* repository)
    : config_(std::move(config)),
      repository_(repository),
      include_logger_regex_(config_.include_logger_regex),
      use_exclude_logger_names_(!config_.exclude_logger_names.empty()),
      writer_thread_(&LogCollectorService::WriterLoop, this) {
  if (repository_ == nullptr) {
    throw std::invalid_argument("Log repository must not be null");
  }
}

LogCollectorService::~LogCollectorService() {
  running_.store(false);
  queue_cv_.notify_all();

  if (writer_thread_.joinable()) {
    writer_thread_.join();
  }

  repository_->Flush();
}

void LogCollectorService::Enqueue(domain::LogRecord record) {
  received_count_.fetch_add(1);

  if (!MatchesFilters(record)) {
    filtered_count_.fetch_add(1);
    return;
  }

  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    if (queue_.size() >= static_cast<size_t>(config_.queue_capacity)) {
      dropped_count_.fetch_add(1);
      return;
    }

    queue_.push_back(std::move(record));
    peak_queue_size_.store(std::max(peak_queue_size_.load(), queue_.size()));
  }

  queue_cv_.notify_one();
}

uint64_t LogCollectorService::dropped_count() const {
  return dropped_count_.load();
}

uint64_t LogCollectorService::received_count() const {
  return received_count_.load();
}

uint64_t LogCollectorService::filtered_count() const {
  return filtered_count_.load();
}

uint64_t LogCollectorService::written_count() const {
  return written_count_.load();
}

size_t LogCollectorService::queue_size() const {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  return queue_.size();
}

size_t LogCollectorService::peak_queue_size() const {
  return peak_queue_size_.load();
}

bool LogCollectorService::MatchesFilters(const domain::LogRecord& record) const {
  if (static_cast<uint8_t>(record.level) < static_cast<uint8_t>(config_.min_level)) {
    return false;
  }

  if (!std::regex_search(record.logger, include_logger_regex_)) {
    return false;
  }

  if (use_exclude_logger_names_) {
    const auto excluded_logger = std::find(config_.exclude_logger_names.begin(),
                                           config_.exclude_logger_names.end(),
                                           record.logger);
    if (excluded_logger != config_.exclude_logger_names.end()) {
      return false;
    }
  }

  return true;
}

bool LogCollectorService::IsQueueEmpty() {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  return queue_.empty();
}

void LogCollectorService::WriterLoop() {
  const size_t batch_size = static_cast<size_t>(std::max(1, config_.writer_batch_size));
  const size_t flush_every_n = static_cast<size_t>(std::max(1, config_.flush_every_n));
  const auto flush_interval = std::chrono::milliseconds(std::max(1, config_.flush_interval_ms));
  size_t write_count_since_flush = 0;
  auto last_flush_time = std::chrono::steady_clock::now();

  while (running_.load() || !IsQueueEmpty()) {
    std::vector<domain::LogRecord> batch;
    batch.reserve(batch_size);

    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      queue_cv_.wait(lock, [this]() { return !running_.load() || !queue_.empty(); });

      while (!queue_.empty() && batch.size() < batch_size) {
        batch.push_back(std::move(queue_.front()));
        queue_.pop_front();
      }
    }

    if (batch.empty()) {
      continue;
    }

    repository_->AppendBatch(batch);
    written_count_.fetch_add(batch.size());
    write_count_since_flush += batch.size();

    const auto now = std::chrono::steady_clock::now();
    if (write_count_since_flush >= flush_every_n ||
        now - last_flush_time >= flush_interval) {
      repository_->Flush();
      write_count_since_flush = 0;
      last_flush_time = now;
    }
  }

  if (write_count_since_flush > 0) {
    repository_->Flush();
  }
}

}  // namespace robot_log_collector::application
