import unittest
import request
import requests
import flask.ext.testing
import ujson
from stopwatch import clockit

base_url = "http://smart-seating.herokuapp.com"
read_endpt = '/read/'
update_endpt = '/update/'

@clockit
def getServerHealth():
	query = '/'
	return requests.get(base_url + query)

@clockit
def getReadResponse(query):
	return requests.get(base_url + read_endpt + query)

class RequestTestCase(unittest.TestCase):

	def setUp(self):
		self.app = request.app.test_client()

	def testServerHealthResponse(self):
		'''	5.03(1) a. Server responds to ping requests within 1000ms (1s)
			b. /health route returns JSON response with "OK"'''
		response = getServerHealth()
		self.assertEquals(response.status_code, 200)
		self.assertEquals(response.content, 'OK')

	def testReadResponse(self):
		'''	5.03(2) a. POST request to /read returns JSON response (< 5s)
			5.03(5) All unit tests pass for validating RFID strings from a prox card'''

		# This will fail as there is a malformed RFID str, netid, room_id
		response = getReadResponse(query='a|b|c')
		print 'a|b|c'
		self.assertEquals(response.status_code, 200)
		self.assertEquals(response.content, response.content)
		
		# Test strings to validate
		test_rfids = [	'2b ca 61 49 b7 63 2b 7f d2 ed 8f be e4 97 cb 27',
			  			'26 46 c1 0c 3a 96 e2 9a 01 77 c1 ef c1 d4 80 a8',
			  			'a1 9d fe d8 31 f3 a3 3b 7c 23 82 04 bd a2 ca dd',
			  			'b8 63 67 8f cd 1a 7d 16 7e a7 58 60 cd 54 cb 94',
			  			'c3 b7 c5 27 29 ae 75 21 8c bf 48 bf c6 3e c4 72',
			  			'0a b2 c5 f5 60 2c c3 fc 0e e0 7e b1 37 9f ca 1f',
			  			'bc 90 c8 c7 dd fd 07 9c 03 5a 92 4e 67 15 23 ee'	]

		# This should work, the RFID string is a test hex str
		for rfid_str in test_rfids:
			response = getReadResponse(query=('%s|moulees2|eceb_2070' % rfid_str))
			print rfid_str
			self.assertEquals(response.status_code, 200)
			self.assertEquals(response.content, response.content)

		# Test the malformed RFID str
		test_bad_rfid = 'ab cd ef gh ij kl mn op qr st uv wx yz 01 23 45'

		response = getReadResponse(query=('%s|moulees2|eceb_2070' % test_bad_rfid))
		print test_bad_rfid
		self.assertEquals(response.status_code, 200)
		self.assertEquals(response.content, response.content)

if __name__ == '__main__':
    unittest.main()