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

    def __init__(self, uri, name, attributes, server, refreshable=False, observable=False, on_demand=False):
        if uri[0] == '<' and uri[-1] == '>':
            uri = uri[1:-1]

        self.uri = uri
        self.name = name
        self.value = None

        self._refreshable = refreshable
        self._observable = observable
        self._on_demand = on_demand
        self._attributes = attributes
        self._server = server

        self._coap_client = None
        self._last_observe_response = None
        self._last_update = 0

        self._start_observe()

    def __del__(self):
        if self._coap_client is not None:
            self._coap_client.cancel_observe(self._last_observe_response, True)

    def update_value(self, val):
        self.value = val
        self._last_update = time()
        self._to_log("updated with " + self.value)

    def is_refreshable(self):
        return self._refreshable

    def is_observed(self):
        return self._observable

    def is_on_demand(self):
        return self._on_demand

    def is_fresh(self, max_age):
        return (time() - self._last_update) < max_age

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
    REFRESH_TIMEOUT = 6
    NODE_INFO_TIMEOUT = 6

    def __init__(self, ip, port, is_active=True):
        self.ip = ip
        self.port = port
        self.info = "Node:" + self.ip
        self.info_uri = ""
        self.active = is_active
        self.discover_successful = False

        self.resources = []

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
        Try refresh, auto by default, manual if specified
        Does not run if thread already running - would cause multiple requests to unique server

        Auto refresh calls GET for refreshable resources
        Manual refresh calls GET for resources on demand

        By default runs refresh in separate thread
    """
    def try_refresh(self, manual=False, blocking=False):
        # Check if thread still running
        if self._refresh_thread is not None:
            if self._refresh_thread.is_alive():
                return False

        if manual:
            resources = [r for r in self.resources if r.is_on_demand()]
        else:
            resources = [r for r in self.resources
                         if r.is_refreshable() and not r.is_fresh(NODE_DATA_REFRESH_INTERVAL_SECONDS)]

        if not blocking:
            self._refresh_thread = threading.Thread(target=self._refresh, args=[resources])
            self._refresh_thread.start()
        else:
            self._refresh(resources)
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
                self.info_uri = uri
                continue

            # Add resource if refreshable
            if name not in defines.ignored_resources:
                refresh = name in defines.refreshable_resources
                observe = name in defines.observed_resources
                on_demand = name in defines.on_demand_resources
                res = NodeResource(uri, name, attributes, (self.ip, self.port),
                                   refreshable=refresh,
                                   observable=observe,
                                   on_demand=on_demand)
                self.resources.append(res)
                self._to_log("added resource " + name + " o=" + str(observe) + " r=" + str(refresh))

        coap_client.close()

        self._to_log("getting node_name")
        res = NodeResource(self.info_uri, "sname", [], (self.ip, self.port))
        self._to_log("received node_name " + str(res.value))
        self._refresh([res])
        self.info = res.value

        self.try_refresh(manual=True, blocking=False)
        self.discover_successful = True


    """
        Node refresh thread method
    """
    def _refresh(self, resources):
        if len(resources) == 0:
            return
        coap_client = HelperClient(server=(self.ip, self.port))
        for res in resources:
            self._to_log("try GET res=" + res.uri)
            response = coap_client.get(res.uri, timeout=self.REFRESH_TIMEOUT)
            res.update_value(response.payload)
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
        from defines import resource_name_attribute
        for attr in attributes:
            if attr.startswith(resource_name_attribute):
                return attr[attr.find('\"') + 1:attr.rfind('\"')]
        return "Unknown"