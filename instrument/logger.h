#pragma once

#include "request_log.pb.h"

class Logger {
   public:
    virtual ~Logger() = default;

    virtual void Log(const proto::RequestLog& log) = 0;
};

class NullLogger : public Logger {
   public:
    void Log(const proto::RequestLog& log) override {}
};

class StdoutLogger : public Logger {
   public:
    void Log(const proto::RequestLog& log) override;
};
