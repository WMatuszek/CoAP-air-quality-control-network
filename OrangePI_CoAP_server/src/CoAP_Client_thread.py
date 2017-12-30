__author__ = 'Witold'

from time import sleep
from threading import Thread, Event
from coapthon.client.helperclient import HelperClient


class ClientThread(Thread):

    def __init__(self, CoAP_client, sleep_s=3):
        Thread.__init__(self)
        self.client = CoAP_client
        self.sleep_sec = sleep_s

        self.__stop = Event()
        self.__stop.clear()

    def run(self):
        while not self.__stop.isSet():
            sleep(self.sleep_sec)
            self.client.loop()
        self.client.stop()

    def stop(self):
        self.__stop.set()