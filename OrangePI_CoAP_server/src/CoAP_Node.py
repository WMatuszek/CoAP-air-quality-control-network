__author__ = 'Witold'

import threading
from time import time
from collections import namedtuple

from coapthon.client.helperclient import HelperClient
from defines import NODE_LIFETIME_SECONDS, NODE_DATA_REFRESH_INTERVAL_SECONDS


class Node(object):

    ResourceStruct_t = namedtuple("ResourceStruct_t", "uri name rt ct")

    DISCOVER_TIMEOUT = 10
    REFRESH_TIMEOUT = 10

    def __init__(self, ip, port, is_active=True):
        self.ip = ip
        self.port = port
        self.active = is_active
        self.coap_client = HelperClient(server=(ip, port))
        self.resources = {}
        # TODO named tuple with resource age
        self.discover_successful = False

        self._refresh_thread = None
        self._discover_thread = None

        self.__last_seen = time()
        self.__last_refresh = 0

    """
        Check for nodes with the same ip:port pair
    """
    @staticmethod
    def is_same_node(a, b):
        if (a.ip == b.ip) and (a.port == b.port):
            return True
        return False

    """
        Get client object for given node
    """
    def get_coap_client(self):
        return self.coap_client

    """
        Try run discover thread
    """
    def try_discover(self):
        # Check if thread still running
        if self._discover_thread is not None:
            if self._discover_thread.is_alive():
                return False
        self._debug("try discover")
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
        return not self.refresh_needed()

    """
        Try run refresh thread
    """
    def try_refresh(self):
        # Check if thread still running
        if self._refresh_thread is not None:
            if self._refresh_thread.is_alive():
                return False
        self._debug("try refresh")
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
        Check if node data refresh needed, if discover successful
    """
    def refresh_needed(self):
        # check if discover successful
        if not self.discover_successful:
            return False
        # check if refresh thread already running
        if self._refresh_thread is not None:
            if self._refresh_thread.is_alive():
                return False
        # check if time since last refresh above max
        if time() - self.__last_refresh > NODE_DATA_REFRESH_INTERVAL_SECONDS:
            return True
        return False

    """
        Node refresh thread method
    """
    def _refresh(self):
        for uri in self.resources:
            self._debug("try refresh res=" + uri)
            response = self.coap_client.get(uri, timeout=self.REFRESH_TIMEOUT)
            self.resources[uri] = response.payload
        self._refreshed()

    """
        Node discover resources thread method
    """
    def _discover(self):
        from defines import refreshable_resources

        response = self.coap_client.discover(timeout=self.DISCOVER_TIMEOUT)  # TODO how to check timeout?
        resources = []
        for resp in response.payload.split(','):
            resp = resp.split(';')
            if len(resp) >= 3:
                resources.append(self.ResourceStruct_t(resp[0], resp[1], resp[2], resp[3]))

        for res in resources:
            if res.name in refreshable_resources:
                self._debug("add resource " + res.uri)
                self.resources.update({res.uri[1:-1]: None})  # strip <>

        self.discover_successful = True

    """
        Update refresh timer
    """
    def _refreshed(self):
        self.__last_refresh = time()
        self.__last_seen = self.__last_refresh

    def _debug(self, msg):
        print "Node " + self.ip + "/" + str(self.port) + ": " + msg