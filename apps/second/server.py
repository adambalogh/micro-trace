import cherrypy
import requests

cherrypy.config.update({
    'server.socket_port': 5001,
    'server.socket_host': '0.0.0.0'
})


class HelloWorld(object):
    @cherrypy.expose
    def index(self):
        r = requests.get('http://third:3131')
        return r.text

if __name__ == '__main__':
    cherrypy.quickstart(HelloWorld())
