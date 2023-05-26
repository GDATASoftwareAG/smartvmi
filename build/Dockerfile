FROM ubuntu:lunar

ARG LLVM_MAJOR=16
ENV LLVM_MAJOR=${LLVM_MAJOR}

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y -q \
    bison \
    clang-${LLVM_MAJOR} \
    cmake \
    curl \
    flex \
    gcovr \
    git \
    g++ \
    libc++-${LLVM_MAJOR}-dev \
    libglib2.0-dev \
    libjson-c-dev \
    libtclap-dev \
    libtool \
    libvirt-dev \
    libyara-dev \
    libxen-dev \
    lldb-${LLVM_MAJOR} \
    make \
    ninja-build \
    pkg-config \
    protobuf-compiler \
    sudo \
    unzip \
    wget \
    && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_MAJOR} 100 \
    && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_MAJOR} 100 \
    && update-alternatives --install /usr/bin/clang-cpp clang-cpp /usr/bin/clang-cpp-${LLVM_MAJOR} 100 \
    && update-alternatives --install /usr/bin/lldb lldb /usr/bin/lldb-${LLVM_MAJOR} 100

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
