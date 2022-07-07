FROM ubuntu:focal

ARG DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    python3 \
    python3-setuptools \
    python3-pip \
    python3-gi \
    python3-venv \
    python3-pkg-resources \
    git \
    ninja-build \
    libasound2 \
    libfftw3-3 \
    ca-certificates \
    libglib2.0-dev \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    libgstreamer1.0-0 \
    gstreamer1.0-tools \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    libgtest-dev \
    libjson-glib-dev \
    ssh \
    valgrind\
    wget

RUN pip3 install --no-cache-dir \
    meson \
    gcovr

RUN groupadd -g 1002 builder
RUN useradd -u 1002 -g 1002 -ms /bin/bash builder

USER builder
