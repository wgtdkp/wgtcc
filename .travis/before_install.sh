#!/bin/bash

install_latest_cmake() {
    wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -
    sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
    sudo apt-get update
    sudo apt-get install cmake

    # Override default cmake
    export PATH="/usr/bin:$PATH"
    cmake --version
}

sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
sudo apt-get update -y
sudo apt-get install gcc-5 g++-5 -y
sudo unlink /usr/bin/gcc
sudo ln -s /usr/bin/gcc-5 /usr/bin/gcc
sudo unlink /usr/bin/g++
sudo ln -s /usr/bin/g++-5 /usr/bin/g++
gcc --version
g++ --version

install_latest_cmake
