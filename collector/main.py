import glob
import sys
sys.path.append('gen-py')

from log import Collector

from thrift.transport import TSocket
from thrift.transport import TTransport
from thrift.protocol import TBinaryProtocol
from thrift.server import TServer
from thrift import TTornado

from tornado import ioloop
from tornado.netutil import bind_unix_socket

UNIX_SOCKET = '/tmp/microtrace.thrift'

class CollectorHandler:
    def Collect(self, log):
        print(log)

if __name__ == '__main__':
    handler = CollectorHandler()
    processor = Collector.Processor(handler)
    pfactory = TBinaryProtocol.TBinaryProtocolFactory()
    server = TTornado.TTornadoServer(processor, pfactory)

    print('Starting the server...')
    socket = bind_unix_socket(UNIX_SOCKET)
    server.add_socket(socket)
    server.start(1)
    ioloop.IOLoop.instance().start()
    server.serve()
    print('done.')

