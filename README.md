# robot_log_collector

## 개요

ROS 2 `/rosout` 토픽 기반 로그 수집 패키지.

동작 흐름:

1. `RosoutCollectorNode`의 `/rosout` 구독
2. `rcl_interfaces/msg/Log`의 `LogRecord` 변환
3. `LogCollectorService`의 레벨 및 logger 필터링
4. 내부 큐 적재 및 writer thread 배치 처리
5. `FileLogRepository`의 `.log` 파일 기록 및 rotate

`main.cpp` 설정:

- `enable_rosout(false)` 적용
- collector 노드 자체 rosout 재발행 루프 방지

## 주요 기능

- `/rosout` 로그 수집
- 로그 레벨 기반 필터링
- `logger name` 정규식 포함 필터링
- `logger name` 목록 제외 필터링
- 비동기 큐 기반 배치 저장
- `flush_every_n`, `flush_interval_ms` 기반 flush 제어
- 파일 크기 기준 rotate
- `max_files` 기준 오래된 로그 파일 정리
- 통계 출력

## 패키지 구조

역할 분리:

| layer | component | role |
| --- | --- | --- |
| `domain` | `LogLevel` | 로그 레벨 변환 |
| `domain` | `LogRecord` | 내부 로그 데이터 모델 |
| `domain` | `SyslogLogFormatter` | 한 줄 로그 포맷 변환 |
| `application` | `LogCollectorService` | 필터링, 큐잉, writer thread 관리 |
| `application` | `LogRepository` | 저장소 인터페이스 |
| `infrastructure` | `RosoutCollectorNode` | ROS node, parameter 로드, `/rosout` subscription |
| `infrastructure` | `FileLogRepository` | 파일 저장, rotate, prune 처리 |

## 빌드

```bash
colcon build --packages-select robot_log_collector
```

## 실행

기본 실행:

```bash
ros2 run robot_log_collector robot_log_collector
```

parameter file 적용:

```bash
ros2 run robot_log_collector robot_log_collector \
  --ros-args \
  --params-file config/robot_log_collector.yaml
```

## 로그 저장 형식

저장 형식:

```text
[LEVEL] [YYYY-MM-DD HH:MM:SS.mmm] [logger_name]: message
```

포맷 처리:

- 기존 syslog prefix 제거
- 기존 ROS console prefix 제거
- `msg->stamp` 기반 `YYYY-MM-DD HH:MM:SS.mmm` 변환
- `msg->name` 기반 `logger name` 사용

## 로그 파일 이름

파일 이름 규칙:

```text
<file_prefix>_YYYYMMDD_HHMMSS_pid<PID>_<index>.log
```

예시:

```text
rosout_20260428_153000_pid12345_000.log
```

rotate 및 정리 기준:

- 현재 파일 크기 + 신규 로그 1줄 > `max_file_size_bytes` 조건 시 rotate
- `max_files` 초과 시 동일 `file_prefix`와 `.log` 확장자 대상 오래된 파일부터 삭제

## Parameters

코드 기준 declare 및 사용 parameter:

기본 `config/robot_log_collector.yaml`에 포함된 parameter:

| key | type | default | description |
| --- | --- | --- | --- |
| `log_output_dir` | `string` | `~/robot_log_collector/logs` | 로그 저장 디렉터리 |
| `file_prefix` | `string` | `rosout` | 로그 파일 prefix |
| `min_level` | `string` | `INFO` | 최소 저장 로그 레벨. `DEBUG`, `INFO`, `WARN`, `WARNING`, `ERROR`, `FATAL` 처리 |
| `max_file_size_mb` | `int` | `50` | 로그 파일 최대 크기 MB 단위 |
| `max_files` | `int` | `20` | 유지할 로그 파일 최대 개수 |
| `flush_every_n` | `int` | `1` | 지정한 로그 건수마다 flush |
| `flush_interval_ms` | `int` | `1000` | 지정한 시간 간격마다 flush |
| `queue_capacity` | `int` | `100000` | 내부 큐 최대 길이 |
| `writer_batch_size` | `int` | `2048` | writer thread가 한 번에 기록하는 최대 배치 크기 |
| `stats_report_period_sec` | `int` | `5` | 통계 출력 주기 초 단위 |
| `include_logger_regex` | `string` | `.*` | 저장 대상 logger name 정규식 |
| `exclude_logger_names` | `string[]` | `["robot_log_collector"]` | 저장 제외 logger name 목록 |

호환성 유지용 deprecated parameter:

| key | type | default | description |
| --- | --- | --- | --- |
| `output_dir` | `string` | `""` | 구 버전 경로 parameter. 값이 있으면 `log_output_dir` 대신 사용하며 deprecated warning 출력 |
| `exclude_logger_name` | `string` | `""` | 구 버전 제외 logger parameter. 값이 있으면 `exclude_logger_names`에 추가하며 deprecated warning 출력 |

flush 동작 기준:

- `flush_every_n` 건 누적 시 flush 수행
- `flush_interval_ms` 시간이 먼저 경과하면 `flush_every_n`에 도달하지 않아도 flush 수행
- 기본값 `flush_interval_ms = 1000`은 지나치게 잦은 flush를 줄이면서 주기적 flush를 유지하기 위한 설정

## 통계 출력

통계 출력 조건:

- `dropped_count == 0` 그리고 `queue size == 0` 조건 시 주기 통계 미출력
- 그 외 조건 시 `received`, `written`, `filtered`, `dropped`, `queue`, `peak_queue` warning log 출력
- 종료 시 `dropped logs` 및 최종 통계 `stderr` 출력

---

# robot_log_collector

## Overview

ROS 2 `/rosout` topic-based log collection package.

Flow:

1. `/rosout` subscription by `RosoutCollectorNode`
2. `rcl_interfaces/msg/Log` to `LogRecord` conversion
3. Level and logger filtering by `LogCollectorService`
4. Internal queueing and writer thread batch processing
5. `.log` file write and rotation by `FileLogRepository`

`main.cpp` configuration:

- `enable_rosout(false)` applied
- prevention of collector node self rosout republish loop

## Features

- `/rosout` log collection
- log level-based filtering
- `logger name` regex include filtering
- `logger name` list exclude filtering
- asynchronous queue-based batch write
- flush control based on `flush_every_n` and `flush_interval_ms`
- file size-based rotation
- old log file pruning based on `max_files`
- statistics output

## Package Structure

Responsibility split:

| layer | component | role |
| --- | --- | --- |
| `domain` | `LogLevel` | log level conversion |
| `domain` | `LogRecord` | internal log data model |
| `domain` | `SyslogLogFormatter` | single-line log format conversion |
| `application` | `LogCollectorService` | filtering, queueing, writer thread management |
| `application` | `LogRepository` | repository interface |
| `infrastructure` | `RosoutCollectorNode` | ROS node, parameter loading, `/rosout` subscription |
| `infrastructure` | `FileLogRepository` | file write, rotation, prune processing |

## Build

```bash
colcon build --packages-select robot_log_collector
```

## Run

Default run:

```bash
ros2 run robot_log_collector robot_log_collector
```

Run with parameter file:

```bash
ros2 run robot_log_collector robot_log_collector \
  --ros-args \
  --params-file config/robot_log_collector.yaml
```

## Log Output Format

Stored log format:

```text
[LEVEL] [YYYY-MM-DD HH:MM:SS.mmm] [logger_name]: message
```

Format processing:

- existing syslog prefix removal
- existing ROS console prefix removal
- `msg->stamp`-based `YYYY-MM-DD HH:MM:SS.mmm` conversion
- `msg->name`-based `logger name` usage

## Log File Name

File naming rule:

```text
<file_prefix>_YYYYMMDD_HHMMSS_pid<PID>_<index>.log
```

Example:

```text
rosout_20260428_153000_pid12345_000.log
```

Rotation and pruning rules:

- rotate when current file size + one new log line exceeds `max_file_size_bytes`
- remove oldest files first among files matching the same `file_prefix` and `.log` extension when `max_files` is exceeded

## Parameters

Declared and used parameters in code:

Parameters included in the default `config/robot_log_collector.yaml`:

| key | type | default | description |
| --- | --- | --- | --- |
| `log_output_dir` | `string` | `~/robot_log_collector/logs` | log output directory |
| `file_prefix` | `string` | `rosout` | log file prefix |
| `min_level` | `string` | `INFO` | minimum persisted log level. Supports `DEBUG`, `INFO`, `WARN`, `WARNING`, `ERROR`, `FATAL` |
| `max_file_size_mb` | `int` | `50` | maximum log file size in MB |
| `max_files` | `int` | `20` | maximum number of retained log files |
| `flush_every_n` | `int` | `1` | flush after the configured number of log records |
| `flush_interval_ms` | `int` | `1000` | flush at the configured time interval |
| `queue_capacity` | `int` | `100000` | maximum internal queue length |
| `writer_batch_size` | `int` | `2048` | maximum batch size written by the writer thread at once |
| `stats_report_period_sec` | `int` | `5` | statistics reporting period in seconds |
| `include_logger_regex` | `string` | `.*` | logger name regex for inclusion |
| `exclude_logger_names` | `string[]` | `["robot_log_collector"]` | logger name list for exclusion |

Deprecated parameters kept for compatibility:

| key | type | default | description |
| --- | --- | --- | --- |
| `output_dir` | `string` | `""` | legacy path parameter. If set, used instead of `log_output_dir` and a deprecated warning is printed |
| `exclude_logger_name` | `string` | `""` | legacy excluded logger parameter. If set, appended to `exclude_logger_names` and a deprecated warning is printed |

Flush behavior:

- flush after `flush_every_n` accumulated records
- flush earlier when `flush_interval_ms` elapses, even if `flush_every_n` has not been reached
- default `flush_interval_ms = 1000` keeps periodic flushing while reducing excessively frequent flush operations

## Statistics Output

Statistics output conditions:

- no periodic statistics output when `dropped_count == 0` and `queue size == 0`
- warning log output of `received`, `written`, `filtered`, `dropped`, `queue`, and `peak_queue` otherwise
- `stderr` output of `dropped logs` and final statistics at shutdown
