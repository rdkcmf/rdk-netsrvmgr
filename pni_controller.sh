#!/bin/bash

THIS_SCRIPT=$(basename "$0")

. /etc/device.properties

if [ -f /etc/env_setup.sh ]; then
    . /etc/env_setup.sh
fi

log()
{
    echo "$(date '+%Y %b %d %H:%M:%S.%6N') [$THIS_SCRIPT#$$]: ${FUNCNAME[1]}: $*" >> /opt/logs/netsrvmgr.log
}

[ -f /lib/systemd/system/virtual-moca-iface.service ] && [ -f /lib/systemd/system/virtual-wifi-iface.service ] && RUN_LINK_LOCAL_SERVICES=true

if [ "$RUN_LINK_LOCAL_SERVICES" == "true" ]; then
    declare "DHCPv4_SERVICE_$ETHERNET_INTERFACE=virtual-moca-iface.service"
    declare "DHCPv4_SERVICE_$WIFI_INTERFACE=virtual-wifi-iface.service"
else
    declare "DHCPv4_SERVICE_$ETHERNET_INTERFACE=udhcpc@${ETHERNET_INTERFACE}.service"
    declare "DHCPv4_SERVICE_$WIFI_INTERFACE=udhcpc@${WIFI_INTERFACE}.service"
fi

ipv4_deconfigure_dhcpip()
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

ipv4_configure_manualip()
{
    # identify ip settings file
    if [ "$1" == "$WIFI_INTERFACE" ]; then
        ip_settings_file="/opt/persistent/ip.wifi.0"
    else
        ip_settings_file="/opt/persistent/ip.eth0.0"
    fi
    [ ! -s $ip_settings_file ] && return 1

    # read ip settings file
    while IFS='=' read -r key val; do
        [[ $key = '#'* ]] && continue
        case "$key" in
            "ipaddress")    ip="$val" ;;
            "netmask")      mask="$val" ;;
            "gateway")      gw="$val" ;;
            "primarydns")   dns1="$val" ;;
            "secondarydns") dns2="$val" ;;
        esac
    done < $ip_settings_file

    ipv4_deconfigure_dhcpip "$1"

    # apply ip settings
    [ "$RUN_LINK_LOCAL_SERVICES" == "true" ] && interface="$1:0" || interface="$1"
    log "$1: ifconfig $interface $ip netmask $mask up"
    ifconfig "$interface" "$ip" netmask "$mask" up
    log "$1: route add default gw $gw dev $interface"
    route add default gw "$gw" dev "$interface"
    log "$1: writing nameservers $dns1, $dns2 to /tmp/resolv.dnsmasq.udhcpc"
    cat /dev/null > /tmp/ipsettings.nameservers
    [ -n "$dns1" ] && echo "nameserver $dns1" >> /tmp/ipsettings.nameservers
    [ -n "$dns2" ] && echo "nameserver $dns2" >> /tmp/ipsettings.nameservers
    cp /tmp/ipsettings.nameservers /tmp/resolv.dnsmasq.udhcpc
    return 0
}

ipv4_deconfigure_interface()
{
    ipv4_deconfigure_dhcpip "$1"
    flush_routes_and_addresses "$1" "-4"
    # cat /dev/null > /tmp/resolv.dnsmasq.udhcpc
}

ipv4_configure_interface()
{
    MIP_ENABLED="$(tr181 Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.Network.ManualIPSettings.Enable 2>&1 > /dev/null)"
    [ "$MIP_ENABLED" == "true" ] && ipv4_configure_manualip "$1" && return 0
    ipv4_configure_dhcpip "$1"
}

ipv4_reconfigure_interface()
{
    log "$1: ipv4_deconfigure_interface"
    ipv4_deconfigure_interface "$1"
    log "$1: systemctl restart pni_controller.service"
    systemctl restart pni_controller.service
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
    ip_family="$2"
    log "$1 (ip_family $ip_family)"
    ip "$ip_family" route flush dev "$1" # remove all routes of specified "$ip_family" configured on passed interface
    ip "$ip_family" route flush cache    # for ipv4: no point, as per https://vincent.bernat.ch/en/blog/2017-ipv6-route-lookup-linux ("While IPv4 lost its route cache in Linux 3.6 (commit 5e9965c15ba8) ...")
    ip "$ip_family" addr flush dev "$1"  # for ipv4: addr flush also removes aliases/virtual interfaces
}

deconfigure_interface()
{
    ipv4_deconfigure_interface "$1"
    ipv6_deconfigure_interface "$1"
}

configure_interface()
{
    ipv6_configure_interface "$1"
    ipv4_configure_interface "$1"
    if [ "$RUN_LINK_LOCAL_SERVICES" == "true" ]; then
        [ -n "$pid_ll_services" ] && kill "$pid_ll_services"    # kill any previously started instance
        restart_link_local_services "$1" & pid_ll_services="$!" # restart link local services in background
    fi
    [ "$PNI_ENABLED" != "true" ] || [ "$CONFIG_DISABLE_CONNECTIVITY_TEST" == "true" ] || test_connectivity "$1" 2000 15
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

# usage: test_connectivity <interface> <timeout_ms> <max_tries>
test_connectivity()
{
    log "$1: BEGIN (timeout=${2}ms, max tries=$3)"
    if /usr/bin/test_connectivity "$2" "$3"; then
        log "$1: PASS"
        return 0
    else
        log "$1: FAIL"
        return 1
    fi
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

set_wifi_state()
{
    case "$1" in
        "up")   set_interface_state "$WIFI_INTERFACE" up;   wpa_supplicant_enable 1 ;;
        "down") set_interface_state "$WIFI_INTERFACE" down; wpa_supplicant_enable 0 ;;
    esac
}

try_ethernet()
{
    rm /tmp/ani_wifi
    rm /tmp/wifi-on # for checkWiFiModule (called by /lib/rdk/zcip.sh (moca.service)) to return 0. otherwise, it returns 1 => zc gets tried on $WIFI_INTERFACE (instead of $ETHERNET_INTERFACE)
    set_interface_state "$ETHERNET_INTERFACE" up
    deconfigure_interface "$WIFI_INTERFACE"
    configure_interface "$ETHERNET_INTERFACE" || return 1
    [ "$PNI_ENABLED" != "true" ] || [ "$CONFIG_ALLOW_PNI_TO_DISABLE_WIFI" == "false" ] || set_wifi_state down || return 0
}

try_wifi()
{
    touch /tmp/ani_wifi
    touch /tmp/wifi-on # for checkWiFiModule (called by /lib/rdk/zcip.sh (moca.service)) to return 1. otherwise, if eth is in, it returns 0 => zc gets tried on $ETHERNET_INTERFACE (instead of $WIFI_INTERFACE)
    set_wifi_state up
    deconfigure_interface "$ETHERNET_INTERFACE"
    configure_interface "$WIFI_INTERFACE" || return 1
}

# usage: try_interface <interface> <reason>
try_interface()
{
    log "$1 ($2)"
    begin="$(cut -d ' ' -f1 /proc/uptime)"
    case "$1" in
        "$ETHERNET_INTERFACE") try_ethernet; result=$? ;;
        "$WIFI_INTERFACE")     try_wifi;     result=$? ;;
    esac
    end="$(cut -d ' ' -f1 /proc/uptime)"
    log "$1 end, took $(dc "$end" "$begin" - p)s"
    return $result
}

if [ "$1" == "ipv4_reconfigure_interface" ]; then
    ipv4_reconfigure_interface "$2"
    exit $?
fi

if [ "$1" == "uses_virtual_interface" ]; then
    [ "$RUN_LINK_LOCAL_SERVICES" == "true" ]
    exit $?
fi

# sleep 5 # delay to filter out ethernet glitches / fast link state transitions

if [ -f /tmp/ethernet_disallowed ] && [ -f /tmp/wifi_disallowed ]; then
    log "$ETHERNET_INTERFACE and $WIFI_INTERFACE are disallowed!"
    set_interface_state "$ETHERNET_INTERFACE" down
    set_wifi_state down
    exit 0
elif [ -f /tmp/wifi_disallowed ]; then
    set_wifi_state down
    try_interface "$ETHERNET_INTERFACE" "$WIFI_INTERFACE is disallowed" # pni setting is irrelevant
elif [ -f /tmp/ethernet_disallowed ]; then
    set_interface_state "$ETHERNET_INTERFACE" down
    try_interface "$WIFI_INTERFACE" "$ETHERNET_INTERFACE is disallowed" # pni setting is irrelevant
elif [ "$(cat /sys/class/net/"$ETHERNET_INTERFACE"/carrier)" != "1" ]; then
    try_interface "$WIFI_INTERFACE" "no carrier on $ETHERNET_INTERFACE" # pni setting is irrelevant
elif [ "$CONFIG_DISABLE_PNI" == "true" ] || [ "$(tr181 Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.PreferredNetworkInterface.Enable 2>&1 > /dev/null)" != "true" ]; then
    [ -f /opt/persistent/pni_wifi ] && rm /opt/persistent/pni_wifi
    [ -f /tmp/pni_wifi ] && rm /tmp/pni_wifi
    set_wifi_state up
    try_interface "$ETHERNET_INTERFACE" "pni feature is not enabled"    # pni setting is irrelevant
else # run pni loop
    PNI_ENABLED=true
    if [ -f /tmp/pni_wifi ]; then
        pni="$WIFI_INTERFACE"
        nonpni="$ETHERNET_INTERFACE"
    else
        pni="$ETHERNET_INTERFACE"
        nonpni="$WIFI_INTERFACE"
        [ "$CONFIG_ALLOW_PNI_TO_DISABLE_WIFI" == "false" ] && set_wifi_state up
    fi
    while true; do
        try_interface "$pni" "pni" && break
        try_interface "$nonpni" "nonpni" && break
        log "$pni (pni) and $nonpni (nonpni) failed, retrying after 10s"
        sleep 10
    done
fi

wait # for any restarted link local services to finish start up before exiting
systemctl restart dibbler.service

exit 0
