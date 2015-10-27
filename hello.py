import os
from flask import Flask

app = Flask(__name__)

@app.route('/')
def hello():
    return 'Hello World!'

@app.route('/read/<data>')
def read(data):
	return str(data.split('|'))

