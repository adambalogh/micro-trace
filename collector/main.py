import glob
import sys
sys.path.append('gen-py')

from log import Collector

from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.server import TServer

UNIX_SOCKET = '/tmp/microtrace.thrift'

class CollectorHandler:
    def Collect(self, log):
        print(log)

if __name__ == '__main__':
    handler = CollectorHandler()
    processor = Collector.Processor(handler)
    transport = TSocket.TServerSocket(unix_socket=UNIX_SOCKET)
    tfactory = TTransport.TBufferedTransportFactory()
    pfactory = TBinaryProtocol.TBinaryProtocolFactory()
    server = TServer.TSimpleServer(processor, transport, tfactory, pfactory)

    print('Starting the server...')
    server.serve()
    print('done.')

