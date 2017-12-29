__author__ = 'Witold'

import coapthon.server.coap
import logging.config
import random
import socket
import struct
import threading

import os

from coapthon import defines
from coapthon.layers.blocklayer import BlockLayer
from coapthon.layers.messagelayer import MessageLayer
from coapthon.layers.observelayer import ObserveLayer
from coapthon.layers.requestlayer import RequestLayer
from coapthon.layers.resourcelayer import ResourceLayer
from coapthon.messages.message import Message
from coapthon.messages.request import Request
from coapthon.messages.response import Response
from coapthon.resources.resource import Resource
from coapthon.serializer import Serializer
from coapthon.utils import Tree, create_logging

if not os.path.isfile("logging.conf"):
    create_logging()

logger = logging.getLogger(__name__)
logging.config.fileConfig("logging.conf", disable_existing_loggers=False)

"""
    CoAP server (node report)
"""
class CoAPServer(coapthon.server.coap.CoAP):
    shared_resource_uri = 'node_report/'

    def __init__(self, host, port, shared_resource=None):
        print "Make server with ip/port: " + host + "/" + str(port)

        self._connected_nodes = set()

        coapthon.server.coap.CoAP.__init__(self, (host, port))

        if shared_resource is not None:
            self.add_resource(self.shared_resource_uri, shared_resource)

    def get_connected_nodes(self):
        return self._connected_nodes

    def listen(self, timeout=10):
        import socket
        """
        Listen for incoming messages. Timeout is used to check if the server must be switched off.\
        Overloaded for client IP access

        :param timeout: Socket Timeout in seconds
        """
        self._socket.settimeout(float(timeout))
        while not self.stopped.isSet():
            try:
                data, client_address = self._socket.recvfrom(4096)
                if len(client_address) > 2:
                    client_address = (client_address[0], client_address[1])

                # add client_address to connected nodes list if not duplicate
                if len(client_address) == 2:
                    self._connected_nodes.add(client_address)

            except socket.timeout:
                continue
            try:
                serializer = Serializer()
                message = serializer.deserialize(data, client_address)
                if isinstance(message, int):
                    logger.error("receive_datagram - BAD REQUEST")

                    rst = Message()
                    rst.destination = client_address
                    rst.type = defines.Types["RST"]
                    rst.code = message
                    rst.mid = self._messageLayer.fetch_mid()
                    self.send_datagram(rst)
                    continue

                logger.debug("receive_datagram - " + str(message))
                if isinstance(message, Request):
                    transaction = self._messageLayer.receive_request(message)
                    if transaction.request.duplicated and transaction.completed:
                        logger.debug("message duplicated, transaction completed")
                        if transaction.response is not None:
                            self.send_datagram(transaction.response)
                        continue
                    elif transaction.request.duplicated and not transaction.completed:
                        logger.debug("message duplicated, transaction NOT completed")
                        self._send_ack(transaction)
                        continue
                    args = (transaction, )
                    t = threading.Thread(target=self.receive_request, args=args)
                    t.start()
                # self.receive_datagram(data, client_address)
                elif isinstance(message, Response):
                    logger.error("Received response from %s", message.source)

                else:  # is Message
                    transaction = self._messageLayer.receive_empty(message)
                    if transaction is not None:
                        with transaction:
                            self._blockLayer.receive_empty(message, transaction)
                            self._observeLayer.receive_empty(message, transaction)

            except RuntimeError:
                print "Exception with Executor"
        self._socket.close()
