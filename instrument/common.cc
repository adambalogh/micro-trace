#include "common.h"

#include "spdlog/spdlog.h"

namespace microtrace {

std::shared_ptr<spdlog::logger> console_log =
    spdlog::stdout_logger_mt("tracing_console_log");
}