__author__ = 'Witold'

from coapthon.defines import COAP_DEFAULT_PORT

OPTION_USE_PROXY = "--use-proxy"

REV_PROXY_NODES_XML_PATH = "rev_proxy_nodes.xml"

DEFAULT_PORT = COAP_DEFAULT_PORT
EXTERNAL_INTERFACE = 'eth0'
INTERNAL_INTERFACE = 'wlan0'

NODE_LIFETIME_SECONDS = 30
NODE_DATA_REFRESH_INTERVAL_SECONDS = 30

'''
    Known resources, identified by 'rt' field in core link format
'''
resource_name_attribute = 'rt'
known_resources = {
    "response": "response",
    "temperature": "temperature",
    "pressure": "pressure",
    "node_info": "sname",
    "air_quality": "pm",
    "battery_state": "battery",
    "sleep_cycle": "sleepcycle"
}
observed_resources = [known_resources["air_quality"]]
refreshable_resources = [known_resources["pressure"],
                         known_resources["temperature"]]
on_demand_resources = [known_resources["node_info"],
                       known_resources["pressure"],
                       known_resources["temperature"],
                       known_resources["battery_state"],
                       known_resources["sleep_cycle"]]
ignored_resources = [known_resources["response"]]

