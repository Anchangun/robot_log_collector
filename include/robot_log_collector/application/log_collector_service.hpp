#ifndef ROBOT_LOG_COLLECTOR_APPLICATION_LOG_COLLECTOR_SERVICE_HPP_
#define ROBOT_LOG_COLLECTOR_APPLICATION_LOG_COLLECTOR_SERVICE_HPP_

#include "robot_log_collector/application/log_repository.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <regex>
#include <string>
#include <thread>
#include <vector>

namespace robot_log_collector::application {

/**
 * @brief 로그 수집 서비스 동작에 필요한 애플리케이션 설정.
 */
struct LogCollectorConfig {
  domain::LogLevel min_level = domain::LogLevel::kInfo;
  int flush_every_n = 100;
  int flush_interval_ms = 1000;
  int queue_capacity = 100000;
  int writer_batch_size = 2048;
  std::string include_logger_regex = ".*";
  std::vector<std::string> exclude_logger_names = {"robot_log_collector"};
};

/**
 * @brief 로그 필터링, 큐잉, 비동기 저장 흐름을 담당하는 애플리케이션 서비스.
 *
 * 이 클래스는 domain 의 `LogRecord`를 받아 필터 규칙을 적용하고,
 * 내부 큐를 통해 백그라운드 쓰기 스레드로 전달한다.
 * 실제 저장 방식은 `LogRepository` 구현체에 위임한다.
 */
class LogCollectorService {
 private:
  // private member variables
  LogCollectorConfig config_;
  LogRepository* repository_;
  std::regex include_logger_regex_;
  bool use_exclude_logger_names_ = false;

  std::atomic<bool> running_{true};
  std::atomic<uint64_t> received_count_{0};
  std::atomic<uint64_t> filtered_count_{0};
  std::atomic<uint64_t> written_count_{0};
  std::atomic<uint64_t> dropped_count_{0};
  std::atomic<size_t> peak_queue_size_{0};

  mutable std::mutex queue_mutex_;
  std::condition_variable queue_cv_;
  std::deque<domain::LogRecord> queue_;
  std::thread writer_thread_;

  // private member functions
  bool MatchesFilters(const domain::LogRecord& record) const;
  bool IsQueueEmpty();
  void WriterLoop();

 public:
  /**
   * @brief 수집 서비스를 생성하고 백그라운드 쓰기 스레드를 시작한다.
   *
   * @param config 필터링 및 flush 정책.
   * @param repository 저장을 위임할 저장소 구현체.
   */
  LogCollectorService(LogCollectorConfig config, LogRepository* repository);

  /**
   * @brief 백그라운드 스레드를 종료하고 남은 로그를 flush 한다.
   */
  ~LogCollectorService();

  LogCollectorService(const LogCollectorService&) = delete;
  LogCollectorService& operator=(const LogCollectorService&) = delete;

  /**
   * @brief 로그 레코드를 수집 서비스 큐에 전달한다.
   *
   * 필터에 맞지 않거나 큐가 가득 찬 경우에는 저장하지 않는다.
   *
   * @param record 수집 대상 로그 레코드.
   */
  void Enqueue(domain::LogRecord record);

  /**
   * @brief 큐 용량 초과로 유실된 로그 개수를 반환한다.
   *
   * @return 유실된 로그 개수.
   */
  uint64_t dropped_count() const;

  /**
   * @brief 수신한 전체 로그 개수를 반환한다.
   *
   * @return 수신 로그 개수.
   */
  uint64_t received_count() const;

  /**
   * @brief 필터 조건으로 제외된 로그 개수를 반환한다.
   *
   * @return 필터 제외 로그 개수.
   */
  uint64_t filtered_count() const;

  /**
   * @brief 파일에 기록 완료한 로그 개수를 반환한다.
   *
   * @return 기록 완료 로그 개수.
   */
  uint64_t written_count() const;

  /**
   * @brief 현재 큐에 쌓여 있는 로그 개수를 반환한다.
   *
   * @return 현재 큐 길이.
   */
  size_t queue_size() const;

  /**
   * @brief 관측된 최대 큐 길이를 반환한다.
   *
   * @return 최대 큐 길이.
   */
  size_t peak_queue_size() const;
};

}  // namespace robot_log_collector::application

#endif  // ROBOT_LOG_COLLECTOR_APPLICATION_LOG_COLLECTOR_SERVICE_HPP_
