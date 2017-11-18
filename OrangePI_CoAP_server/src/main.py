import threading
import time

from coapthon.client.helperclient import HelperClient
from coapthon.server.coap import CoAP
from coapthon.defines import COAP_DEFAULT_PORT
from coapthon.transaction import Transaction
from resource_test import TestResource


from collections import namedtuple

ResourceStruct = namedtuple("ResourceStruct", "uri path rt ct")

used_resources = ("temperature", "pressure", "pm")

test_resource_uri = 'test/'
test_resource_instance = TestResource();

class CoAPServer(CoAP):
    def __init__(self, host, port):
        CoAP.__init__(self, (host, port))
        self.add_resource(test_resource_uri, test_resource_instance)

run = True
def resource_monitor():
    while run:
        time.sleep(2)
        print test_resource_instance.connected_nodes

def main():
    target = "192.168.0.165"
    #host = "0.0.0.0"
    host = "192.168.0.164"
    server = CoAPServer(host, COAP_DEFAULT_PORT)
    try:
        print "Listen..."
        t = threading.Thread(target=server.listen)
        t.start()
        t2 = threading.Thread(target=resource_monitor)
        t2.start()

    except KeyboardInterrupt:
        print "Server Shutdown"
        server.close()
        run = False
        t.join()
        t2.join()
        print "Exiting..."

if __name__ == '__main__':
    main()

# client = HelperClient(server=(target, COAP_DEFAULT_PORT))
# response = client.discover()
# resources = []
#
# for res in response.payload.split(','):
#     res = res.split(';')
#     if len(res) >= 3:
#         resources.append(ResourceStruct(res[0], res[1], res[2], res[3]))
#
# for res in resources:
#     if res.path in used_resources:
#         print "Node " + host + " resource " + res.path + " GET: "
#         response = client.get(res.path)
#         print response.payload
#
# client.stop()
