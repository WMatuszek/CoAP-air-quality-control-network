__author__ = 'Witold'

import threading
import json
from time import time

from coapthon.client.helperclient import HelperClient
from defines import NODE_LIFETIME_SECONDS, NODE_DATA_REFRESH_INTERVAL_SECONDS


class NodeResource(object):
    def __init__(self, uri, name):
        self.uri = uri
        self.name = name
        self.value = None
        self.observed = False

        self._last_update = 0

    def update(self, val):
        self.value = val
        self._last_update = time()

    def is_fresh(self, max_age):
        return (time() - self._last_update) > max_age


class Node(object):

    DISCOVER_TIMEOUT = 10
    REFRESH_TIMEOUT = 10

    def __init__(self, ip, port, is_active=True):
        self.ip = ip
        self.port = port
        self.info = "Node:" + self.ip
        self.active = is_active
        self.coap_client = None
        self.resources = []
        self.discover_successful = False

        self._refresh_thread = None
        self._discover_thread = None

        self.__last_seen = time()
        self.__last_refresh = 0

    def __eq__(self, other):
        if isinstance(other, Node):
            return (self.ip == other.ip) and (self.port == other.port)
        return False

    def __ne__(self, other):
        return not self.__eq__(self, other)

    def __hash__(self):
        return hash((self.ip, self.port))

    """
        Check for nodes with the same ip:port pair
    """
    # @staticmethod
    # def is_same_node(a, b):
    #     if (a.ip == b.ip) and (a.port == b.port):
    #         return True
    #     return False

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
        return True

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
        Node refresh thread method
    """
    def _refresh(self):
        from defines import refreshable_resources

        self.coap_client = HelperClient(server=(self.ip, self.port))
        refresh_cnt = 0
        for res in self.resources:
            if res.name in refreshable_resources:
                if res.is_fresh(NODE_DATA_REFRESH_INTERVAL_SECONDS):
                    self._debug("try refresh res=" + res.uri)
                    response = self.coap_client.get(res.uri, timeout=self.REFRESH_TIMEOUT)
                    res.update(response.payload)
                    refresh_cnt += 1

        if refresh_cnt:
            self.coap_client.close()

    """
        Node discover resources thread method
    """
    def _discover(self):
        from defines import known_resources, refreshable_resources

        self.coap_client = HelperClient(server=(self.ip, self.port))
        response = self.coap_client.discover(timeout=self.DISCOVER_TIMEOUT)
        for resp in response.payload.split(','):
            resp = resp.split(';')
            # skip entries without name
            if len(resp) < 2:
                continue
            discovered_res = NodeResource(resp[0][1:-1], resp[1])  # strip <>
            # get node info resource
            if discovered_res.name == known_resources["node_info"]:
                self._debug("getting node info")
                response = self.coap_client.get(discovered_res.uri, timeout=self.REFRESH_TIMEOUT)
                self.info = response.payload
                self._debug("received " + self.info)
            # add resource if no ignored
            if discovered_res.name in refreshable_resources:
                self._debug("added resource " + discovered_res.name)
                self.resources.append(discovered_res)

        self.discover_successful = True
        self.coap_client.close()


    """
        Update refresh timer
    """
    def _refreshed(self):
        self.__last_refresh = time()
        self.__last_seen = time()

    def _debug(self, msg):
        print "Node " + self.ip + "/" + str(self.port) + ": " + msg