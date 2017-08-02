#pragma once

#include <memory>
#include <mutex>

#include <thrift/transport/TSocket.h>

#include "gen-cpp/Collector.h"
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

class ThriftLogger : public TraceLogger {
   public:
    const static int COLLECTOR_PORT = 9934;

    ThriftLogger();

    void Log(const proto::RequestLog& log) override;

   private:
    std::mutex mu_;

    static const int max_size_ = 200;
    std::vector<std::string> buffer_;

    /*
     * Indicates if we have connected to the Collector
     */
    bool connected_;

    boost::shared_ptr<apache::thrift::transport::TTransport> transport_;

    std::unique_ptr<CollectorClient> client_;
};

class ThriftLoggerInstance {
   public:
    typedef ThriftLogger instance;

    ThriftLoggerInstance();
    instance* get();

   private:
    std::unique_ptr<instance> logger_;
};
}
