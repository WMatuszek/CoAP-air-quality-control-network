__author__ = 'Witold'

from flask import Flask, request, render_template
from main import CoAPClientApp
from defines import INTERNAL_INTERFACE, DEFAULT_PORT
from get_interface_ip import get_interface_ip

flaskApp = Flask(__name__)
coap_client_app_obj = None


def install_coap_client_app_obj(client_obj):
    global coap_client_app_obj
    coap_client_app_obj = client_obj


def parse_node_data(nodes_tmp):
    from defines import ignored_resources

    nodes_info = []
    for node in nodes_tmp:
        node_info_dict = dict(info="", data=[])
        node_info_dict['info'] = (str(node.node_name) if node.node_name is not None else "UNKNOWN") + \
                                 " at " + node.ip + ":" + str(node.port)
        for res in node.resources:
            if res.name not in ignored_resources:
                node_info_dict['data'].append(res)
        nodes_info.append(node_info_dict)

    return nodes_info


@flaskApp.route('/')
def index():
    node_data = []
    if coap_client_app_obj is not None:
        node_data = parse_node_data(coap_client_app_obj.get_nodes())
    return render_template('node_report.html',
                           node_data=node_data,
                           server_port=DEFAULT_PORT,
                           server_ip=get_interface_ip(INTERNAL_INTERFACE))


@flaskApp.route('/refresh/', methods=['POST', 'GET'])
def refresh():
    node_data = []
    if coap_client_app_obj is not None:
        node_data = parse_node_data(coap_client_app_obj.get_nodes(refresh=True))
    return render_template('node_report.html',
                           node_data=node_data,
                           server_port=DEFAULT_PORT,
                           server_ip=get_interface_ip(INTERNAL_INTERFACE))

