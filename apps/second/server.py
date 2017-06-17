import cherrypy
import urllib3
import requests

cherrypy.config.update({
    'server.socket_port': 5001,
    'server.socket_host': '0.0.0.0'
})

THIRD_URL = 'http://third:3131'

http = urllib3.PoolManager()

class HelloWorld(object):
    @cherrypy.expose
    def index(self):
        r = http.request('GET', THIRD_URL)
        return r.data

if __name__ == '__main__':
    cherrypy.quickstart(HelloWorld())
