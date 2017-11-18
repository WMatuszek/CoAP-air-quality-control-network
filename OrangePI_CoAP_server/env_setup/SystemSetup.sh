#!/bin/bash

ap_setup_script="APConfig/SetupAP.sh"

sudo apt-get -y install python3
sudo apt-get install python-pip
pip install --upgrade pip
pip install setuptools
pip install CoAPthon

sudo apt-get -y install hostapd

/bin/bash $ap_setup_script


