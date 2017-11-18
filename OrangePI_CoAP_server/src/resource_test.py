__author__ = 'Witold'

from coapthon.resources.resource import Resource


class TestResource(Resource):
    def __init__(self, name="TestResource", coap_server=None):
        super(TestResource, self).__init__(name, coap_server, visible=True,
                                            observable=True, allow_children=True)
        self.payload = "HELLO"
        self.connected_nodes = []

    def render_GET(self, request):
        print request.source
        self.connected_nodes.append(request.source)
        return self

    def render_PUT(self, request):
        return self

    def render_POST(self, request):
        res = TestResource()
        res.location_query = request.uri_query
        res.payload = request.payload
        return res

    def render_DELETE(self, request):
        return True
