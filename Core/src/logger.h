#pragma once

#include <chrono>
#include <memory>

#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

class Logger {
 public:
  static void Init();

  static std::shared_ptr<spdlog::logger>& GetCoreLogger() {
    return s_core_logger_;
  }

 private:
  static std::shared_ptr<spdlog::logger> s_core_logger_;
};

#define LOG_TRACE(...) ::Logger::GetCoreLogger()->trace(__VA_ARGS__)
#define LOG_INFO(...) ::Logger::GetCoreLogger()->info(__VA_ARGS__)
#define LOG_WARN(...) ::Logger::GetCoreLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...) ::Logger::GetCoreLogger()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::Logger::GetCoreLogger()->critical(__VA_ARGS__)

#define LOG_INFO_THROTTLED(delay_ms, ...)                                    \
  do {                                                                       \
    static auto last_log_time = std::chrono::steady_clock::now();            \
    auto current_time = std::chrono::steady_clock::now();                    \
    if (std::chrono::duration_cast<std::chrono::milliseconds>(current_time - \
                                                              last_log_time) \
            .count() > delay_ms) {                                           \
      LOG_INFO(__VA_ARGS__);                                                 \
      last_log_time = current_time;                                          \
    }                                                                        \
  } while (0)
