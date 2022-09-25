from http.server import HTTPServer, BaseHTTPRequestHandler
import json


class MyHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        # read the content-length header
        content_length = int(self.headers.get("Content-Length"))
        # read that many bytes from the body of the request
        body = self.rfile.read(content_length)

        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()
        # echo the body in the response
        print("Headers: %s" % (self.headers))
        print("Body: %s" % (body))

        try:
            data = json.loads(body)
            print("{}".format(data))
        except:
            pass

        resp_msg = {
            'code': 0,
            'message': 'success',
        }
        self.wfile.write(str.encode(json.dumps(resp_msg)))

    def do_GET(self):
        print("get request path '%s'" % (self.path))
        self.wfile.write(json.dumps(
            {'hello': 'world', 'received': 'ok'}))


httpd = HTTPServer(('localhost', 8071), MyHandler)
httpd.serve_forever()
