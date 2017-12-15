#!/bin/bash

VERSION=`cat /etc/deipdversion`

DEIPD="/usr/local/deipd-full/bin/deipd"

chown -R deipd:deipd $HOME

# clean out data dir since it may be semi-persistent block storage on the ec2 with stale data
rm -rf $HOME/*

# seed nodes come from doc/seednodes.txt which is
# installed by docker into /etc/deipd/seednodes.txt
SEED_NODES="$(cat /etc/deipd/seednodes.txt | awk -F' ' '{print $1}')"

ARGS=""

# if user did not pass in any desired
# seed nodes, use the ones above:
if [[ -z "$DEIPD_SEED_NODES" ]]; then
    for NODE in $SEED_NODES ; do
        ARGS+=" --seed-node=$NODE"
    done
fi

# if user did pass in desired seed nodes, use
# the ones the user specified:
if [[ ! -z "$DEIPD_SEED_NODES" ]]; then
    for NODE in $DEIPD_SEED_NODES ; do
        ARGS+=" --seed-node=$NODE"
    done
fi

NOW=`date +%s`
DEIPD_FEED_START_TIME=`expr $NOW - 1209600`

ARGS+=" --follow-start-feeds=$DEIPD_FEED_START_TIME"

ARGS+=" --disable-get-block"

# overwrite local config with image one
cp /etc/deipd/fullnode.config.ini $HOME/config.ini

chown deipd:deipd $HOME/config.ini

cd $HOME

mv /etc/nginx/nginx.conf /etc/nginx/nginx.original.conf
cp /etc/nginx/deipd.nginx.conf /etc/nginx/nginx.conf

# get blockchain state from an S3 bucket
echo deipd: beginning download and decompress of s3://$S3_BUCKET/blockchain-$VERSION-latest.tar.bz2
if [[ "$USE_RAMDISK" ]]; then
  mkdir -p /mnt/ramdisk
  mount -t ramfs -o size=${RAMDISK_SIZE_IN_MB:-51200}m ramfs /mnt/ramdisk
  ARGS+=" --shared-file-dir=/mnt/ramdisk/blockchain"
  s3cmd get s3://$S3_BUCKET/blockchain-$VERSION-latest.tar.bz2 - | pbzip2 -m2000dc | tar x --wildcards 'blockchain/block*' -C /mnt/ramdisk 'blockchain/shared*'
  chown -R deipd:deipd /mnt/ramdisk/blockchain
else
  s3cmd get s3://$S3_BUCKET/blockchain-$VERSION-latest.tar.bz2 - | pbzip2 -m2000dc | tar x
fi
if [[ $? -ne 0 ]]; then
  if [[ ! "$SYNC_TO_S3" ]]; then
    echo notifyalert deipd: unable to pull blockchain state from S3 - exiting
    exit 1
  else
    echo notifydeipdsync deipdsync: shared memory file for $VERSION not found, creating a new one by replaying the blockchain
    mkdir blockchain
    aws s3 cp s3://$S3_BUCKET/block_log-latest blockchain/block_log
    if [[ $? -ne 0 ]]; then
      echo notifydeipdsync deipdsync: unable to pull latest block_log from S3, will sync from scratch.
    else
      ARGS+=" --replay-blockchain --force-validate"
    fi
    touch /tmp/isnewsync
  fi
fi

cd $HOME

if [[ "$SYNC_TO_S3" ]]; then
  touch /tmp/issyncnode
  chown www-data:www-data /tmp/issyncnode
fi

chown -R deipd:deipd $HOME/*

# start multiple read-only instances based on the number of cores
# attach to the local interface since a proxy will be used to loadbalance
if [[ "$USE_MULTICORE_READONLY" ]]; then
    exec chpst -udeipd \
        $DEIPD \
            --rpc-endpoint=127.0.0.1:8091 \
            --p2p-endpoint=0.0.0.0:2001 \
            --data-dir=$HOME \
            $ARGS \
            $DEIPD_EXTRA_OPTS \
            2>&1 &
    # sleep for a moment to allow the writer node to be ready to accept connections from the readers
    sleep 30
    PORT_NUM=8092
    cp /etc/nginx/healthcheck.conf.template /etc/nginx/healthcheck.conf
    CORES=$(nproc)
    PROCESSES=$((CORES * 4))
    for (( i=2; i<=$PROCESSES; i++ ))
      do
        echo server 127.0.0.1:$PORT_NUM\; >> /etc/nginx/healthcheck.conf
        ((PORT_NUM++))
    done
    echo } >> /etc/nginx/healthcheck.conf
    PORT_NUM=8092
    for (( i=2; i<=$PROCESSES; i++ ))
      do
        exec chpst -udeipd \
        $DEIPD \
          --rpc-endpoint=127.0.0.1:$PORT_NUM \
          --data-dir=$HOME \
          $ARGS \
          --read-forward-rpc=127.0.0.1:8091 \
          --read-only \
          2>&1 &
          ((PORT_NUM++))
          sleep 1
    done
    # start nginx now that the config file is complete with all endpoints
    # all of the read-only processes will connect to the write node onport 8091
    # nginx will balance all incoming traffic on port 8090
    rm /etc/nginx/sites-enabled/default
    cp /etc/nginx/healthcheck.conf /etc/nginx/sites-enabled/default
    /etc/init.d/fcgiwrap restart
    service nginx restart
    # start runsv script that kills containers if they die
    mkdir -p /etc/service/deipd
    cp /usr/local/bin/paas-sv-run.sh /etc/service/deipd/run
    chmod +x /etc/service/deipd/run
    runsv /etc/service/deipd
else
    cp /etc/nginx/healthcheck.conf.template /etc/nginx/healthcheck.conf
    echo server 127.0.0.1:8091\; >> /etc/nginx/healthcheck.conf
    echo } >> /etc/nginx/healthcheck.conf
    rm /etc/nginx/sites-enabled/default
    cp /etc/nginx/healthcheck.conf /etc/nginx/sites-enabled/default
    /etc/init.d/fcgiwrap restart
    service nginx restart
    exec chpst -udeipd \
        $DEIPD \
            --rpc-endpoint=0.0.0.0:8091 \
            --p2p-endpoint=0.0.0.0:2001 \
            --data-dir=$HOME \
            $ARGS \
            $DEIPD_EXTRA_OPTS \
            2>&1&
    SAVED_PID=`pgrep -f p2p-endpoint`
    echo $SAVED_PID >> /tmp/deipdpid
    mkdir -p /etc/service/deipd
    if [[ ! "$SYNC_TO_S3" ]]; then
      cp /usr/local/bin/paas-sv-run.sh /etc/service/deipd/run
    else
      cp /usr/local/bin/sync-sv-run.sh /etc/service/deipd/run
    fi
    chmod +x /etc/service/deipd/run
    runsv /etc/service/deipd
fi
