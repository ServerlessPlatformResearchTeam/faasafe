#!/usr/bin/env python3
from http.server import BaseHTTPRequestHandler,HTTPServer

PORT_NUMBER = 8080

#This class will handles any incoming request from
#the browser
class myHandler(BaseHTTPRequestHandler):

	#Handler for the GET requests
	def do_GET(self):
		self.send_response(200)
		self.send_header('Content-type','text/html')
		self.end_headers()
		# Send the html message
		self.wfile.write("Hello World !")
		return

input1 = input("Welcome to our http server!Type something to test this out: \n")
print("Thank you for your input: \n", input1, flush=True)

try:
	#Create a web server and define the handler to manage the
	#incoming request
        print ("Starting server...", flush=True)
        server = HTTPServer(('', PORT_NUMBER), myHandler)
        print ('Started httpserver on port ', PORT_NUMBER, flush=True)
        #Wait forever for incoming htto requests
        server.serve_forever()

except KeyboardInterrupt:
	print ('^C received, shutting down the web server')
	server.socket.close()

