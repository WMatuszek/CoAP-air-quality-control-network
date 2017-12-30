import time

import CoAP_Node
import HTTP_endpoint
from CoAP_report_server import CoAPServer
from CoAP_reverse_proxy import CoAPRevProxyServer
from CoAP_Server_thread import ServerThread
from CoAP_Client_thread import ClientThread

class CoAPClientApp():
    """
        CoAP app main loop
    """

    def __init__(self, report_serv, rproxy=None):
        self._known_nodes = []

        self._report_server = report_serv
        self._rev_proxy = rproxy

    def loop(self):
        connected_nodes = self._report_server.get_connected_nodes()
        # Check for newly connected nodes
        for conn_node in connected_nodes:
            # Check connected node for duplicate
            ip = conn_node[0]
            port = conn_node[1]
            duplicate = False
            for known_node in self._known_nodes:
                if (ip == known_node.ip) and (port == known_node.port):
                    known_node.set_alive()
                    duplicate = True
            if not duplicate:
                # Add node to known nodes, discover resources, add to reverse proxy (if present)
                node = CoAP_Node.Node(ip, port)
                node.try_discover()
                self._known_nodes.append(node)
                # Try add new node reverse proxy TODO wait for node info GET successful
                if self._rev_proxy is not None:
                    address = node.ip + ":" + str(node.port)
                    self._rev_proxy.add_proxied_node((address, node.info))

        self._refresh_nodes()

    def get_nodes(self):
        return self._known_nodes

    def stop(self):
        for node in self._known_nodes:
            node.stop_threads()

    def _refresh_nodes(self):
        print "\nNode cnt = " + str(len(self._known_nodes))
        for node in self._known_nodes:
            print node.ip + " " + str(node.port) + ":" + \
                str([(res.name, res.value) for res in node.resources])
            if node.discover_successful:
                node.try_refresh()

"""
    MAIN
"""
def main():
    from defines import EXTERNAL_INTERFACE, INTERNAL_INTERFACE, DEFAULT_PORT
    from get_interface_ip import get_interface_ip

    rev_proxy_server = CoAPRevProxyServer(get_interface_ip(EXTERNAL_INTERFACE), DEFAULT_PORT)
    rev_proxy_thread = ServerThread(rev_proxy_server)

    report_server = CoAPServer(get_interface_ip(INTERNAL_INTERFACE), DEFAULT_PORT)
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
