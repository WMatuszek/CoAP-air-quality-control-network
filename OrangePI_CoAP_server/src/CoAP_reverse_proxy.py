__author__ = 'Witold'

import logging.config
import random
import socket
import struct
import threading
import xml.etree.ElementTree as ElementTree

import os
import re

from coapthon.reverse_proxy.coap import CoAP
from coapthon import defines
from coapthon.client.helperclient import HelperClient
from coapthon.layers.blocklayer import BlockLayer
from coapthon.layers.cachelayer import CacheLayer
from coapthon.layers.forwardLayer import ForwardLayer
from coapthon.layers.messagelayer import MessageLayer
from coapthon.layers.observelayer import ObserveLayer
from coapthon.layers.resourcelayer import ResourceLayer
from coapthon.messages.message import Message
from coapthon.messages.request import Request
from coapthon.resources.remoteResource import RemoteResource
from coapthon.resources.resource import Resource
from coapthon.serializer import Serializer
from coapthon.utils import Tree, create_logging

if not os.path.isfile("logging.conf"):
    create_logging()

logger = logging.getLogger(__name__)
logging.config.fileConfig("logging.conf", disable_existing_loggers=False)


class CoAPRevProxyServer(CoAP):
    """
    CoAP reverse proxy server (+ node report)
    """

    def __init__(self, host, port):
        from defines import REV_PROXY_NODES_XML_PATH
        CoAP.__init__(self, (host, port), REV_PROXY_NODES_XML_PATH, cache=True)

        print "Make reverse proxy with ip/port: " + host + "/" + str(port)
        self._known_nodes = []
        self._candidate_nodes = []

    def add_proxied_node(self, node_info):
        """
        Add node to proxy.
        :param node_info: ("IP:port",name) tuple identifying node
        """
        self._candidate_nodes.append(node_info)

    def parse_core_link_format(self, link_format, base_path, remote_server):
        """
        Parse discovery results.
        Overridden for
            ignore defined resource instances
            remove bad attribute keys in discovered resources

        :param link_format: the payload of the response to the discovery request
        :param base_path: the base path used to create child resources discovered on the remote server
        :param remote_server: the (ip, port) of the remote server
        """
        from defines import ignored_resources, resource_name_attribute

        while len(link_format) > 0:
            pattern = "<([^>]*)>;"
            result = re.match(pattern, link_format)
            path = result.group(1)
            path = path.split("/")
            path = path[1:][0]
            link_format = link_format[result.end(1) + 2:]
            pattern = "([^<,])*"
            result = re.match(pattern, link_format)
            attributes = result.group(0)
            dict_att = {}
            if len(attributes) > 0:
                attributes = attributes.split(";")
                for att in attributes:
                    a = att.split("=")
                    if len(a) > 1:
                        dict_att[a[0]] = a[1]
                    else:
                        dict_att[a[0]] = a[0]
                link_format = link_format[result.end(0) + 1:]
            # TODO handle observing

            # filter resources in ignored_resources
            if resource_name_attribute in dict_att.keys():
                if_val = dict_att[resource_name_attribute]
                if_val = if_val[1:-1] if if_val.startswith('\"') else if_val
                if if_val in ignored_resources:
                    logger.info("Ignoring res=" + str(path) + " at " + str(remote_server))
                    continue

            # remove bad keys
            for key in dict_att.keys():
                if key not in defines.corelinkformat.keys():
                    dict_att.pop(key)

            resource = RemoteResource('server', remote_server, path, coap_server=self, visible=True, observable=False,
                                      allow_children=True)
            resource.attributes = dict_att
            self.add_resource(base_path + "/" + path, resource)

        logger.info(self.root.dump())

    def listen(self, timeout=10):
        """
        Listen for incoming messages. Timeout is used to check if the server must be switched off.
        Overridden for proxied node add between UDP listen

        :param timeout: Socket Timeout in seconds
        """
        self._socket.settimeout(float(timeout))
        while not self.stopped.isSet():

            # try add node to proxy
            for node in self._candidate_nodes:
                if node not in self._known_nodes:
                    logger.info("discover node " + str(node))
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
