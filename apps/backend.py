import requests

from flask import Flask
app = Flask(__name__)

import logging
log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)

@app.route("/")
def hello():
    r = requests.get('http://localhost:3131')
    return r.text

if __name__ == "__main__":
    app.run()

