#!/usr/bin/env bash
set -Eeuo pipefail

trap 'error "Status $? while: $BASH_COMMAND (line $LINENO/$BASH_LINENO)"' ERR

# Docker environment variables
: "${SW_IF:=""}"         # Physical switch Ethernet interface names (comma separated)

