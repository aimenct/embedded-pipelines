FROM ubuntu:22.04

# Required to build Ubuntu without user prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install basic libraries 
RUN apt-get update && apt-get install -y software-properties-common
RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN apt-get update && apt-get install -y --no-install-recommends \
    curl \
    wget \
    python3-dev \
    python3-pip \
    git \
    ssh \
    libssl-dev \
    devscripts \
    build-essential \
    gdb \
    nano \
    libpthread-stubs0-dev \
    vim \
    htop \
    locales \
    x11-apps

RUN locale-gen en_US.UTF-8


# Install embedded-pipelines apt-get dependencies
RUN apt-get install -y --no-install-recommends \
    libgstreamer1.0-0 \
    libgstrtspserver-1.0-dev \
    libtool \
    libxml2-dev \
    libusb-1.0-0-dev \
    cmake \
    libyaml-cpp-dev \
    freeglut3-dev \
    libmodbus-dev \
    libopencv-dev \
    doxygen && \
    pip install sphinx

# Change working directory to build dependencies
WORKDIR /tmp

# Install Open62541 library
RUN git clone --branch v1.4.1 https://github.com/open62541/open62541.git && \
    cd open62541 && \
    git submodule update --init --recursive && \
    mkdir build && cd build && \
    cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo -DUA_NAMESPACE_ZERO=FULL \
    -DUA_ENABLE_METHODCALLS=ON -DUA_ENABLE_NODEMANAGEMENT=ON -DUA_ENABLE_NODESETLOADER=ON .. && \
    make && \
    make install

# Install Aravis and dependencies
RUN pip install -U pip && \
    pip install meson ninja && \
    apt-get install -y --no-install-recommends \
    zlib1g-dev \
    libcanberra-gtk-module \
    libcanberra-gtk3-module \
    gstreamer1.0-gtk3 \
    libgtk-3-dev \
    usbutils \
    ninja-build \
    udev \
    gettext

RUN git clone --depth 1 --branch 0.8.31 https://github.com/AravisProject/aravis.git && \
    cd aravis && \
    meson build && \
    cd build && \
    ninja && \
    ninja install && \
    cp ../src/aravis.rules /etc/udev/rules.d/ &&\
    ldconfig

# Setup user account as the same as host
ARG USERNAME=user
ARG USER_UID=$USER_UID
ARG USER_GID=$USER_UID

# Create the user
RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    # [Optional] Add sudo support. Omit if you don't need to install software after connecting.
    && apt-get update \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME

RUN apt-get install -y --no-install-recommends \
valgrind

USER $USERNAME

RUN pip install breathe sphinx_rtd_theme

RUN ["/bin/bash"]
