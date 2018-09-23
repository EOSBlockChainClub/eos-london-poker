#!/usr/bin/env bash

cd /home/ubuntu/eos/

docker cp do.sh eosio_notechain_container:/opt/eosio/bin/scripts/do.sh
docker exec -t eosio_notechain_container /opt/eosio/bin/scripts/do.sh