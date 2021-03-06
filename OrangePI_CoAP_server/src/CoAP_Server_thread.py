__author__ = 'Witold'

from threading import Thread

class ServerThread(Thread):

    def __init__(self, CoAP_server):
        Thread.__init__(self)
        self.server = CoAP_server

    def run(self):
        if self.server is not None:
            self.server.listen()

    def stop(self):
        if self.server is not None:
            self.server.close()
