#!/bin/sh

URL_BASE="localhost:8888"

getName() {
   curl -X 'PATCH' \
   "$URL_BASE/rest/restconf/data/openconfig-interfaces:interfaces/interface=lan1/config/name" \
   -H 'accept: application/yang-data+json' \
   -H 'Content-Type: application/yang-data+json' \
   -d '{
   "openconfig-interfaces:name": "test0"
   }'
}

pushVLAN() {
   curl -X 'PATCH' \
   "$URL_BASE/rest/restconf/data/openconfig-interfaces:interfaces/interface=lan1/openconfig-if-ethernet:ethernet/openconfig-vlan:switched-vlan/config" \
   -H 'accept: application/yang-data+json' \
   -H 'Content-Type: application/yang-data+json' \
   -d '{
      "openconfig-vlan:config": {
         "access-vlan": "30"
      }
   }'
}

commit() {
curl -X POST \
  -u admin:admin \
  -H "Content-Type: application/yang-data+json" \
  -H "Accept: application/yang-data+json" \
  $URL_BASE/rest/restconf/operations/ietf-netconf:commit \
  -d '{
        "ietf-netconf:input": {}
      }'
}

getVLAN() {
   curl -X 'GET' \
      "$URL_BASE/rest/restconf/data/openconfig-interfaces:interfaces/interface=lan1/openconfig-if-ethernet:ethernet/openconfig-vlan:switched-vlan" \
      -H 'accept: application/yang-data+json'
}


usage() {
   echo "Usage: $0 [-a] [-b] [-c] [-d] [-h]"
   exit 1
}

while getopts "abcdh" opt; do
   case "$opt" in
      a) getName ;;
      b) pushVLAN ;;
      c) getVLAN ;;
      d) commit ;;
      h) usage ;;
      *) usage ;;
   esac
done