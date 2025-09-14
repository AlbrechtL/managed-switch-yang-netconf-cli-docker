#!/bin/sh

set -Eeuo pipefail

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
rm /etc/nginx/http.d/default.conf

# Start processes
exec multirun \
  "clixon_backend -F" \
  "/usr/sbin/sshd -D" \
  "wssh --address='127.0.0.1' --port=8888" \
  "nginx"

