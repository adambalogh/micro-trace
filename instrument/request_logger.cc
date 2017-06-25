#include "request_logger.h"

#include <math.h>
#include <iostream>
#include <string>

#include "google/protobuf/text_format.h"

// spdlog customizations here
#define SPDLOG_NO_DATETIME
#define SPDLOG_NO_THREAD_ID
#define SPDLOG_NO_NAME
#define SPDLOG_EOL ""

#include "spdlog/spdlog.h"

#include "common.h"

using ::google::protobuf::TextFormat;

namespace microtrace {

void StdoutRequestLogger::Log(const proto::RequestLog& log) {
    std::string str;
    TextFormat::PrintToString(log, &str);
    std::cout << str << std::endl;
    std::cout << std::flush;
}

void SpdRequestLogger::Log(const proto::RequestLog& log) {
    std::string str;
    TextFormat::PrintToString(log, &str);
    spdlog_->info(str);
}

SpdRequestLoggerInstance::SpdRequestLoggerInstance() {
    const int queue_size = pow(2, 14);  // TODO find a good number here
    spdlog::set_async_mode(queue_size,
                           spdlog::async_overflow_policy::block_retry);
    auto spdlogger =
        spdlog::basic_logger_mt("request_logger", REQUEST_LOG_PATH);
    spdlog::set_sync_mode();  // Make sure other loggers are not async
    spdlogger->set_pattern("%v");

    logger_.reset(new instance(std::move(spdlogger)));
}

SpdRequestLoggerInstance::instance* SpdRequestLoggerInstance::get() {
    return logger_.get();
}
}
