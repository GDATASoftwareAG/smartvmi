FROM ubuntu:latest

ARG LLVM_MAJOR=18
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
    libc++abi-${LLVM_MAJOR}-dev \
    libc++-${LLVM_MAJOR}-dev \
    libglib2.0-dev \
    libjson-c-dev \
    libtool \
    libvirt-dev \
    libxen-dev \
    lld-${LLVM_MAJOR} \
    make \
    ninja-build \
    pkg-config \
    protobuf-compiler \
    sudo \
    unzip \
    wget \
    zip \
    && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --install /usr/bin/clang clang /usr/bin/clang-${LLVM_MAJOR} 100 \
    && update-alternatives --install /usr/bin/clang++ clang++ /usr/bin/clang++-${LLVM_MAJOR} 100 \
    && update-alternatives --install /usr/bin/clang-cpp clang-cpp /usr/bin/clang-cpp-${LLVM_MAJOR} 100 \
    && update-alternatives --install /usr/bin/lld lld /usr/bin/lld-${LLVM_MAJOR} 100

RUN git clone https://github.com/bitdefender/libkvmi.git \
    && cd libkvmi \
    && ./bootstrap \
    && ./configure \
    && make \
    && make install \
    && cd - \
    && rm -rf libkvmi

RUN cd /usr/local/share/ && \
    git clone https://github.com/microsoft/vcpkg.git && \
    cd vcpkg && \
    ./bootstrap-vcpkg.sh --disableMetrics && \
    mv vcpkg /usr/local/bin/
ENV VCPKG_ROOT=/usr/local/share/vcpkg

COPY rust_exec.sh /root/rust_exec.sh
RUN export RUSTUP_HOME=/opt/rust && \
    export CARGO_HOME=/opt/rust && \
    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y --no-modify-path  && \
    bash -c \
    'cargo_binaries=($(ls ${RUSTUP_HOME}/bin/)); \
    for i in "${cargo_binaries[@]}"; do \
        cp /root/rust_exec.sh "/usr/local/bin/$i" && chmod +x "/usr/local/bin/$i"; \
    done' && \
    rm -f /root/rust_exec.sh
