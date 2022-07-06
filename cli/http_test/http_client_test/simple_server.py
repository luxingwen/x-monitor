from http.server import HTTPServer, BaseHTTPRequestHandler, SimpleHTTPRequestHandler
import simplejson


class MyHandler(SimpleHTTPRequestHandler):
    def do_POST(self):
        # read the content-length header
        content_length = int(self.headers.get("Content-Length"))
        # read that many bytes from the body of the request
        body = self.rfile.read(content_length)

        self.send_response(200)
        self.end_headers()
        # echo the body in the response
        print("Headers: %s" % (self.headers))
        print("Body: %s" % (body))

        try:
            data = simplejson.loads(body)
            print("{}".format(data))
        except:
            pass
        self.wfile.write(body)

    def do_GET(self):
        print("get request path '%s'" % (self.path))
        return SimpleHTTPRequestHandler.do_GET(self)


httpd = HTTPServer(('localhost', 8071), MyHandler)
httpd.serve_forever()
