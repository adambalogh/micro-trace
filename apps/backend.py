from flask import Flask
app = Flask(__name__)

import logging
log = logging.getLogger('werkzeug')
log.setLevel(logging.ERROR)

@app.route("/")
def hello():
    total = 0
    for i in range(10000):
        total += i
    return str(total)

if __name__ == "__main__":
    app.run()
