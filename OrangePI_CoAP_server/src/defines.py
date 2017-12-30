__author__ = 'Witold'

from coapthon.defines import COAP_DEFAULT_PORT

REV_PROXY_NODES_XML_PATH = "rev_proxy_nodes.xml"

DEFAULT_PORT = COAP_DEFAULT_PORT
EXTERNAL_INTERFACE = 'eth0'
INTERNAL_INTERFACE = 'wlan0'

NODE_LIFETIME_SECONDS = 30
NODE_DATA_REFRESH_INTERVAL_SECONDS = 30

'''
    Known resources, identified by 'if' field in core link format
'''
known_resources = {
    "response": "response",
    "temperature": "temperature",
    "pressure": "pressure",
    "node_info": "info",
    "air_quality": "pm",
    "battery_state": "battery"
}
# TODO air_quality as observed
refreshable_resources = [known_resources["pressure"],
                         known_resources["air_quality"],
                         known_resources["battery_state"]]
info_resources = [known_resources["node_info"]]
observed_resources = [known_resources["temperature"]]
ignored_resources = [known_resources["response"]]

