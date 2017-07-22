from flask import Flask
import requests
import urllib3

THIRD_URL = 'http://third:3131'

app = Flask(__name__)

http = urllib3.PoolManager()


@app.route('/')
def hello_world():
    r = http.request('GET', THIRD_URL)
    r = http.request('GET', THIRD_URL)
    return r.data
