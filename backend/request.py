import os
import time
from flask import Flask
import re
from flask_mail import Mail, Message

app = Flask(__name__)
# app.run(host='localhost', port=5000, request_handler=runtime.MyFancyRequestHandler)

app.config.update(
		DEBUG=True,
		#EMAIL SETTINGS
		MAIL_SERVER='smtp.gmail.com',
		MAIL_PORT=465,
		MAIL_USE_SSL=True,
		MAIL_USERNAME = 'smartseating445@gmail.com',
		MAIL_PASSWORD = 'GyH-Rb2-cmY-d9e'
	)

# Create mail instance
mail = Mail(app)

contactCard_neel = {'name': 'Neel Mouleeswaran', 'email': 'moulees2@illinois.edu'}
contactCard_srini = {'name': 'Srini Srikumar', 'email': 'srikuma2@illinois.edu'}
contactCard_mitch = {'name': 'Mitchell Appelbaum', 'email': 'appelbm2@illinois.edu'}
contactCard_brady = {'name': 'Brady Salz', 'email': 'salz2@illinois.edu'}

users = {	'0A': contactCard_neel,		'0B': contactCard_srini,	'0C': contactCard_mitch,	'0D': contactCard_brady,
			'1A': contactCard_neel,		'1B': contactCard_srini,	'1C': contactCard_mitch,	'1D': contactCard_brady,
			'2A': contactCard_neel,		'2B': contactCard_srini,	'2C': contactCard_mitch,	'2D': contactCard_brady,	
			'3A': contactCard_neel,		'3B': contactCard_srini,	'3C': contactCard_mitch,	'3D': contactCard_brady
		}

checked_in_users = []

status_string = ['(no student)', '(no student)', '(no student)', '(no student)']

@app.route('/')
def health():
    return 'OK'

@app.route('/dash')
def statusStringToRes():
	return 'Bench 0: ' + status_string[0] + "\n\n" + 'Bench 1: ' + status_string[1] + "\n\n" + 'Bench 2: ' + status_string[2] + "\n\n" + 'Bench 3: ' + status_string[3]

@app.route('/read/<data>')
def read(data):
	global status_string
	global checked_in_users
	if str(data) in checked_in_users:
		return 'Success'
	elif str(data) not in checked_in_users:
		if str(data) in users:
			checked_in_users.append(data)
			# print users
			# print data
			user = users[data]
			status_string[int(data[0])] = user['name']
			msg = Message("Hi " + user['name'] + ", you checked into eceB-2070, bench #" + data[0] + "!",
							sender="smartseating445@gmail.com",
							recipients=[user['email']])

			msg.body = getMessageBodyForUser(user, data[0])
			mail.send(msg)
			return 'Success'
		else:
			return 'Success'
		
	else:
		return 'Failure'

@app.route('/reset')
def reset():
	global checked_in_users
	checked_in_users = []
	global status_string
	status_string = ['(no student)', '(no student)', '(no student)', '(no student)']
	return 'Success'