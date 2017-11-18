#!/bin/bash

now=$(date -R)

tar="/etc/hostapd/hostapd.conf"
cp_tar="/etc/hostapd/backup/hostapd.conf"
tar_init="/etc/init.d/hostapd"
cp_tar_init="/etc/hostapd/backup/hostapd"

mkdir -p /etc/hostapd/backup

/etc/init.d/hostapd stop

if [ -e $tar ]
then
	cp $tar "$cp_tar $now"
	rm $tar
fi
	cp hostapd.conf $tar

if [ -e $tar_init ]
then
	cp $tar_init "$cp_tar_init $now"
	rm $tar_init
fi
	cp hostapd_daemon $tar_init

systemctl daemon-reload
/etc/init.d/hostapd start
