#!/bin/bash

now=$(date -R)

tar_hostapd_conf="/etc/hostapd/hostapd.conf"
tar_hostapd="/etc/init.d/hostapd"
tar_udhcpd_conf="/etc/udhcpd.conf"
tar_udhcpd="/etc/default/udhcpd"

path_backup="backup/"

mkdir -p /etc/hostapd/backup

if [ -e $tar_hostapd_conf ]
then
	cp $tar_hostapd_conf "$path_backup hostapd $now"
	rm $tar_hostapd_conf
fi
	cp hostapd.conf $tar_hostapd_conf

if [ -e $tar_hostapd ]
then
	cp $tar_hostapd "$path_backup hostapd_daemon $now"
	rm $tar_hostapd
fi
	cp hostapd_daemon $tar_hostapd

if [ -e $tar_udhcpd_conf ]
then
	cp $tar_udhcpd_conf "$path_backup udhcpd_conf $now"
	rm $tar_udhcpd_conf
fi
	cp udhcpd.conf $tar_udhcpd_conf

if [ -e $tar_udhcpd ]
then
	cp $tar_udhcpd "$path_backup udhcpd $now"
	rm $tar_udhcpd
fi
	cp udhcpd $tar_udhcpd


systemctl daemon-reload
/etc/init.d/hostapd start
