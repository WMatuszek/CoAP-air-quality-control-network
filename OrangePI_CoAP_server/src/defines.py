__author__ = 'Witold'

from coapthon.defines import COAP_DEFAULT_PORT

REV_PROXY_NODES_XML_PATH = "rev_proxy_nodes.xml"

EXTERNAL_INTERFACE = 'eth0'
INTERNAL_INTERFACE = 'wlan0'
DEFAULT_PORT = COAP_DEFAULT_PORT

NODE_LIFETIME_SECONDS = 30
NODE_DATA_REFRESH_INTERVAL_SECONDS = 30

known_resources = {
    "response": "response",
    "temperature": "temperature",
    "pressure": "pressure",
    "node_info": "info",
    "air_quality": "pm",
    "battery_state": "battery"
}

info_resources = [known_resources["node_info"]]

refreshable_resources = [known_resources["temperature"],
                         known_resources["pressure"],
                         known_resources["air_quality"],
                         known_resources["battery_state"]]

observed_resources = [known_resources["air_quality"]]

ignored_resources = [known_resources["response"]]

