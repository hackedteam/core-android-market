from gevent import monkey, socket; monkey.patch_all()
import bson; bson.patch_socket()
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("127.0.0.1", 6954))
s.send(bson.dumps({u"cmd" : "HELO"}))
resp = s.recv(4096)
print "received: ", resp

b = bson.loads(resp)
print "decoded: ", b