FROM phusion/baseimage:0.9.19

#ARG DEIPD_BLOCKCHAIN=https://example.com/deipd-blockchain.tbz2

ENV LANG=en_US.UTF-8

RUN \
    apt-get update && \
    apt-get install -y \
        autoconf \
        automake \
        autotools-dev \
        bsdmainutils \
        build-essential \
        cmake \
        doxygen \
        git \
        libboost-all-dev \
        libreadline-dev \
        libssl-dev \
        libtool \
        ncurses-dev \
        pbzip2 \
        pkg-config \
        python3 \
        python3-dev \
        python3-jinja2 \
        python3-pip \
        nginx \
        fcgiwrap \
        s3cmd \
        awscli \
        jq \
        wget \
        gdb \
    && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* && \
    pip3 install gcovr

ADD . /usr/local/src/deip

RUN \
    cd /usr/local/src/deip && \
    git submodule update --init --recursive && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_BUILD_TYPE=Debug \
        -DBUILD_DEIP_TESTNET=ON \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=ON \
        .. && \
    make -j$(nproc) deipd && \
    cd /usr/local/src/deip && \
    doxygen && \
    programs/build_helpers/check_reflect.py && \
    programs/build_helpers/get_config_check.sh && \
    rm -rf /usr/local/src/deip/build

RUN \
    cd /usr/local/src/deip && \
    git submodule update --init --recursive && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/deipd-default \
        -DCMAKE_BUILD_TYPE=Debug \
        -DLOW_MEMORY_NODE=ON \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=ON \
        -DBUILD_DEIP_TESTNET=OFF \
        .. \
    && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    ( /usr/local/deipd-default/bin/deipd --version \
      | grep -o '[0-9]*\.[0-9]*\.[0-9]*' \
      && echo '_' \
      && git rev-parse --short HEAD ) \
      | sed -e ':a' -e 'N' -e '$!ba' -e 's/\n//g' \
      > /etc/deipdversion && \
    cat /etc/deipdversion && \
    rm -rfv build && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/deipd-full \
        -DCMAKE_BUILD_TYPE=Debug \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=OFF \
        -DSKIP_BY_TX_ID=ON \
        -DBUILD_DEIP_TESTNET=OFF \
        .. \
    && \
    make -j$(nproc) && \
    make install && \
    rm -rf /usr/local/src/deip

RUN \
    apt-get remove -y \
        automake \
        autotools-dev \
        bsdmainutils \
        build-essential \
        cmake \
        doxygen \
        dpkg-dev \
        git \
        libboost-all-dev \
        libc6-dev \
        libexpat1-dev \
        libgcc-5-dev \
        libhwloc-dev \
        libibverbs-dev \
        libicu-dev \
        libltdl-dev \
        libncurses5-dev \
        libnuma-dev \
        libopenmpi-dev \
        libpython-dev \
        libpython2.7-dev \
        libreadline-dev \
        libreadline6-dev \
        libssl-dev \
        libstdc++-5-dev \
        libtinfo-dev \
        libtool \
        linux-libc-dev \
        m4 \
        make \
        manpages \
        manpages-dev \
        mpi-default-dev \
        python-dev \
        python2.7-dev \
        python3-dev \
    && \
    apt-get autoremove -y && \
    rm -rf \
        /var/lib/apt/lists/* \
        /tmp/* \
        /var/tmp/* \
        /var/cache/* \
        /usr/include \
        /usr/local/include

RUN useradd -s /bin/bash -m -d /var/lib/deipd deipd

RUN mkdir /var/cache/deipd && \
    chown deipd:deipd -R /var/cache/deipd

# add blockchain cache to image
#ADD $DEIPD_BLOCKCHAIN /var/cache/deipd/blocks.tbz2

ENV HOME /var/lib/deipd
RUN chown deipd:deipd -R /var/lib/deipd

VOLUME ["/var/lib/deipd"]

# rpc service:
EXPOSE 8090
# p2p service:
EXPOSE 2001

# add seednodes from documentation to image
ADD doc/seednodes.txt /etc/deipd/seednodes.txt

# the following adds lots of logging info to stdout
ADD contrib/config-for-docker.ini /etc/deipd/config.ini
ADD contrib/fullnode.config.ini /etc/deipd/fullnode.config.ini

# add normal startup script that starts via sv
ADD contrib/deipd.run /usr/local/bin/deip-sv-run.sh
RUN chmod +x /usr/local/bin/deip-sv-run.sh

# add nginx templates
ADD contrib/deipd.nginx.conf /etc/nginx/deipd.nginx.conf
ADD contrib/healthcheck.conf.template /etc/nginx/healthcheck.conf.template

# add config
ADD config.ini /usr/local/deipd-default/bin/config.ini
ADD config.ini /usr/local/deipd-full/bin/config.ini

# add PaaS startup script and service script
ADD contrib/startpaasdeipd.sh /usr/local/bin/startpaasdeipd.sh
ADD contrib/paas-sv-run.sh /usr/local/bin/paas-sv-run.sh
ADD contrib/sync-sv-run.sh /usr/local/bin/sync-sv-run.sh
ADD contrib/healthcheck.sh /usr/local/bin/healthcheck.sh
RUN chmod +x /usr/local/bin/startpaasdeipd.sh
RUN chmod +x /usr/local/bin/paas-sv-run.sh
RUN chmod +x /usr/local/bin/sync-sv-run.sh
RUN chmod +x /usr/local/bin/healthcheck.sh

# new entrypoint for all instances
# this enables exitting of the container when the writer node dies
# for PaaS mode (elasticbeanstalk, etc)
# AWS EB Docker requires a non-daemonized entrypoint
ADD contrib/deipdentrypoint.sh /usr/local/bin/deipdentrypoint.sh
RUN chmod +x /usr/local/bin/deipdentrypoint.sh
CMD /usr/local/bin/deipdentrypoint.sh




# FROM phusion/baseimage:latest

# #ARG DEIPD_BLOCKCHAIN=https://example.com/deipd-blockchain.tbz2

# ENV LANG=en_US.UTF-8

# RUN \
#         apt-get update && \
#         apt-get install -y \
#             autoconf \
#             automake \
#             autotools-dev \
#             bsdmainutils \
#             build-essential \
#             cmake \
#             doxygen \
#             git \
#             libboost-all-dev \
#             libreadline-dev \
#             libssl-dev \
#             libtool \
#             ncurses-dev \
#             pbzip2 \
#             pkg-config \
#             python3 \
#             python3-dev \
#             python3-jinja2 \
#             python3-pip \
#             python-pip \
#             nginx \
#             fcgiwrap \
#             s3cmd \
#             awscli \
#             jq \
#             wget \
#             gdb \
#         && \
#         apt-get clean && \
#         rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* && \
#         pip3 install gcovr

# ADD . /usr/local/src/deip

# RUN \
#     cd /usr/local/src/deip && \
#     mkdir build && \
#     cd build && \
#     cmake \
#         -DCMAKE_BUILD_TYPE=Debug \
#         -DENABLE_COVERAGE_TESTING=ON \
#         -DLOW_MEMORY_NODE=OFF \
#         -DCLEAR_VOTES=ON \
#         -DSKIP_BY_TX_ID=ON \
#         .. && \
#     make -j$(nproc) wallet_tests chain_test test_fixed_string && \   
#     ./tests/chain_test && \
#     ./tests/wallet_tests && \
#     cd /usr/local/src/deip && \
#     doxygen && \
#     programs/build_helpers/check_reflect.py && \
#     programs/build_helpers/get_config_check.sh && \
#     cd /usr/local/src/deip && \
#     rm -rf /usr/local/src/deip/build

# RUN \
#     cd /usr/local/src/deip && \
#     mkdir build && \
#     cd build && \
#     cmake \
#         -DCMAKE_INSTALL_PREFIX=/usr/local/deipd-default \
#         -DCMAKE_BUILD_TYPE=Release \
#         -DLOW_MEMORY_NODE=ON \
#         -DCLEAR_VOTES=ON \
#         -DSKIP_BY_TX_ID=ON \
#         .. && \
#     make -j$(nproc) && \
#     ./tests/chain_test && \
#     ./tests/wallet_tests && \
#     ./programs/util/test_fixed_string && \
#     make install && \
# 	rm -rf /usr/local/src/deip/build

# RUN \
# 	cd /usr/local/src/deip && \
# 	mkdir build && \
#     cd build && \
#     cmake \
#         -DCMAKE_INSTALL_PREFIX=/usr/local/deipd-full \
#         -DCMAKE_BUILD_TYPE=Release \
#         -DLOW_MEMORY_NODE=OFF \
#         -DCLEAR_VOTES=OFF \
#         -DSKIP_BY_TX_ID=ON \
#         .. && \
#     make -j$(nproc) && \
#     ./tests/chain_test && \
#     ./tests/wallet_tests && \
#     ./programs/util/test_fixed_string && \
#     make install && \
#     cd / && \
#     rm -rf /usr/local/src/deip

# RUN \
#     ( /usr/local/deipd-full/bin/deipd --version \
#       | grep -o '[0-9]*\.[0-9]*\.[0-9]*' \
#       && echo '_' \
#       && git rev-parse --short HEAD ) \
#       | sed -e ':a' -e 'N' -e '$!ba' -e 's/\n//g' \
#       > /etc/deipdversion && \
#     cat /etc/deipdversion

# RUN \
#         apt-get remove -y \
#             automake \
#             autotools-dev \
#             bsdmainutils \
#             build-essential \
#             cmake \
#             doxygen \
#             dpkg-dev \
#             git \
#             libboost-all-dev \
#             libc6-dev \
#             libexpat1-dev \
#             libgcc-5-dev \
#             libhwloc-dev \
#             libibverbs-dev \
#             libicu-dev \
#             libltdl-dev \
#             libncurses5-dev \
#             libnuma-dev \
#             libopenmpi-dev \
#             libpython-dev \
#             libpython2.7-dev \
#             libreadline-dev \
#             libreadline6-dev \
#             libssl-dev \
#             libstdc++-5-dev \
#             libtinfo-dev \
#             libtool \
#             linux-libc-dev \
#             m4 \
#             make \
#             manpages \
#             manpages-dev \
#             mpi-default-dev \
#             python-dev \
#             python2.7-dev \
#             python3-dev \
#         && \
#         apt-get autoremove -y && \
#         rm -rf \
#             /var/lib/apt/lists/* \
#             /tmp/* \
#             /var/tmp/* \
#             /var/cache/* \
#             /usr/include \
#             /usr/local/include

# RUN useradd -s /bin/bash -m -d /var/lib/deipd deipd

# RUN mkdir /var/cache/deipd && \
#           chown deipd:deipd -R /var/cache/deipd

# # add blockchain cache to image
# #ADD $DEIPD_BLOCKCHAIN /var/cache/deipd/blocks.tbz2

# ENV HOME /var/lib/deipd
# RUN chown deipd:deipd -R /var/lib/deipd

# VOLUME ["/var/lib/deipd"]

# # rpc service:
# EXPOSE 8090
# # p2p service:
# EXPOSE 2001

# # add seednodes from documentation to image
# ADD doc/seednodes.txt /etc/deipd/seednodes.txt

# # the following adds lots of logging info to stdout
# ADD contrib/fullnode.config.ini /etc/deipd/fullnode.config.ini

# # add normal startup script that start via runsv
# # RUN mkdir /etc/sv/deipd
# # RUN mkdir /etc/sv/deipd/log
# # ADD contrib/runsv/deipd.run /etc/sv/deipd/run
# # ADD contrib/runsv/deipd-log.run /etc/sv/deipd/log/run
# # ADD contrib/runsv/deipd-log.config /etc/sv/deipd/log/config
# # RUN chmod +x /etc/sv/deipd/run /etc/sv/deipd/log/run

# # add nginx templates
# ADD contrib/deipd.nginx.conf /etc/nginx/deipd.nginx.conf
# ADD contrib/healthcheck.conf.template /etc/nginx/healthcheck.conf.template

# # add healthcheck script
# ADD contrib/healthcheck.sh /usr/local/bin/healthcheck.sh
# RUN chmod +x /usr/local/bin/healthcheck.sh

# # new entrypoint for all instances
# # this enables exitting of the container when the writer node dies
# # for PaaS mode (elasticbeanstalk, etc)
# # AWS EB Docker requires a non-daemonized entrypoint
# ADD contrib/deipdentrypoint.sh /usr/local/bin/deipdentrypoint.sh
# RUN chmod +x /usr/local/bin/deipdentrypoint.sh
# CMD /usr/local/bin/deipdentrypoint.shFROM phusion/baseimage:latest

# #ARG DEIPD_BLOCKCHAIN=https://example.com/deipd-blockchain.tbz2

# ENV LANG=en_US.UTF-8

# RUN \
#         apt-get update && \
#         apt-get install -y \
#             autoconf \
#             automake \
#             autotools-dev \
#             bsdmainutils \
#             build-essential \
#             cmake \
#             doxygen \
#             git \
#             libboost-all-dev \
#             libreadline-dev \
#             libssl-dev \
#             libtool \
#             ncurses-dev \
#             pbzip2 \
#             pkg-config \
#             python3 \
#             python3-dev \
#             python3-jinja2 \
#             python3-pip \
#             python-pip \
#             nginx \
#             fcgiwrap \
#             s3cmd \
#             awscli \
#             jq \
#             wget \
#             gdb \
#         && \
#         apt-get clean && \
#         rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/* && \
#         pip3 install gcovr

# ADD . /usr/local/src/deip

# RUN \
#     cd /usr/local/src/deip && \
#     mkdir build && \
#     cd build && \
#     cmake \
#         -DCMAKE_BUILD_TYPE=Debug \
#         -DENABLE_COVERAGE_TESTING=ON \
#         -DLOW_MEMORY_NODE=OFF \
#         -DCLEAR_VOTES=ON \
#         -DSKIP_BY_TX_ID=ON \
#         .. && \
#     make -j$(nproc) wallet_tests chain_test test_fixed_string && \   
#     ./tests/chain_test && \
#     ./tests/wallet_tests && \
#     cd /usr/local/src/deip && \
#     doxygen && \
#     programs/build_helpers/check_reflect.py && \
#     programs/build_helpers/get_config_check.sh && \
#     cd /usr/local/src/deip && \
#     rm -rf /usr/local/src/deip/build

# RUN \
#     cd /usr/local/src/deip && \
#     mkdir build && \
#     cd build && \
#     cmake \
#         -DCMAKE_INSTALL_PREFIX=/usr/local/deipd-default \
#         -DCMAKE_BUILD_TYPE=Release \
#         -DLOW_MEMORY_NODE=ON \
#         -DCLEAR_VOTES=ON \
#         -DSKIP_BY_TX_ID=ON \
#         .. && \
#     make -j$(nproc) && \
#     ./tests/chain_test && \
#     ./tests/wallet_tests && \
#     ./programs/util/test_fixed_string && \
#     make install && \
# 	rm -rf /usr/local/src/deip/build

# RUN \
# 	cd /usr/local/src/deip && \
# 	mkdir build && \
#     cd build && \
#     cmake \
#         -DCMAKE_INSTALL_PREFIX=/usr/local/deipd-full \
#         -DCMAKE_BUILD_TYPE=Release \
#         -DLOW_MEMORY_NODE=OFF \
#         -DCLEAR_VOTES=OFF \
#         -DSKIP_BY_TX_ID=ON \
#         .. && \
#     make -j$(nproc) && \
#     ./tests/chain_test && \
#     ./tests/wallet_tests && \
#     ./programs/util/test_fixed_string && \
#     make install && \
#     cd / && \
#     rm -rf /usr/local/src/deip

# RUN \
#     ( /usr/local/deipd-full/bin/deipd --version \
#       | grep -o '[0-9]*\.[0-9]*\.[0-9]*' \
#       && echo '_' \
#       && git rev-parse --short HEAD ) \
#       | sed -e ':a' -e 'N' -e '$!ba' -e 's/\n//g' \
#       > /etc/deipdversion && \
#     cat /etc/deipdversion

# RUN \
#         apt-get remove -y \
#             automake \
#             autotools-dev \
#             bsdmainutils \
#             build-essential \
#             cmake \
#             doxygen \
#             dpkg-dev \
#             git \
#             libboost-all-dev \
#             libc6-dev \
#             libexpat1-dev \
#             libgcc-5-dev \
#             libhwloc-dev \
#             libibverbs-dev \
#             libicu-dev \
#             libltdl-dev \
#             libncurses5-dev \
#             libnuma-dev \
#             libopenmpi-dev \
#             libpython-dev \
#             libpython2.7-dev \
#             libreadline-dev \
#             libreadline6-dev \
#             libssl-dev \
#             libstdc++-5-dev \
#             libtinfo-dev \
#             libtool \
#             linux-libc-dev \
#             m4 \
#             make \
#             manpages \
#             manpages-dev \
#             mpi-default-dev \
#             python-dev \
#             python2.7-dev \
#             python3-dev \
#         && \
#         apt-get autoremove -y && \
#         rm -rf \
#             /var/lib/apt/lists/* \
#             /tmp/* \
#             /var/tmp/* \
#             /var/cache/* \
#             /usr/include \
#             /usr/local/include

# RUN useradd -s /bin/bash -m -d /var/lib/deipd deipd

# RUN mkdir /var/cache/deipd && \
#           chown deipd:deipd -R /var/cache/deipd

# # add blockchain cache to image
# #ADD $DEIPD_BLOCKCHAIN /var/cache/deipd/blocks.tbz2

# ENV HOME /var/lib/deipd
# RUN chown deipd:deipd -R /var/lib/deipd

# VOLUME ["/var/lib/deipd"]

# # rpc service:
# EXPOSE 8090
# # p2p service:
# EXPOSE 2001

# # add seednodes from documentation to image
# ADD doc/seednodes.txt /etc/deipd/seednodes.txt

# # the following adds lots of logging info to stdout
# ADD contrib/fullnode.config.ini /etc/deipd/fullnode.config.ini

# # add normal startup script that start via runsv
# # RUN mkdir /etc/sv/deipd
# # RUN mkdir /etc/sv/deipd/log
# # ADD contrib/runsv/deipd.run /etc/sv/deipd/run
# # ADD contrib/runsv/deipd-log.run /etc/sv/deipd/log/run
# # ADD contrib/runsv/deipd-log.config /etc/sv/deipd/log/config
# # RUN chmod +x /etc/sv/deipd/run /etc/sv/deipd/log/run

# # add nginx templates
# ADD contrib/deipd.nginx.conf /etc/nginx/deipd.nginx.conf
# ADD contrib/healthcheck.conf.template /etc/nginx/healthcheck.conf.template

# # add healthcheck script
# ADD contrib/healthcheck.sh /usr/local/bin/healthcheck.sh
# RUN chmod +x /usr/local/bin/healthcheck.sh

# # new entrypoint for all instances
# # this enables exitting of the container when the writer node dies
# # for PaaS mode (elasticbeanstalk, etc)
# # AWS EB Docker requires a non-daemonized entrypoint
# ADD contrib/deipdentrypoint.sh /usr/local/bin/deipdentrypoint.sh
# RUN chmod +x /usr/local/bin/deipdentrypoint.sh
# CMD /usr/local/bin/deipdentrypoint.sh