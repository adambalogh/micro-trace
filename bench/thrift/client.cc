#include <chrono>
#include <iostream>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include "Calculator.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using namespace std::chrono;

using boost::shared_ptr;

int main() {
    boost::shared_ptr<TTransport> socket(new TSocket("localhost", 8080));
    boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
    boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    CalculatorClient client(protocol);

    transport->open();

    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    for (int i = 0; i < 10000; ++i) {
        client.proxy_add(10, 9);
    }
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(t2 - t1).count();

    std::cout << duration << std::endl;
}
