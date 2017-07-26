#include "trace_logger.h"

#include <math.h>
#include <iostream>
#include <mutex>
#include <sstream>

#include "google/protobuf/text_format.h"

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include "common.h"
#include "gen-cpp/Collector.h"

using ::google::protobuf::TextFormat;

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

namespace microtrace {

const int COLLECTOR_PORT = 4138;

void StdoutTraceLogger::Log(const proto::RequestLog& log) {
    std::string str;
    TextFormat::PrintToString(log, &str);
    std::cout << str << std::endl;
    std::cout << std::flush;
}

ThriftLogger::ThriftLogger() : connected_(false) {
    boost::shared_ptr<TTransport> socket(
        new TSocket("localhost", COLLECTOR_PORT));
    transport_.reset(new TBufferedTransport(socket));
    boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport_));
    client_.reset(new CollectorClient(protocol));
}

void ThriftLogger::Log(const proto::RequestLog& log) {
    std::string str;
    log.SerializeToString(&str);

    {
        std::unique_lock<std::mutex> l(mu_);
        buffer_.push_back(std::move(str));
        if (buffer_.size() > max_size_) {
            if (!connected_) {
                transport_->open();
                connected_ = true;
            }
            client_->Collect(buffer_);
            buffer_.clear();
        }
    }
}

ThriftLoggerInstance::ThriftLoggerInstance() : logger_(new ThriftLogger) {}

ThriftLoggerInstance::instance* ThriftLoggerInstance::get() {
    return logger_.get();
}
}
