FROM phusion/baseimage:0.9.19

ARG build_testnet=OFF
ENV LANG=en_US.UTF-8
ENV BUILD_TESTNET=${build_testnet}

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

ADD . /deip-blockchain


RUN \
    #
    # Check reflections & config
    echo && echo '------ Check reflections & config ------' && \
    cd /deip-blockchain && \
    doxygen && \
    programs/build_helpers/check_reflect.py && \
    programs/build_helpers/get_config_check.sh

RUN \
    cd /deip-blockchain && \
    git submodule update --init --recursive && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_BUILD_TYPE=Release \
        -DLOW_MEMORY_NODE=ON \
        -DCLEAR_VOTES=ON \
        -DSKIP_BY_TX_ID=ON \
        -DBUILD_DEIP_TESTNET=$BUILD_TESTNET \
        .. \
    && \
    make -j$(nproc) && \
    make install && \
    cd .. && \
    rm -rfv build && \
    mkdir build && \
    cd build && \
    cmake \
        -DCMAKE_INSTALL_PREFIX=/usr/local/deipd-full \
        -DCMAKE_BUILD_TYPE=Release \
        -DLOW_MEMORY_NODE=OFF \
        -DCLEAR_VOTES=OFF \
        -DSKIP_BY_TX_ID=ON \
        -DBUILD_DEIP_TESTNET=$BUILD_TESTNET \
        .. \
    && \
    make -j$(nproc) && \
    make install && \
    rm -rf /deip-blockchain

RUN \
    (/usr/local/deipd-default/bin/deipd --version \
      | grep -o '[0-9]*\.[0-9]*\.[0-9]*' \
      && echo '_' \
      && git rev-parse --short HEAD ) \
      | sed -e ':a' -e 'N' -e '$!ba' -e 's/\n//g' \
      > /etc/deipdversion && \
    cat /etc/deipdversion

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

# new entrypoint for all instances
# this enables exitting of the container when the writer node dies
ADD contrib/deipdentrypoint.sh /usr/local/bin/deipdentrypoint.sh
RUN chmod +x /usr/local/bin/deipdentrypoint.sh
CMD /usr/local/bin/deipdentrypoint.sh