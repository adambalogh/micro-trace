#pragma once

#include <memory>

#include "request_log.pb.h"

namespace spdlog {
class logger;
}

namespace microtrace {

/*
 * Implementations should be thread-safe.
 */
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
    SpdRequestLogger(std::shared_ptr<spdlog::logger> spdlog)
        : spdlog_(std::move(spdlog)) {}

    void Log(const proto::RequestLog& log) override;

   private:
    // must be a thread-safe version
    std::shared_ptr<spdlog::logger> spdlog_;
};

class SpdRequestLoggerInstance {
   public:
    typedef SpdRequestLogger instance;

    SpdRequestLoggerInstance();
    instance* get();

   private:
    std::unique_ptr<instance> logger_;
};
}
