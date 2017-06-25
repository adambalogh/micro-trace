#include "request_logger.h"

#include <iostream>
#include <string>

#include <google/protobuf/text_format.h>

using namespace ::google::protobuf;

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
}