# 1. Install build dependencies (example for Debian/Ubuntu)
```bash
sudo apt-get install git meson pkg-config libjson-c-dev libkeyutils-dev libkmod-dev libiniparser-dev libtracefs-dev libtraceevent-dev libnuma-dev uuid-dev libudev-dev asciidoctor libiniparser-dev
```

# 2. Clear previous version

```bash
sudo apt-get purge ndctl
sudo apt autoremove
```

# 3. Clone the official repository

```
git clone https://github.com/pmem/ndctl.git
cd ndctl
git checkout <expected version>
```

# 4. Configure and build

```bash
meson setup build
cd build
ninja
```

# 5. Install the new tools

```bash
sudo ninja install
sudo ldconfig
```