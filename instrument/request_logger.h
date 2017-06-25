#pragma once

#include <memory>

#include "spdlog/spdlog.h"

#include "request_log.pb.h"

namespace microtrace {

class RequestLogger {
   public:
    virtual ~RequestLogger() = default;

    virtual void Log(const proto::RequestLog& log) = 0;
};

/*
 * Discards logs. Should only be used for testing or debugging.
 */
class NullRequestLogger : public RequestLogger {
   public:
    void Log(const proto::RequestLog& log) override {}
};

/*
 * Logs to stdout. Generally, it should only be used for testing.
 */
class StdoutRequestLogger : public RequestLogger {
   public:
    void Log(const proto::RequestLog& log) override;
};

class SpdRequestLogger : public RequestLogger {
   public:
    void Log(const proto::RequestLog& log) override;

   private:
    std::shared_ptr<spdlog::logger> spdlog_;
};
}
