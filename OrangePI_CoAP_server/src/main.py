import threading
import time
import sqlite3
from collections import namedtuple

from coapthon.client.helperclient import HelperClient
from coapthon.server.coap import CoAP
from coapthon.defines import COAP_DEFAULT_PORT

import CoAP_Node
from CoAP_Server_thread import ServerThread
from CoAP_Client_thread import ClientThread
from resource_test import TestResource

import SimpleHTTPServer

default_host = "192.168.42.1"
default_port = COAP_DEFAULT_PORT

shared_resource_uri = 'node_report/'
shared_resource_i = TestResource()


class CoAPServer(CoAP):
    def __init__(self, host, port):
        CoAP.__init__(self, (host, port))
        self.add_resource(shared_resource_uri, shared_resource_i)


class CoAPClient(CoAP):
    ResourceStruct_t = namedtuple("ResourceStruct_t", "uri name rt ct")

    def __init__(self, shared_res=TestResource()):
        self.shared_resource = shared_res
        self.known_nodes = []

        self.db_ctrl = object  # TODO sqlite3 db access

    def loop(self):
        connected_nodes = self.shared_resource.get_connected_nodes_copy()
        print "Connected nodes: " + str(connected_nodes)

        for conn_node in connected_nodes:
            node = CoAP_Node.Node(ip=conn_node[0], port=conn_node[1])

            duplicate = False
            for known_node in self.known_nodes:
                if CoAP_Node.Node.is_same_node(node, known_node):
                    known_node.set_alive()
                    duplicate = True
            if not duplicate:
                node.try_discover()
                self.known_nodes.append(node)

        print "\nNode cnt = " + str(len(self.known_nodes))
        for node in self.known_nodes:
            print node.ip + " " + str(node.port) + ":" + str(node.resources)

            if node.refresh_needed():
                node.try_refresh()

    def stop(self):
        self.client.stop()


#
# MAIN
#
def main():
    server = CoAPServer(default_host, default_port)
    server_thread = ServerThread(server)

    client = CoAPClient(shared_resource_i)
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
