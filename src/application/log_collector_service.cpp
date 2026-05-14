#include "robot_log_collector/application/log_collector_service.hpp"

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

namespace robot_log_collector::application {

namespace {

int64_t TimestampToNs(const domain::LogTimestamp& timestamp) {
  return static_cast<int64_t>(timestamp.seconds) * 1000000000LL +
         static_cast<int64_t>(timestamp.nanoseconds);
}

bool StampLess(const domain::LogRecord& lhs, const domain::LogRecord& rhs) {
  const int64_t lhs_ns = TimestampToNs(lhs.stamp);
  const int64_t rhs_ns = TimestampToNs(rhs.stamp);
  if (lhs_ns != rhs_ns) {
    return lhs_ns < rhs_ns;
  }

  return lhs.received_time_ns < rhs.received_time_ns;
}

}  // namespace

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
  const auto reorder_window =
      std::chrono::milliseconds(std::max(0, config_.reorder_window_ms));
  const int64_t reorder_window_ns =
      std::chrono::duration_cast<std::chrono::nanoseconds>(reorder_window).count();
  int64_t max_seen_stamp_ns = std::numeric_limits<int64_t>::min();
  std::vector<domain::LogRecord> pending;
  size_t write_count_since_flush = 0;
  auto last_flush_time = std::chrono::steady_clock::now();
  auto last_enqueue_time = std::chrono::steady_clock::now();

  auto write_ready_records = [&](std::vector<domain::LogRecord>& records) {
    if (records.empty()) {
      return;
    }

    repository_->AppendBatch(records);
    written_count_.fetch_add(records.size());
    write_count_since_flush += records.size();
  };

  auto flush_if_needed = [&]() {
    const auto now = std::chrono::steady_clock::now();
    if (write_count_since_flush >= flush_every_n ||
        now - last_flush_time >= flush_interval) {
      repository_->Flush();
      write_count_since_flush = 0;
      last_flush_time = now;
    }
  };

  while (running_.load() || !IsQueueEmpty()) {
    std::vector<domain::LogRecord> batch;
    batch.reserve(batch_size);

    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      if (pending.empty()) {
        queue_cv_.wait(lock, [this]() { return !running_.load() || !queue_.empty(); });
      } else {
        queue_cv_.wait_for(lock, reorder_window,
                           [this]() { return !running_.load() || !queue_.empty(); });
      }

      while (!queue_.empty() && batch.size() < batch_size) {
        batch.push_back(std::move(queue_.front()));
        queue_.pop_front();
      }
    }

    const auto now = std::chrono::steady_clock::now();
    if (!batch.empty()) {
      last_enqueue_time = now;
      for (auto& record : batch) {
        max_seen_stamp_ns = std::max(max_seen_stamp_ns, TimestampToNs(record.stamp));
        pending.push_back(std::move(record));
      }
    }

    if (pending.empty()) {
      continue;
    }

    std::sort(pending.begin(), pending.end(), StampLess);

    const int64_t watermark_ns = max_seen_stamp_ns - reorder_window_ns;
    auto ready_end = std::upper_bound(
        pending.begin(), pending.end(), watermark_ns,
        [](const int64_t watermark, const domain::LogRecord& record) {
          return watermark < TimestampToNs(record.stamp);
        });

    const bool inactive_tail =
        batch.empty() && now - last_enqueue_time >= reorder_window;
    if (inactive_tail) {
      ready_end = pending.end();
    }

    if (ready_end != pending.begin()) {
      std::vector<domain::LogRecord> ready;
      ready.reserve(static_cast<size_t>(std::distance(pending.begin(), ready_end)));
      std::move(pending.begin(), ready_end, std::back_inserter(ready));
      pending.erase(pending.begin(), ready_end);
      write_ready_records(ready);
    }

    flush_if_needed();
  }

  if (!pending.empty()) {
    std::sort(pending.begin(), pending.end(), StampLess);
    write_ready_records(pending);
  }

  if (write_count_since_flush > 0) {
    repository_->Flush();
  }
}

}  // namespace robot_log_collector::application
