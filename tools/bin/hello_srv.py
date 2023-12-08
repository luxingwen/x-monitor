from time import sleep
from http.server import BaseHTTPRequestHandler, HTTPServer

class MyHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-type', 'text/plain')
        self.end_headers()
        sleep(6)
        self.wfile.write(b'hello')

httpd = HTTPServer(('localhost', 8000), MyHandler)
httpd.serve_forever()