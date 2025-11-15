# Updating `ndctl` from source

Some CXL and advanced NUMA features depend on functionality that is only available in newer versions of `ndctl`. This document shows how to build and install `ndctl` from the official repository on Debian or Ubuntu like systems.

## 1. Install build dependencies

```bash
sudo apt-get install \
    git meson pkg-config \
    libjson-c-dev libkeyutils-dev libkmod-dev \
    libiniparser-dev libtracefs-dev libtraceevent-dev \
    libnuma-dev uuid-dev libudev-dev \
    asciidoctor
```

Adjust the package list if your distribution uses different names.

## 2. Remove old `ndctl`

It is safer to remove distribution packages first to avoid conflicts with the version built from source.

```bash
sudo apt-get purge ndctl
sudo apt autoremove
```

## 3. Clone the official repository

```bash
git clone https://github.com/pmem/ndctl.git
cd ndctl
git checkout <expected-version>   # for example v78 or a tag you need
```

Check the repository tags to choose the version that matches your requirements.

## 4. Configure and build

```bash
meson setup build
cd build
ninja
```

If configuration fails, recheck that all required `-dev` packages are installed.

## 5. Install the new tools

```bash
sudo ninja install
sudo ldconfig
```

After installation, confirm that the new `ndctl` and `cxl` is in effect:

```bash
ndctl --version
cxl --version
```