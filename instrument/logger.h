#pragma once

#include "request_log.pb.h"

class Logger {
   public:
    virtual ~Logger() = default;

    virtual void Log(const proto::RequestLog& log) = 0;
};

class StdoutLogger : public Logger {
   public:
    void Log(const proto::RequestLog& log) override;
};
