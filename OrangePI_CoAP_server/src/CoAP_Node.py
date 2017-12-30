__author__ = 'Witold'

import logging.config
import threading
import json
import os
from time import time

from coapthon.client.helperclient import HelperClient
from coapthon.utils import create_logging

from defines import NODE_LIFETIME_SECONDS, NODE_DATA_REFRESH_INTERVAL_SECONDS

if not os.path.isfile("logging.conf"):
    create_logging()

logger = logging.getLogger(__name__)
logging.config.fileConfig("logging.conf", disable_existing_loggers=False)


class NodeResource(object):
    """
    Simple CoAP resource representation
    """

    def __init__(self, uri, name, attributes, server, refreshable=False, observable=False):
        if uri[0] == '<' and uri[-1] == '>':
            uri = uri[1:-1]

        self.uri = uri
        self.name = name
        self.value = None

        self._refreshable = refreshable
        self._observable = observable
        self._attributes = attributes
        self._server = server

        self._coap_client = None
        self._last_observe_response = None
        self._last_update = 0

        self._start_observe()

    def __del__(self):
        if self._coap_client is not None:
            self._coap_client.cancel_observe(self._last_observe_response, False)

    def update_value(self, val):
        self.value = val
        self._last_update = time()
        self._to_log("updated with " + self.value)

    def is_refreshable(self):
        return self._refreshable

    def is_observed(self):
        return self._observable

    def is_fresh(self, max_age):
        return (time() - self._last_update) > max_age

    def _start_observe(self):
        if self._observable:
            self._coap_client = HelperClient(server=self._server)
            self._coap_client.observe(self.uri, self._observe_callback)
            self._to_log("start observe")

    def _observe_callback(self, response):
        self._last_observe_response = response
        self.update_value(response.payload)

    def _to_log(self, msg):
        logger.info("Node=" + str(self._server) + " res=" + self.name + ": " + msg)


class Node(object):
    """
    CoAP sensor node representation
    """

    DISCOVER_TIMEOUT = 10
    REFRESH_TIMEOUT = 10
    NODE_INFO_TIMEOUT = 10

    def __init__(self, ip, port, is_active=True):
        self.ip = ip
        self.port = port
        self.info = "Node:" + self.ip
        self.active = is_active
        self.discover_successful = False

        self.resources = []
        #self.coap_client = HelperClient(server=(self.ip, self.port))

        self._refresh_thread = None
        self._discover_thread = None

        self.__last_seen = time()
        self.__last_refresh = 0

    def __del__(self):
        self.stop_threads()

    def __eq__(self, other):
        if isinstance(other, Node):
            return (self.ip == other.ip) and (self.port == other.port)
        return False

    def __ne__(self, other):
        return not self.__eq__(other)

    """
        Stop node threads
    """
    def stop_threads(self):
        if self._discover_thread is not None:
            if self._discover_thread.is_alive():
                self._discover_thread.join()
        if self._refresh_thread is not None:
            if self._refresh_thread.is_alive():
                self._refresh_thread.join()

    """
        Try run discover thread
    """
    def try_discover(self):
        # Check if thread still running
        if self._discover_thread is not None:
            if self._discover_thread.is_alive():
                return False
        self._to_log("try discover")
        self._discover_thread = threading.Thread(target=self._discover)
        self._discover_thread.start()
        return True

    """
        Check for node data refresh thread done
        @:returns False if refresh thread running, else return if refresh needed
    """
    def is_refresh_done(self):
        if self._refresh_thread is not None:
            return not self._refresh_thread.is_alive()
        return True

    """
        Try run refresh thread
    """
    def try_refresh(self):
        # Check if thread still running
        if self._refresh_thread is not None:
            if self._refresh_thread.is_alive():
                return False
        self._refresh_thread = threading.Thread(target=self._refresh)
        self._refresh_thread.start()
        return True

    """
        Is node alive, ie. time since last communication did not exceed NODE_LIFETIME_SECONDS
    """
    def is_node_alive(self):
        if self.active:
            if time() - self.__last_seen > NODE_LIFETIME_SECONDS:
                self.active = False
        return self.active

    """
        Node active setter
    """
    def set_alive(self):
        self.active = True
        self.__last_seen = time()

    """
        Node discover resources thread method
    """
    def _discover(self):
        import defines

        coap_client = HelperClient(server=(self.ip, self.port))
        response = coap_client.discover(timeout=self.DISCOVER_TIMEOUT)
        if not response.payload:
            return   # Return on empty response
        for resp in response.payload.split(','):
            resp = resp.split(';')
            if len(resp) < 2:
                continue  # Skip entries without name
            uri = resp[0]
            attributes = resp[1:]
            name = Node._get_name_from_attributes(attributes)

            # Get node info resource
            if name == defines.known_resources["node_info"]:
                self._to_log("getting node info")
                resp = coap_client.get(uri, timeout=self.NODE_INFO_TIMEOUT)
                self._to_log("received " + str(self.info))
                if resp.payload:
                    self.info = resp.payload
                continue

            # Add resource if refreshable
            if name not in defines.ignored_resources:
                refresh = name in defines.refreshable_resources
                observe = name in defines.observed_resources
                res = NodeResource(uri, name, attributes, (self.ip, self.port),
                                   refreshable=refresh,
                                   observable=observe)
                self.resources.append(res)
                self._to_log("added resource " + name + " o=" + str(observe) + " r=" + str(refresh))

        self.discover_successful = True
        coap_client.close()

    """
        Node refresh thread method
    """
    def _refresh(self):
        coap_client = HelperClient(server=(self.ip, self.port))
        refresh_cnt = 0
        for res in self.resources:
            if res.is_refreshable():
                if res.is_fresh(NODE_DATA_REFRESH_INTERVAL_SECONDS):
                    self._to_log("try refresh res=" + res.uri)
                    response = coap_client.get(res.uri, timeout=self.REFRESH_TIMEOUT)
                    res.update_value(response.payload)
                    refresh_cnt += 1
        if refresh_cnt > 0:
            coap_client.close()

    """
        Update refresh timer
    """
    def _refreshed(self):
        self.__last_refresh = time()
        self.__last_seen = time()

    def _to_log(self, msg):
        logger.info("Node " + self.ip + "/" + str(self.port) + ": " + msg)

    @staticmethod
    def _get_name_from_attributes(attributes):
        for attr in attributes:
            if attr.startswith('if='):
                return attr[attr.find('\"') + 1:attr.rfind('\"')]
        return "Unknown"