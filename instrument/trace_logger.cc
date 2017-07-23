#include "trace_logger.h"

#include <math.h>
#include <iostream>
#include <sstream>

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

const std::string TRACE_LOG_PATH = "/var/log/microtrace/log.proto";

void StdoutTraceLogger::Log(const proto::RequestLog& log) {
    std::string str;
    TextFormat::PrintToString(log, &str);
    std::cout << str << std::endl;
    std::cout << std::flush;
}

void SpdTraceLogger::Log(const proto::RequestLog& log) {
    std::cout << "logging" << std::endl << std::flush;

    std::string buf;
    VERIFY(log.SerializeToString(&buf), "Could not serialize RequestLog proto");

    spdlog_->info("{:32X}{}", buf.size(), buf);
}

SpdTraceLoggerInstance::SpdTraceLoggerInstance() {
    const int queue_size = pow(2, 8);  // TODO find a good number here

    // spdlog::set_async_mode(queue_size,
    //                       spdlog::async_overflow_policy::block_retry,
    //                       nullptr,
    //                       std::chrono::seconds(2));
    auto spdlogger = spdlog::basic_logger_mt("request_logger", TRACE_LOG_PATH);
    // spdlog::set_sync_mode();  // Make sure other loggers are not async

    spdlogger->set_pattern("%v");
    logger_.reset(new instance(std::move(spdlogger)));
}

SpdTraceLoggerInstance::instance* SpdTraceLoggerInstance::get() {
    return logger_.get();
}
}
