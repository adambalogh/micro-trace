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
class TraceLogger {
   public:
    virtual ~TraceLogger() = default;

    /*
     * Log should save the log so it can be collected later.
     *
     * It shouldn't keep a reference to the given log after the method returns.
     */
    virtual void Log(const proto::RequestLog& log) = 0;
};

/*
 * Discards logs. Should only be used for testing or debugging.
 */
class NullTraceLogger : public TraceLogger {
   public:
    void Log(const proto::RequestLog& log) override {}
};

/*
 * Logs to stdout. Generally, it should only be used for testing.
 */
class StdoutTraceLogger : public TraceLogger {
   public:
    void Log(const proto::RequestLog& log) override;
};

class SpdTraceLogger : public TraceLogger {
   public:
    SpdTraceLogger(std::shared_ptr<spdlog::logger> spdlog)
        : spdlog_(std::move(spdlog)) {}

    void Log(const proto::RequestLog& log) override;

   private:
    // must be a thread-safe version
    std::shared_ptr<spdlog::logger> spdlog_;
};

class SpdTraceLoggerInstance {
   public:
    typedef SpdTraceLogger instance;

    SpdTraceLoggerInstance();
    instance* get();

   private:
    std::unique_ptr<instance> logger_;
};
}
