#!/usr/bin/env python

import tornado.httpserver
import tornado.ioloop
import tornado.options
import tornado.web

from tornado import gen
from tornado.httpclient import AsyncHTTPClient
from tornado.options import define, options

define("port", default=8888, help="run on the given port", type=int)

http_client = AsyncHTTPClient()

class MainHandler(tornado.web.RequestHandler):
    @gen.coroutine
    def get(self):
	response = yield http_client.fetch("http://www.google.com/")
        self.write(response.body)


def main():
    tornado.options.parse_command_line()
    application = tornado.web.Application([
        (r"/", MainHandler),
    ])
    http_server = tornado.httpserver.HTTPServer(application)
    http_server.listen(options.port)
    tornado.ioloop.IOLoop.current().start()


if __name__ == "__main__":
    main()
