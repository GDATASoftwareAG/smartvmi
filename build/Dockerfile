FROM ubuntu:jammy

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y -q \
    bison \
    clang \
    clang-tidy \
    cmake \
    curl \
    flex \
    gcovr \
    git \
    g++ \
    libglib2.0-dev \
    libjson-c-dev \
    libtclap-dev \
    libtool \
    libvirt-dev \
    libyara-dev \
    libxen-dev \
    make \
    pkg-config \
    sudo \
    wget \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/bitdefender/libkvmi.git \
    && cd libkvmi \
    && ./bootstrap \
    && ./configure \
    && make \
    && make install \
    && cd - \
    && rm -rf libkvmi

COPY rust_exec.sh /root/rust_exec.sh
SHELL ["/bin/bash", "-c"]
RUN RUSTUP_HOME=/opt/rust \
    && export RUSTUP_HOME \
    && CARGO_HOME=/opt/rust \
    && export CARGO_HOME \
    && curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --no-modify-path \
    && cargo_binaries=($(ls ${RUSTUP_HOME}/bin/)); \
    for i in "${cargo_binaries[@]}"; do \
        cp /root/rust_exec.sh "/usr/local/bin/$i" && chmod +x "/usr/local/bin/$i"; \
    done \
    && rm -f /root/rust_exec.sh
