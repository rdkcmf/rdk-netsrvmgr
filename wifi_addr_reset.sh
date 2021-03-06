#!/bin/bash

THIS_SCRIPT=$(basename "$0")

log()
{
    echo "$(date '+%Y %b %d %H:%M:%S.%6N') [$THIS_SCRIPT#$$]: $*" >> /opt/logs/netsrvmgr.log
}

. /etc/device.properties

LOCAL_SERVICE=network@${WIFI_INTERFACE}.service

if [ -f /lib/systemd/system/virtual-moca-iface.service ]; then
LOCAL_SERVICE=virtual-wifi-iface.service
fi

log "Flushing Address and routes"
ip addr flush dev $WIFI_INTERFACE
ip route flush dev  $WIFI_INTERFACE
ip route flush cache


log "Restarting Local Services"

touch /opt/persistent/ip.wifi.0
systemctl restart ${LOCAL_SERVICE}
/lib/rdk/zcip.sh
ip route add 224.0.0.0/4 dev $MOCA_INTERFACE

systemctl restart xupnp.service
systemctl restart xcal-device.service

wait # for any restarted link local services to finish start up before exiting
systemctl restart dibbler.service
