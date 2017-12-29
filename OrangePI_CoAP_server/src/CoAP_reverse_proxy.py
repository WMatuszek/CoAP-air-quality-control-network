__author__ = 'Witold'

import coapthon.reverse_proxy.coap

"""
    CoAP reverse proxy server (+ node report)
"""
class CoAPRevProxyServer(coapthon.reverse_proxy.coap.CoAP):
    def __init__(self, host, port):
        from defines import REV_PROXY_NODES_XML_PATH
        print "Make reverse proxy with ip/port: " + host + "/" + str(port)

        self._known_nodes = []
        self._candidate_nodes = []

        coapthon.reverse_proxy.coap.CoAP.__init__(self, (host, port), REV_PROXY_NODES_XML_PATH)

    def add_proxied_node(self, node_info):
        """
        Add node to proxy.

        :param node_info: IP/port/name tuple identifying node
        """
        self._candidate_nodes.append(node_info)

    def _toDebug(self, msg):
        print "Reverse proxy " + self.server_address[0] + " : " + msg

    def listen(self, timeout=10):
        import socket
        """
        Listen for incoming messages. Timeout is used to check if the server must be switched off.
        Overriden for proxied node add inbetween UDP listen

        :param timeout: Socket Timeout in seconds
        """
        self._socket.settimeout(float(timeout))
        while not self.stopped.isSet():

            # try add node to proxy
            for node in self._candidate_nodes:
                if node not in self._known_nodes:
                    self._toDebug("discover node " + str(node))
                    self.discover_remote(node[0], node[1])
                    self._known_nodes.append(node)
            del self._candidate_nodes[:]

            try:
                data, client_address = self._socket.recvfrom(4096)
            except socket.timeout:
                continue
            try:

                self.receive_datagram((data, client_address))
            except RuntimeError:
                print "Exception with Executor"
        self._socket.close()
