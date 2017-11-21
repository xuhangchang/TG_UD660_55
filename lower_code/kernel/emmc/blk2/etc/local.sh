#!/bin/sh
cd /etc

myfile="config"
if [ ! -f "$myfile" ]; then
 echo "creator default file"
 echo "DEVICE h3" >> $myfile
 echo "IP 192.168.1.222" >> $myfile
 echo "MAC 36:BC:01:02:00:00" >> $myfile
 cat /proc/cpuinfo | grep "Serial" | awk '{print "ID "$3}' >> $myfile
fi
 
#DEV=$(sed -n "1p" $myfile | awk '{printf $2}')
#ID=$(sed -n "2p" $myfile | awk '{printf $2}')
#MAC=$(sed -n "3p" $myfile | awk '{printf $2}')
#IP=$(sed -n "4p" $myfile | awk '{printf $2}')
DEV=$(cat $myfile | grep "DEVICE" | awk '{printf $2}')
ID=$(cat $myfile | grep "ID" | awk '{printf $2}')
MAC=$(cat $myfile | grep "MAC" | awk '{printf $2}')
IP=$(cat $myfile | grep "IP" | awk '{printf $2}')
 
 
echo $DEV
echo $ID
echo $MAC
echo $IP

                                                     
insmod /etc/g_ether.ko hostmac=$MAC                                                     
ifconfig usb0 $IP up
