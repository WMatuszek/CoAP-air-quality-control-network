import threading
import time

import coapthon.reverse_proxy.coap
import coapthon.server.coap

import CoAP_Node
import HTTP_endpoint
from CoAP_report_server import CoAPServer
from CoAP_reverse_proxy import CoAPRevProxyServer
from CoAP_Server_thread import ServerThread
from CoAP_Client_thread import ClientThread
from resource_test import TestResource

shared_resource_i = TestResource()


"""
    CoAP app main loop
"""
class CoAPClientApp():

    def __init__(self, report_serv, shared_res=None, rproxy=None):
        self.known_nodes = []

        self.report_server = report_serv
        self.shared_resource = shared_res
        self.rev_proxy = rproxy

        if self.shared_resource is None and self.report_server is None:
            raise RuntimeError

    def loop(self):

        connected_nodes = self.report_server.get_connected_nodes()
        print "\nConnected nodes: " + str(connected_nodes)

        # Check for newly connected nodes
        for conn_node in connected_nodes:
            node = CoAP_Node.Node(ip=conn_node[0], port=conn_node[1])

            duplicate = False
            for known_node in self.known_nodes:
                if node == known_node:
                    known_node.set_alive()
                    duplicate = True
            if not duplicate:
                node.try_discover()
                self.known_nodes.append(node)
                # try add new node reverse proxy
                if self.rev_proxy is not None:
                    addr = node.ip + ":" + str(node.port)
                    self.rev_proxy.add_proxied_node((addr, node.info))

        # Print node data, refresh
        print "\nNode cnt = " + str(len(self.known_nodes))
        for node in self.known_nodes:
            print node.ip + " " + str(node.port) + ":" + \
                str([(res.name, res.value) for res in node.resources])
            node.try_refresh()

    def get_nodes(self):
        return self.known_nodes

    def stop(self):
        self.client.stop()


"""
    MAIN
"""
def main():
    from defines import EXTERNAL_INTERFACE, INTERNAL_INTERFACE, DEFAULT_PORT
    from get_interface_ip import get_interface_ip

    rev_proxy_server = CoAPRevProxyServer(get_interface_ip(EXTERNAL_INTERFACE), DEFAULT_PORT)
    rev_proxy_thread = ServerThread(rev_proxy_server)

    report_server = CoAPServer(get_interface_ip(INTERNAL_INTERFACE), DEFAULT_PORT, shared_resource_i)
    server_thread = ServerThread(report_server)

    client = CoAPClientApp(rproxy=rev_proxy_server, report_serv=report_server)
    client_thread = ClientThread(client)

    try:
        print "Listen..."
        server_thread.start()
        client_thread.start()
        rev_proxy_thread.start()

        HTTP_endpoint.install_coap_client_app_obj(client)
        HTTP_endpoint.flaskApp.run(host='0.0.0.0', port=8000)

        while True:
            time.sleep(1)

    except KeyboardInterrupt:
        print "Server Shutdown"
        server_thread.stop()
        client_thread.stop()
        rev_proxy_thread.stop()
        server_thread.join()
        client_thread.join()
        rev_proxy_thread.join()
        print "Exiting..."

    exit(1)


if __name__ == '__main__':
    main()
