scp -i ~/.ssh/rdb_duxi.pem -r contracts ubuntu@13.59.177.32:/home/ubuntu/eos/eosio_docker/
scp -i ~/.ssh/rdb_duxi.pem server.sh ubuntu@13.59.177.32:/home/ubuntu/eos/server.sh
scp -i ~/.ssh/rdb_duxi.pem do.sh ubuntu@13.59.177.32:/home/ubuntu/eos/do.sh
ssh -i ~/.ssh/rdb_duxi.pem ubuntu@13.59.177.32 "/home/ubuntu/eos/server.sh"
