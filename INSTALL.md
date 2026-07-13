# How to install `axon`

Installation notes tested on **Red Hat Enterprise Linux 9** (and compatible: Rocky Linux 9, AlmaLinux 9). For RHEL/CentOS 7 see the legacy `INSTALL-el9.md`.

1. [Preparing the Environment](#toc1)
	1. [Extra repos](#toc11)
	2. [Confluent repo](#toc12)
	3. [Build system](#toc13)
2. [Installing the dependencies](#toc2)
	1. [From repo](#toc21)
	2. [avro-c](#toc22)
	3. [libserdes](#toc23)
	4. [libsmb2](#toc24)
	5. [scylla-cpp-driver](#toc25)
	6. [AWS C++ SDK](#toc26)
	7. [Oracle Instant Client (OCI)](#toc27)
	8. [libhdfs3 and libgsasl](#toc28)
3. [Compiling the library](#toc3)


## Preparing the Environment <a name="toc1"></a>

### ⭐ Extra repos <a name="toc11"></a>

```bash
sudo dnf install epel-release dnf-plugins-core wget nano
```

### ⭐ Confluent repo <a name="toc12"></a>

[Confluent](https://www.confluent.io/) publishes RPM packages for librdkafka, libserdes, and avro-c that are used by the Kafka connector.

```bash
sudo rpm --import https://packages.confluent.io/rpm/7.8/archive.key

sudo tee /etc/yum.repos.d/confluent.repo << 'EOF'
[Confluent]
name=Confluent repository
baseurl=https://packages.confluent.io/rpm/7.8
gpgcheck=1
gpgkey=https://packages.confluent.io/rpm/7.8/archive.key
enabled=1

[Confluent-Clients]
name=Confluent Clients repository
baseurl=https://packages.confluent.io/clients/rpm/centos/$releasever/$basearch
gpgcheck=1
gpgkey=https://packages.confluent.io/clients/rpm/archive.key
enabled=1
EOF
```

> ⚠️ When pasting the heredoc, confirm `$releasever` and `$basearch` are written literally — they are expanded by yum at runtime, not by your shell.

### ⭐ Build system <a name="toc13"></a>

RHEL 9 ships GCC 11 with full C++17 support. No devtoolset is required.

```bash
sudo dnf install automake bison cmake gcc-c++ gettext gettext-devel git \
     gperf libtool readline readline-devel rpmdevtools rpmlint
```


## Installing the dependencies <a name="toc2"></a>

### ✔️ From repo <a name="toc21"></a>

The following packages cover most of axon's runtime and build-time dependencies and are available from standard RHEL 9 + EPEL + Confluent repos:

```bash
sudo dnf install \
    bzip2 bzip2-devel \
    boost-devel boost-regex boost-iostreams boost-filesystem boost-json \
    file file-devel \
    hiredis hiredis-devel \
    jansson-devel \
    krb5-devel krb5-libs \
    libblkid-devel \
    libconfig libconfig-devel \
    libcurl-devel \
    libgcrypt-devel \
    libntlm-devel \
    librdkafka1 librdkafka-devel \
    librabbitmq librabbitmq-devel \
    libssh2-devel \
    libuuid libuuid-devel \
    libuv libuv-devel \
    libxml2 libxml2-devel \
    libzstd-devel \
    openldap openldap-devel \
    openssl-devel \
    protobuf protobuf-devel \
    cyrus-sasl-devel cyrus-sasl-gssapi cyrus-sasl-plain \
    sqlite-devel \
    xz-devel
```

### ✔️ avro-c <a name="toc22"></a>

Install from the Confluent repo if available:

```bash
sudo dnf install avro-c-devel
```

If the package is missing or too old (requires ≥ 1.8.0), build from source:

```bash
sudo dnf install jansson-devel

wget https://github.com/apache/avro/archive/refs/tags/release-1.11.4.tar.gz \
     -O avro-release-1.11.4.tar.gz
tar xf avro-release-1.11.4.tar.gz
cd avro-release-1.11.4/lang/c
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
sudo make install
```

### ✔️ libserdes <a name="toc23"></a>

Install from the Confluent repo:

```bash
sudo dnf install confluent-libserdes-devel
```

> ⚠️ The Confluent package sometimes sets incorrect permissions on the header directory. Check after install:
> ```bash
> ls -la /usr/include/libserdes/
> ```

If the package is missing, build from source:

```bash
wget https://github.com/confluentinc/libserdes/archive/refs/tags/v7.8.1.tar.gz \
     -O confluent-libserdes-7.8.1.tar.gz
tar xf confluent-libserdes-7.8.1.tar.gz && cd libserdes-7.8.1
./configure --prefix=/usr
sudo make install
```

### ✔️ libsmb2 <a name="toc24"></a>

[libsmb2](https://github.com/sahlberg/libsmb2) is a portable SMB2/3 client library. axon requires ≥ 4.0.0.

```bash
wget https://github.com/sahlberg/libsmb2/archive/refs/tags/v4.0.0.tar.gz
tar xf v4.0.0.tar.gz && cd libsmb2-4.0.0
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
sudo make install
```

### ✔️ scylla-cpp-driver <a name="toc25"></a>

The ScyllaDB C++ driver is a fork of the DataStax driver with shard-aware routing. axon requires ≥ 2.16.0.

```bash
sudo dnf install libuv-devel openssl-devel zlib-devel

wget https://github.com/scylladb/cpp-driver/archive/refs/tags/2.16.2b.tar.gz \
     -O scylla-cpp-driver-2.16.2b.tar.gz
tar xf scylla-cpp-driver-2.16.2b.tar.gz && cd cpp-driver-2.16.2b
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
sudo make install
```

### ✔️ AWS C++ SDK <a name="toc26"></a>

axon uses the `s3` and `kinesis` components. Build only those to keep compile time short. A known stable release is 1.11.345.

```bash
wget https://github.com/aws/aws-sdk-cpp/archive/refs/tags/1.11.345.tar.gz \
     -O aws-sdk-cpp-1.11.345.tar.gz
tar xf aws-sdk-cpp-1.11.345.tar.gz && cd aws-sdk-cpp-1.11.345
./prefetch_crt_dependency.sh
mkdir build && cd build
cmake .. \
    -DCMAKE_INSTALL_PREFIX=/usr \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_ONLY="core;s3;kinesis" \
    -DENABLE_TESTING=OFF \
    -Wno-dev
sudo make install
```

### ✔️ Oracle Instant Client (OCI) <a name="toc27"></a>

Oracle OCI is required for `axon::database::oracle` and `axon::stream::ocn`. Download RPMs from the [Oracle Instant Client download page](https://www.oracle.com/database/technologies/instant-client/linux-x86-64-downloads.html). You need:

- **Basic Package** — runtime libraries (required)
- **SDK Package** — headers and link libraries (required)

For Oracle 23c on RHEL 9:

```bash
sudo dnf install \
    https://download.oracle.com/otn_software/linux/instantclient/2360000/oracle-instantclient-basic-23.6.0.24.10-1.el9.x86_64.rpm \
    https://download.oracle.com/otn_software/linux/instantclient/2360000/oracle-instantclient-devel-23.6.0.24.10-1.el9.x86_64.rpm
```

Set the required environment variables (add to `~/.bashrc`):

```bash
export ORACLE_BASE=/usr/lib/oracle
export ORACLE_HOME=/usr/lib/oracle/23/client64
export LD_LIBRARY_PATH=$ORACLE_HOME/lib:$LD_LIBRARY_PATH
```

For Oracle 19c (still widely deployed):

```bash
sudo dnf install \
    https://download.oracle.com/otn_software/linux/instantclient/1919000/oracle-instantclient19.19-basic-19.19.0.0.0-1.x86_64.rpm \
    https://download.oracle.com/otn_software/linux/instantclient/1919000/oracle-instantclient19.19-devel-19.19.0.0.0-1.x86_64.rpm

export ORACLE_HOME=/usr/lib/oracle/19.19/client64
```

### ✔️ libhdfs3 and libgsasl <a name="toc28"></a>

The HDFS connector requires custom forks of these libraries. The upstream versions have broken Kerberos support.

#### libgsasl

The [Nokia fork](https://github.com/nokia/libgsasl) of libgsasl includes fixes for modern OpenSSL APIs that the original and the Brett Rosen fork do not have.

```bash
sudo dnf install krb5-devel openssl-devel

wget https://github.com/nokia/libgsasl/releases/download/v1.8.2/libgsasl-1.8.2.tar.gz
tar xf libgsasl-1.8.2.tar.gz && cd libgsasl-1.8.2
LDFLAGS="-lssl -lcrypto" CFLAGS="-g -O2 -fPIC" \
    ./configure --prefix=/usr --with-gssapi-impl=mit
make
sudo make install
```

#### libhdfs3

Use the [thegeekyboy fork](https://github.com/thegeekyboy/libhdfs3) which is kept in sync with Kerberos and protobuf compatibility fixes.

```bash
sudo dnf install libxml2-devel protobuf-devel boost-devel libcurl-devel libgsasl

git clone https://github.com/thegeekyboy/libhdfs3
cd libhdfs3
mkdir build && cd build
cmake .. -Wno-dev -DCMAKE_INSTALL_PREFIX=/usr
sudo make install
```


## Compiling the library <a name="toc3"></a>

Set the Oracle library path then build:

```bash
export LD_LIBRARY_PATH=/usr/lib:/usr/lib64:/usr/local/lib64:$ORACLE_HOME/lib:$LD_LIBRARY_PATH

git clone https://github.com/thegeekyboy/axon
cd axon && mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
sudo make install
```

### Debug build

```bash
cmake .. -DDEBUG=3 -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr
make
```

Debug levels:

| Level | Output |
|---|---|
| `DEBUG=0` | silent (default) |
| `DEBUG=1` | `DBGPRN` function-level debug |
| `DEBUG=2` | `WRNPRN` warnings |
| `DEBUG=3` | `INFPRN` + `BENCHMARK` timing |
| `DEBUG=4` | AWS SDK trace logging |

### Submodule / embedded build

When axon is added as a CMake submodule it automatically builds as a **static library**:

```cmake
add_subdirectory(axon)
target_link_libraries(my_target PRIVATE axon)
```

No `make install` is needed in this case.