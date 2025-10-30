#!/bin/sh

URL_BASE="localhost:8888"

# curl -X 'PATCH' \
#   "$URL_BASE/rest/restconf/data/openconfig-interfaces:interfaces/interface=lan1/config/name" \
#   -H 'accept: application/yang-data+json' \
#   -H 'Content-Type: application/yang-data+json' \
#   -d '{
#   "openconfig-interfaces:name": "test0"
# }'

curl -X 'PATCH' \
  "$URL_BASE/rest/restconf/data/openconfig-interfaces:interfaces/interface=lan1/ethernet/switched-vlan/config" \
  -H 'accept: application/yang-data+json' \
  -H 'Content-Type: application/yang-data+json' \
  -d '{
   "openconfig-vlan:config": {
      "openconfig-vlan:interface-mode": "TRUNK"
   }
}'