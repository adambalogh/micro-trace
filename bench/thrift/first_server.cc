#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TSocket.h>
#include "Calculator.h"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

class CalculatorHandler : virtual public CalculatorIf {
   public:
    CalculatorHandler() {
        // Your initialization goes here
        boost::shared_ptr<TTransport> socket(new TSocket("localhost", 8181));
        boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
        boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
        client_.reset(new CalculatorClient{protocol});
        transport->open();
    }

    int32_t proxy_add(const int32_t num1, const int32_t num2) {
        return client_->add(num1, num2);
    }

    int32_t add(const int32_t num1, const int32_t num2) { abort(); }

   private:
    shared_ptr<CalculatorClient> client_;
};

int main(int argc, char **argv) {
    int port = 8080;
    shared_ptr<CalculatorHandler> handler(new CalculatorHandler());
    shared_ptr<TProcessor> processor(new CalculatorProcessor(handler));
    shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    shared_ptr<TTransportFactory> transportFactory(
        new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    TSimpleServer server(processor, serverTransport, transportFactory,
                         protocolFactory);
    server.serve();
    return 0;
}
