#!/bin/bash

THIS_SCRIPT=$(basename "$0")

log()
{
    echo "$(date '+%Y %b %d %H:%M:%S.%6N') [$THIS_SCRIPT#$$]: ${FUNCNAME[1]}: $*" >> /opt/logs/netsrvmgr.log
}

. /etc/device.properties

[ -f /lib/systemd/system/virtual-moca-iface.service ] && [ -f /lib/systemd/system/virtual-wifi-iface.service ] && RUN_LINK_LOCAL_SERVICES=true

if [ "$RUN_LINK_LOCAL_SERVICES" == "true" ]; then
    declare "DHCPv4_SERVICE_$ETHERNET_INTERFACE=virtual-moca-iface.service"
    declare "DHCPv4_SERVICE_$WIFI_INTERFACE=virtual-wifi-iface.service"
else
    declare "DHCPv4_SERVICE_$ETHERNET_INTERFACE=udhcpc@${ETHERNET_INTERFACE}.service"
    declare "DHCPv4_SERVICE_$WIFI_INTERFACE=udhcpc@${WIFI_INTERFACE}.service"
fi

ipv4_deconfigure_interface()
{
    DHCPv4_SERVICE=DHCPv4_SERVICE_$1
    log "$1: systemctl stop ${!DHCPv4_SERVICE}"
    systemctl stop "${!DHCPv4_SERVICE}"
}

ipv4_configure_dhcpip()
{
    DHCPv4_SERVICE=DHCPv4_SERVICE_$1
    log "$1: systemctl start ${!DHCPv4_SERVICE}"
    systemctl start "${!DHCPv4_SERVICE}"
}

configure_port()
{
    if [ "$1" == "$WIFI_INTERFACE" ]; then
        port=$(grep "WIFI_INTERFACE=" /etc/device.properties  | cut -d "=" -f 2)
    else
        port=$(grep "ETHERNET_INTERFACE=" /etc/device.properties  | cut -d "=" -f 2)
    fi

    if [ -n "$port" ]; then
        port="$port:0"
    else
        return 1
    fi

    ifconfig $port $2 netmask $3
    route add default gw $4
    return 0
}

configure_dns()
{
    if [ -n "$1" ]; then
        echo "nameserver $1" >> /tmp/resolv.dnsmasq.udhcpc
        log "writing nameserver $1  to resolv.dnsmasq.udhcpc"
    fi

    if [ -n "$2" ]; then
        echo "nameserver $2" >> /tmp/resolv.dnsmasq.udhcpc
        log "writing nameserver $2  to resolv.dnsmasq.udhcpc"
    fi
}

configure_settings_from_file()
{
    if [ ! -f $file ]
    then
        return 1
    fi

    declare -a keys=()
    declare -a vals=()
    while IFS='=' read -r key val; do
        [[ $key = '#'* ]] && continue
        keys+=("$key"); vals+=("$val")
    done < $file

    for ((i = 0; i < ${#keys[@]}; i++)); do
        case "${keys[i]}" in
            "ipaddress")    ip=${vals[i]} ;;
            "netmask")      mask=${vals[i]} ;;
            "gateway")      gw=${vals[i]} ;;
            "primarydns")   dns1=${vals[i]} ;;
            "secondarydns") dns2=${vals[i]} ;;
        esac
    done

    configure_port $1 $ip $mask $gw
    if [ $? -eq 1 ]; then
        return 1
    fi
    configure_dns $dns1 $dns2
    return 0
}

ipv4_configure_manualip()
{
    if [ "$1" == "$WIFI_INTERFACE" ]; then
        file="/opt/persistent/ip.wifi.0"
    else
        file="/opt/persistent/ip.eth0.0"
    fi
    configure_settings_from_file $1 $file
    if [ $? -eq 1 ]; then
        return 1
    else
        return 0
    fi
}

ipv4_configure_interface()
{
    MIP_ENABLED="$(tr181 Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Network.ManualIPSettings.Enable 2>&1 > /dev/null)"
    if [ "$MIP_ENABLED" == "true" ]; then
        ipv4_configure_manualip $1
        if [ $? -eq 1 ]; then
            ipv4_configure_dhcpip $1
        fi
    else
        ipv4_configure_dhcpip $1
    fi
}

ipv6_deconfigure_interface()
{
    log "$1: sysctl -w net.ipv6.conf.$1.disable_ipv6=1"
    sysctl -w net.ipv6.conf."$1".disable_ipv6=1
}

ipv6_configure_interface()
{
    log "$1: sysctl -w net.ipv6.conf.$1.disable_ipv6=0"
    sysctl -w net.ipv6.conf."$1".disable_ipv6=0
}

flush_routes_and_addresses()
{
    log "$1"
    ip route flush dev "$1" # remove all routes configured on passed interface
    ip route flush cache
    ip addr flush dev "$1"
}

deconfigure_interface()
{
    ipv4_deconfigure_interface "$1"
    ipv6_deconfigure_interface "$1"
    flush_routes_and_addresses "$1"
}

configure_interface()
{
    ipv6_configure_interface "$1"
    ipv4_configure_interface "$1"
    [ "$RUN_LINK_LOCAL_SERVICES" != "true" ] && return
    # restart link local services in background after killing any previously started instance
    [ -n "$!" ] && kill $!; restart_link_local_services "$1" &
}

restart_link_local_services()
{
    log "$1: /lib/rdk/zcip.sh"
    /lib/rdk/zcip.sh
ip route add 224.0.0.0/4 dev "$MOCA_INTERFACE"
    log "$1: systemctl restart xupnp.service"
    systemctl restart xupnp.service
    log "$1: systemctl restart xcal-device.service"
    systemctl restart xcal-device.service
}

# usage: test_interface <interface> <timeout seconds> <tries>
test_interface()
{
    endpoint_host="xre.ccp.xcal.tv"
    endpoint_port="10601"
    endpoint="${endpoint_host}:${endpoint_port}"
    log "$1: BEGIN (endpoint=$endpoint, timeout=${2}s, max tries=$3)"
    i=0
    while true; do
        i=$((i+1))
        if timeout "$2" bash -c "echo > /dev/tcp/${endpoint_host}/${endpoint_port}" &> /dev/null; then
            log "$1: PASS (endpoint=$endpoint, timeout=${2}s, tries=$i)"
            return 0
        elif [ "$i" -ge "$3" ]; then
            log "$1: FAIL (endpoint=$endpoint, timeout=${2}s, tries=$i)"
            return 1
        fi
        sleep 1
    done
}

# usage: configure_interfaces_and_test <interface to configure> <interface to deconfigure>
configure_interfaces_and_test()
{
    log "$1: BEGIN (deconfigure $2, configure $1, test $1)"
    begin="$(cut -d ' ' -f1 /proc/uptime)"
    deconfigure_interface "$2"
    configure_interface "$1"
    test_interface "$1" 2 15
    test_result=$?
    end="$(cut -d ' ' -f1 /proc/uptime)"
    log "$1: END (deconfigure $2, configure $1, test $1), took $(dc "$end" "$begin" - p)s"
    return $test_result
}

set_interface_state()
{
    log "ip link set dev $1 $2"
    ip link set dev "$1" "$2"
}

wpa_supplicant_enable()
{
    log "$1 (IARM_event_sender WiFiInterfaceStateEvent $1)"
    IARM_event_sender WiFiInterfaceStateEvent "$1"
}

try_ethernet()
{
    rm /tmp/ani_wifi
    rm /tmp/wifi-on # for checkWiFiModule (called by /lib/rdk/zcip.sh (moca.service)) to return 0. otherwise, it returns 1 => zc gets tried on $WIFI_INTERFACE (instead of $ETHERNET_INTERFACE)
    set_interface_state "$ETHERNET_INTERFACE" up
    configure_interfaces_and_test "$ETHERNET_INTERFACE" "$WIFI_INTERFACE" || return 1
    set_interface_state "$WIFI_INTERFACE" down
    wpa_supplicant_enable 0
    return 0
}

try_wifi()
{
    touch /tmp/ani_wifi
    touch /tmp/wifi-on # for checkWiFiModule (called by /lib/rdk/zcip.sh (moca.service)) to return 1. otherwise, if eth is in, it returns 0 => zc gets tried on $ETHERNET_INTERFACE (instead of $WIFI_INTERFACE)
    set_interface_state "$WIFI_INTERFACE" up
    wpa_supplicant_enable 1
    configure_interfaces_and_test "$WIFI_INTERFACE" "$ETHERNET_INTERFACE" || return 1
    return 0
}

# usage: try_interface <interface> <reason>
try_interface()
{
    log "$1 ($2)"
    case "$1" in
        "$ETHERNET_INTERFACE") try_ethernet; result=$? ;;
        "$WIFI_INTERFACE")     try_wifi;     result=$? ;;
    esac
    log "$1 end"
    return $result
}

# sleep 5 # delay to filter out ethernet glitches / fast link state transitions

if [ -f /tmp/wifi_disallowed ]; then
    set_interface_state "$WIFI_INTERFACE" down
    wpa_supplicant_enable 0
    try_interface "$ETHERNET_INTERFACE" "$WIFI_INTERFACE is disallowed" # pni setting is irrelevant
elif [ "$(cat /sys/class/net/"$ETHERNET_INTERFACE"/carrier)" != "1" ]; then # no carrier on ethernet
    try_interface "$WIFI_INTERFACE" "no carrier on $ETHERNET_INTERFACE" # pni setting is irrelevant
else
    RFC_ENABLED="$(tr181 Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.PreferredNetworkInterface.Enable 2>&1 > /dev/null)"
    if [ "$RFC_ENABLED" == "true" ] && [ -f /tmp/pni_wifi ]; then
        pni="$WIFI_INTERFACE"
        nonpni="$ETHERNET_INTERFACE"
    else
        pni="$ETHERNET_INTERFACE"
        nonpni="$WIFI_INTERFACE"
    fi
    log "pni=$pni nonpni=$nonpni (RFC enabled = $RFC_ENABLED)"
    while true; do
        try_interface "$pni" "pni" && break
        try_interface "$nonpni" "nonpni" && break
        log "$pni (pni) and $nonpni (nonpni) failed, retrying after 10s"
        sleep 10
    done
fi

wait # for any restarted link local services to finish start up before exiting

exit 0
