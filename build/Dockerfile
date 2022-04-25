FROM ubuntu:jammy

ARG USERNAME=builder

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
    && rm -rf /var/lib/apt/lists/*

RUN git clone https://github.com/bitdefender/libkvmi.git \
    && cd libkvmi \
    && ./bootstrap \
    && ./configure \
    && make \
    && make install \
    && cd - \
    && rm -rf libkvmi

RUN useradd -m $USERNAME \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

USER $USERNAME

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y
