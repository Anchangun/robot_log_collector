#ifndef ROBOT_LOG_COLLECTOR_APPLICATION_LOG_REPOSITORY_HPP_
#define ROBOT_LOG_COLLECTOR_APPLICATION_LOG_REPOSITORY_HPP_

#include "robot_log_collector/domain/log_record.hpp"

#include <vector>

namespace robot_log_collector::application {

/**
 * @brief 로그 영속화 구현체가 따라야 하는 저장소 인터페이스.
 *
 * 애플리케이션 계층은 로그를 어떤 저장 매체에 기록하는지 알지 않아야 하므로,
 * 로그 기록과 flush 동작만 이 인터페이스로 의존한다.
 */
class LogRepository {
 public:
  virtual ~LogRepository() = default;

  /**
   * @brief 로그 레코드 하나를 저장소에 추가한다.
   *
   * @param record 저장할 도메인 로그 레코드.
   */
  virtual void Append(const domain::LogRecord& record) = 0;

  /**
   * @brief 버퍼링된 로그를 즉시 저장 매체로 반영한다.
   */
  virtual void Flush() = 0;

  /**
   * @brief 로그 레코드 여러 건을 저장소에 추가한다.
   *
   * 기본 구현은 `Append()`를 반복 호출한다.
   *
   * @param records 저장할 도메인 로그 레코드 목록.
   */
  virtual void AppendBatch(const std::vector<domain::LogRecord>& records) {
    for (const auto& record : records) {
      Append(record);
    }
  }
};

}  // namespace robot_log_collector::application

#endif  // ROBOT_LOG_COLLECTOR_APPLICATION_LOG_REPOSITORY_HPP_
