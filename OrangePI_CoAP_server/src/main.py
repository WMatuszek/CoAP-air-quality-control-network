import threading
import time
import sqlite3
from collections import namedtuple

from coapthon.client.helperclient import HelperClient
from coapthon.server.coap import CoAP
from coapthon.defines import COAP_DEFAULT_PORT

from CoAP_Server_thread import ServerThread
from CoAP_Client_thread import ClientThread
from resource_test import TestResource

default_host = "192.168.42.1"
default_port = COAP_DEFAULT_PORT

used_resources = ("temperature", "pressure", "pm")

shared_resource_uri = 'node_report/'
shared_resource_i = TestResource()


class CoAPServer(CoAP):
    def __init__(self, host, port):
        CoAP.__init__(self, (host, port))
        self.add_resource(shared_resource_uri, shared_resource_i)


class CoAPClient(CoAP):

    ResourceStruct_t = namedtuple("ResourceStruct_t", "uri path rt ct")

    default_resources = {
        "temperature": "temperature",
        "pressure": "pressure",
        "node_info": "info",
        "air_data": "pm"
    }

    def __init__(self, host, port, shared_res=TestResource()):
        self.shared_resource = shared_res
        self.client = HelperClient(server=(host, port))
        self.known_nodes = []

        self.db_ctrl = object #TODO sqlite3 db access

    def loop(self):
        nodes = self.shared_resource.get_connected_nodes_copy()
        print "Nodes" + str(nodes)
        for node in nodes:
            if any(node is known_node for known_node in self.known_nodes):
                self.__discover_node(node)

            print self.__get_from_node(node)

    def __discover_node(self, target):
        response = self.client.discover()
        #TODO parse response, put into Node() struct
        return response

    def __get_from_node(self, target):
        response = self.client.get(self.default_resources["temperature"])
        return response.payload

    def stop(self):
        self.client.stop()


def main():
    server = CoAPServer(default_host, default_port)
    server_thread = ServerThread(server)

    client = CoAPClient(default_host, default_port, shared_resource_i)
    client_thread = ClientThread(client)

    try:
        print "Listen..."
        server_thread.start()
        client_thread.start()

        while True:
            time.sleep(1)

    except KeyboardInterrupt:
        print "Server Shutdown"
        server_thread.stop()
        client_thread.stop()
        server_thread.join()
        client_thread.join()
        print "Exiting..."

    exit(1)


if __name__ == '__main__':
    main()

# client = HelperClient(server=(target, COAP_DEFAULT_PORT))
# response = client.discover()
# resources = []
#
# for res in response.payload.split(','):
# res = res.split(';')
#     if len(res) >= 3:
#         resources.append(ResourceStruct_t(res[0], res[1], res[2], res[3]))
#
# for res in resources:
#     if res.path in used_resources:
#         print "Node " + host + " resource " + res.path + " GET: "
#         response = client.get(res.path)
#         print response.payload
#
# client.stop()
