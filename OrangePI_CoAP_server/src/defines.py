__author__ = 'Witold'

NODE_LIFETIME_SECONDS = 20
NODE_DATA_REFRESH_INTERVAL_SECONDS = 10

known_resources = {
    "temperature": "temperature",
    "pressure": "pressure",
    "node_info": "info",
    "air_quality": "pm"
}

refreshable_resources = [known_resources["temperature"],
                         known_resources["pressure"],
                         known_resources["air_quality"]]