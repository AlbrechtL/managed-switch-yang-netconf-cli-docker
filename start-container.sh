#!/bin/sh

set -Eeuo pipefail
trap - ERR

. /usr/local/bin/helpers.sh

# Attach physical interfaces to Docker container
attach_eth_if () {
  HOST_IF=$1
  CONTAINER_IF=$2
  # Privileged=true and pid=host mode necessary
  # Sources
  # * https://serverfault.com/questions/688483/assign-physical-interface-to-docker-exclusively
  # * https://medium.com/lucjuggery/a-container-to-access-the-shell-of-the-host-2c7c227c64e9
  # * https://developers.redhat.com/blog/2018/10/22/introduction-to-linux-interfaces-for-virtual-networking#

  echo "Attaching physical Ethernet interface $HOST_IF into container with the name $CONTAINER_IF ..."

  # Check if interface exists
  if ! nsenter --target 1 --uts --net --ipc --mount ip link show "$HOST_IF" &> /dev/null; then
    echo "Host Ethernet interface $HOST_IF does not exists. It can be a wrong interface name or the Ethernet interface is already assigend to this container."
    return
  fi

  PID_CONTAINTER=$(nsenter --target 1 --uts --net --ipc --mount docker inspect -f '{{.State.Pid}}' $(cat /etc/hostname))
  nsenter --target 1 --uts --net --ipc --mount mkdir -p /var/run/netns
  nsenter --target 1 --uts --net --ipc --mount ln -s /proc/$PID_CONTAINTER/ns/net /var/run/netns/$PID_CONTAINTER
  nsenter --target 1 --uts --net --ipc --mount ip link set $HOST_IF netns $PID_CONTAINTER name $CONTAINER_IF
  nsenter --target 1 --uts --net --ipc --mount ip netns exec $PID_CONTAINTER ip link set $CONTAINER_IF up
  nsenter --target 1 --uts --net --ipc --mount rm /var/run/netns/$PID_CONTAINTER

  ip link set $CONTAINER_IF up
}

# Check if SSH host keys exist in /storage/etc/ssh (volume), generate if missing
if ! ls /storage/etc/ssh/ssh_host_*_key >/dev/null 2>&1; then
    echo "No SSH host keys found in /storage. Generating new SSH host keys..."
    mkdir -p /storage/etc/ssh
    ssh-keygen -A -f /storage
fi

# Symlink keys from /storage/etc/ssh to /etc/ssh
for key in /storage/etc/ssh/ssh_host_*_key*; do
    basekey=$(basename "$key")
    if [ ! -e "/etc/ssh/$basekey" ]; then
        ln -sf "/storage/etc/ssh/$basekey" "/etc/ssh/$basekey"
    fi
done

# Remove default nginx config to avoid conflicts
rm -f /etc/nginx/http.d/default.conf

# Attach interfaces specified in SW_IF environment variable
if [ -n "${SW_IF:-}" ]; then
    OLD_IFS="$IFS"
    IFS=',' 

    # Try to create bridge with vlan_filtering enabled
    if ip link add name br0 type bridge vlan_filtering 1 2>/dev/null; then
      echo "Bridge br0 created with vlan_filtering enabled."
    else
      echo "vlan_filtering not supported, creating bridge br0 without vlan_filtering."
      ip link add name br0 type bridge
    fi

    # For all switch ports
    for iface in $SW_IF; do
        iface=$(echo "$iface" | xargs)  # Trim whitespace
        attach_eth_if "$iface" "$iface" # Move iface from host to container
        ip link set dev "$iface" master br0 # Add iface to bridge
    done
    IFS="$OLD_IFS"

    ip link set dev br0 up
fi

# Start processes
exec multirun \
  "clixon_backend -F -D default -s startup" \
  "/usr/sbin/sshd -D" \
  "ttyd --writable --port 2000 --debug 1 sh -c 'cat /etc/motd && clixon_cli'" \
  "nginx"


# exec multirun \
#   "clixon_backend -F -D restconf -s startup" \
#   "/usr/sbin/sshd -D" \
#   "wssh --address='127.0.0.1' --port=2000" \
#   "nginx" \
#   "snmpd -f -Lo -I -ifTable -I -system_mib -I -sysORTable" \
#   "sh -c 'sleep 10 && clixon_snmp -f /usr/local/etc/clixon/ietf-ip.xml -D init'"