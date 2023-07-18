from time import sleep
from flask import Flask
app = Flask(__name__)

@app.route('/hello/<int:wait_time>')
def hello(wait_time):
    sleep(wait_time)
    return 'hello'

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=31001)