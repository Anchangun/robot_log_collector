#ifndef ROBOT_LOG_COLLECTOR_INFRASTRUCTURE_FILE_LOG_REPOSITORY_HPP_
#define ROBOT_LOG_COLLECTOR_INFRASTRUCTURE_FILE_LOG_REPOSITORY_HPP_

#include "robot_log_collector/application/log_repository.hpp"
#include "robot_log_collector/domain/syslog_log_formatter.hpp"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace robot_log_collector::infrastructure {

inline constexpr char kDefaultLogOutputDir[] = "~/robot_log_collector/logs";

/**
 * @brief 파일 기반 로그 저장소 설정.
 */
struct FileLogRepositoryConfig {
  std::filesystem::path output_dir = kDefaultLogOutputDir;
  std::string file_prefix = "rosout";
  size_t max_file_size_bytes = 50U * 1024U * 1024U;
  int max_files = 20;
};

/**
 * @brief 도메인 로그를 회전 가능한 `.log` 파일로 저장하는 인프라 구현체.
 */
class FileLogRepository : public application::LogRepository {
 private:
  // private member variables
  FileLogRepositoryConfig config_;
  domain::SyslogLogFormatter formatter_;
  std::filesystem::path current_file_path_;
  std::ofstream output_file_;
  size_t current_file_size_bytes_ = 0;
  size_t file_index_ = 0;

  // private member functions
  void EnsureFileOpen();
  bool IsCurrentFileMissing() const;
  void OpenNewFile();
  void RotateFile();
  void PruneOldFiles();
  void AppendFormattedLine(const std::string& formatted_line);
  void InitializeNextFileIndex();
  std::filesystem::path FindAppendableLogFileForCurrentDate() const;
  std::string MakeLogFileName();

 public:
  /**
   * @brief 파일 저장소를 생성한다.
   *
   * @param config 출력 디렉터리와 회전 정책.
   */
  explicit FileLogRepository(FileLogRepositoryConfig config);

  /**
   * @brief 열린 파일을 flush 하고 정리한다.
   */
  ~FileLogRepository() override;

  FileLogRepository(const FileLogRepository&) = delete;
  FileLogRepository& operator=(const FileLogRepository&) = delete;

  /**
   * @brief 로그 레코드 하나를 현재 출력 파일에 추가한다.
   *
   * @param record 저장할 로그 레코드.
   */
  void Append(const domain::LogRecord& record) override;

  /**
   * @brief 열린 출력 파일 버퍼를 즉시 flush 한다.
   */
  void Flush() override;

  /**
   * @brief 로그 레코드 여러 건을 현재 출력 파일에 추가한다.
   *
   * @param records 저장할 로그 레코드 목록.
   */
  void AppendBatch(const std::vector<domain::LogRecord>& records) override;
};

/**
 * @brief `~`가 포함된 사용자 경로를 실제 파일 시스템 경로로 확장한다.
 *
 * @param path 확장할 원본 경로.
 * @return 확장된 경로.
 */
std::filesystem::path ExpandUserPath(const std::string& path);

/**
 * @brief 로그 파일명에 사용할 현재 시각 문자열을 생성한다.
 *
 * @return `YYYYMMDD_HHMMSS` 형식 문자열.
 */
std::string CurrentTimeForFileName();

/**
 * @brief 현재 호스트 이름을 조회한다.
 *
 * @return 조회된 호스트명. 실패 시 `localhost`.
 */
std::string CurrentHostName();

}  // namespace robot_log_collector::infrastructure

#endif  // ROBOT_LOG_COLLECTOR_INFRASTRUCTURE_FILE_LOG_REPOSITORY_HPP_
