#pragma once

#include <memory>

#include "request_log.pb.h"

class Logger {
   public:
    virtual ~Logger() = default;

    virtual void Log(const proto::RequestLog& log) = 0;
};

/*
 * Doesn't log. Should only be used for testing.
 */
class NullLogger : public Logger {
   public:
    void Log(const proto::RequestLog& log) override {}
};

/*
 * Logs to stdout. Generally, it should only be used for testing.
 */
class StdoutLogger : public Logger {
   public:
    void Log(const proto::RequestLog& log) override;
};

/*
 * Logs to a given file.
 */
class FileLogger : public Logger {
   public:
    FileLogger();

    FileLogger(const FileLogger&) = delete;
    FileLogger& operator=(const FileLogger&) = delete;

    void Log(const proto::RequestLog& log) override;

   private:
};

/*
 * BufferedLogger buffers logs in memory in order to increase efficiency.
 *
 * It can be flushed manually and it is automatically flushed once the buffer is
 * full.
 */
template <typename LogImpl>
class BufferedLogger : public Logger {
   public:
    BufferedLogger(LogImpl log_impl) : log_impl_(std::move(log)) {}

    BufferedLogger(const BufferedLogger&) = delete;
    BufferedLogger& operator=(const BufferedLogger&) = delete;

    void Log(const proto::RequestLog& log) override;

   private:
    void Flush();

    LogImpl log_impl_;
};
