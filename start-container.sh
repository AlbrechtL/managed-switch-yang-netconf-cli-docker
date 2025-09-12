#!/bin/sh

set -Eeuo pipefail

# Check if SSH host keys exist in /etc/ssh, generate if missing
if ! ls /etc/ssh/ssh_host_*_key >/dev/null 2>&1; then
    echo "No SSH host keys found in /etc/ssh. Generating new SSH host keys..."
    ssh-keygen -A
fi

# Start processes
exec multirun \
  "clixon_backend -F" \
  "/usr/sbin/sshd -D"

