#include "logger.h"

#include "spdlog/sinks/stdout_color_sinks.h"

std::shared_ptr<spdlog::logger> Logger::s_core_logger_;

void Logger::Init() {
  spdlog::set_pattern("%^[%T] %n: %v%$");
  s_core_logger_ = spdlog::stdout_color_mt("[SAT-SOLVER]");
  s_core_logger_->set_level(spdlog::level::trace);
}
