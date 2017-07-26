from flask import Flask
import requests
import urllib3

THIRD_URL = 'http://third-app-service.default:8080'

app = Flask(__name__)

http = urllib3.PoolManager()


@app.route('/')
def hello_world():
    r = http.request('GET', THIRD_URL)
    r = http.request('GET', THIRD_URL)
    return r.data
