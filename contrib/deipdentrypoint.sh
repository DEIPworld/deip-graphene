#!/bin/bash

echo /tmp/core | tee /proc/sys/kernel/core_pattern
ulimit -c unlimited

mkdir -p /etc/service/deipd
cp /usr/local/bin/deipd.run /etc/service/deipd/run
chmod +x /etc/service/deipd/run
runsv /etc/service/deipd

