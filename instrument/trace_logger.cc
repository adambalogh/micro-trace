#include "trace_logger.h"

#include <math.h>
#include <iostream>
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

const std::string COLLECTOR_UNIX_SOCKET = "/tmp/microtrace.thrift";

void StdoutTraceLogger::Log(const proto::RequestLog& log) {
    std::string str;
    TextFormat::PrintToString(log, &str);
    std::cout << str << std::endl;
    std::cout << std::flush;
}

ThriftLogger::ThriftLogger() {
    boost::shared_ptr<TTransport> socket(new TSocket(COLLECTOR_UNIX_SOCKET));
    boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
    boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    client_.reset(new CollectorClient(protocol));
    transport->open();
}

void ThriftLogger::Log(const proto::RequestLog& log) {
    std::string str;
    log.SerializeToString(&str);
    client_->Collect(str);
}

ThriftLoggerInstance::ThriftLoggerInstance() : logger_(new ThriftLogger) {}

ThriftLoggerInstance::instance* ThriftLoggerInstance::get() {
    return logger_.get();
}
}
