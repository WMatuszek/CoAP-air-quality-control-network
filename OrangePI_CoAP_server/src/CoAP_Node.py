__author__ = 'Witold'

from time import time
from defines import NODE_LIFETIME_SECONDS

class Node(object):

    def __init__(self, ip, port, active=True):
        self.ip = ip
        self.port = port
        self.active = True

        self.uris = [] # TODO discovered uris here

        self.__last_seen = time()

    def is_alive(self):
        if self.active:
            if time() - self.__last_seen > NODE_LIFETIME_SECONDS:
                self.active = False
        return self.active

    def set_alive(self):
        self.active = True