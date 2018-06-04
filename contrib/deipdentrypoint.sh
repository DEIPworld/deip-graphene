#!/bin/bash

echo /tmp/core | tee /proc/sys/kernel/core_pattern
ulimit -c unlimited

# if we're not using PaaS mode then start deipd traditionally with sv to control it
if [[ ! "$USE_PAAS" ]]; then
  mkdir -p /etc/service/deipd
  cp /usr/local/bin/deip-sv-run.sh /etc/service/deipd/run
  cp /usr/local/bin/config.ini /etc/service/deipd
  chmod +x /etc/service/deipd/run
  runsv /etc/service/deipd
else
  /usr/local/bin/startpaasdeipd.sh
fi
