#pragma once

#include <memory>

#include "request_log.pb.h"

namespace microtrace {

template <class LogImpl>
class BufferedLogger;
class FileLogger;

typedef BufferedLogger<FileLogger> BufferedFileLogger;

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

// TODO think about thread-safety
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
template <class LogImpl>
class BufferedLogger : public Logger {
   public:
    static const size_t DEFAULT_MAX_SIZE = 1000 * 10;

    BufferedLogger(LogImpl log_impl, size_t max_size = DEFAULT_MAX_SIZE)
        : max_size_(max_size), size_(0), log_impl_(std::move(log_impl)) {}

    BufferedLogger(const BufferedLogger&) = delete;
    BufferedLogger& operator=(const BufferedLogger&) = delete;

    void Log(const proto::RequestLog& log) override;

    void Flush();

   private:
    /*
     * Max size of the buffer in bytes.
     */
    const size_t max_size_;

    /*
     * Current size of the buffer in bytes.
     */
    size_t size_;

    std::vector<proto::RequestLog> buffer_;

    /*
     * Logger instance that actually does the logging.
     */
    LogImpl log_impl_;
};

template <class LogImpl>
void BufferedLogger<LogImpl>::Log(const proto::RequestLog& log) {
    // TODO maybe use a shared_ptr to avoid copying the whole object
    buffer_.push_back(log);
    size_ += log.ByteSizeLong();

    if (size_ > max_size_) {
        Flush();
    }
}

template <class LogImpl>
void BufferedLogger<LogImpl>::Flush() {
    for (const auto& log : buffer_) {
        log_impl_.Log(log);
    }

    buffer_.clear();
    size_ = 0;
}
}
