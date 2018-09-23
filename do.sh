#!/usr/bin/env bash

/opt/eosio/bin/scripts/deploy_contract.sh notechain notechainacc notechainwal $(cat notechain_wallet_password.txt)
