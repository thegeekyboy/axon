# How to install `axon`

this installation notes are tested in centos 7 and redhat 7. this should work until 9.

## Preparing the Envioronment

### ⭐Extra repos

Some non-standard repos are needed to get the build system ready

```bash
$ sudo yum install epel-release centos-release-scl dnf-plugins-core wget nano
```

### ⭐Confluent

[confluent](https://www.confluent.io/) manages a repo of rpm packages to support their products. conveniently they have made it public domain and we can use them to avoid compiling from source.

detail guide [here](https://docs.confluent.io/platform/current/installation/overview.html)

```bash
$ wget --no-check-certificate https://packages.confluent.io/rpm/7.4/archive.key
$ sudo rpm --import archive.key

$ sudo cat << EOF >> /etc/yum.repos.d/Confluent.repo
[Confluent]
name=Confluent repository
baseurl=https://packages.confluent.io/rpm/7.4
gpgcheck=1
gpgkey=https://packages.confluent.io/rpm/7.4/archive.key
enabled=1
sslverify=0

[Confluent-Clients]
name=Confluent Clients repository
baseurl=https://packages.confluent.io/clients/rpm/centos/\$releasever/\$basearch
gpgcheck=1
gpgkey=https://packages.confluent.io/clients/rpm/archive.key
enabled=1
sslverify=0
EOF
```

### ⭐Build system

Since centos/redhat 7 do not come with C++17, need to install gcc 11 from scl/devtoolset

```bash
$ sudo yum install git cmake3 cmake gcc-c++ devtoolset-12 devtoolset-12-runtime devtoolset-12-gcc devtoolset-12-gcc-c++ devtoolset-12-libstdc++-devel devtoolset-12-make
```

## Installing the dependencies

#### ✔️ from repo
there are bunch of dependencies for axon to run correctly.

- [boost](https://www.boost.org/) (iostreams, system, thread, regex)
- [libconfig](http://hyperrealm.github.io/libconfig/)
- [libcurl](https://curl.se/libcurl/)
- [openssl](https://www.openssl.org/)
- [libgcrypt](https://gnupg.org/software/libgcrypt/index.html)
- [kerberos](https://web.mit.edu/kerberos/)
- [libntlm](https://gitlab.com/gsasl/libntlm/)
- [bzip2](http://www.bzip.org/)
- [zstd](http://facebook.github.io/zstd/)
- [avro](https://avro.apache.org/)
- [rdkafka](https://github.com/confluentinc/librdkafka)
- [libserdes](https://github.com/confluentinc/libserdes)
- [libmagic](https://www.darwinsys.com/file/)

```bash
$ sudo yum install libcurl-devel openssl-devel libgcrypt-devel bzip2-devel libzstd-devel boost-devel boost-regex boost-iostreams boost-system boost-thread boost-filesystem sqlite-devel libssh2-devel libconfig-devel libblkid-devel librdkafka-devel krb5-devel krb5-libs krb5-workstation libntlm-devel gssntlmssp-devel jansson-devel librdkafka-devel confluent-libserdes-devel avro-c-devel avro-cpp-devel file-devel
```

#### ✔️ oracle

Oralce provide OCI library to interface with oracle database engine. OCI is needed to read/write to database and DCN. Download RPMs instal client from [Oralce](https://www.oracle.com/database/technologies/instant-client/linux-x86-64-downloads.html) website. you will need the following

- Basic Package (must)
- SDK Package (must)
- JDBC Suppliment Package (optional)
- ODBC Package (optional)

for example if you want to download the v19.19 then run

```bash
$ sudo yum install https://download.oracle.com/otn_software/linux/instantclient/1919000/oracle-instantclient19.19-basic-19.19.0.0.0-1.x86_64.rpm https://download.oracle.com/otn_software/linux/instantclient/1919000/oracle-instantclient19.19-devel-19.19.0.0.0-1.x86_64.rpm
```

update the environment variable

```bash
cat << EOF >> ~/.bashrc
export ORACLE_BASE=/usr/lib/oracle
export ORACLE_HOME=/usr/lib/oracle/19.19/client64
EOF
```

#### ✔️ aws-c-sdk

if you face any issue compiling then, download a known [working release](https://github.com/aws/aws-sdk-cpp/archive/refs/tags/1.10.57.tar.gz)

```bash
git clone https://github.com/aws/aws-sdk-cpp
cd aws-sdk-cpp
./prefetch_crt_dependency.sh
mkdir build && cd build
cmake3 -DCMAKE_INSTALL_PREFIX:PATH=/usr -DBUILD_SHARED_LIBS=ON -DBUILD_ONLY="s3;dynamodb;kafkaconnect;kafka;kinesis;sqs;clouddirectory" -DENABLE_TESTING=OFF -Wno-dev ..
sudo make install
```

#### ✔️ libsmb2

[libsmb2](https://www.snia.org/sites/default/files/SDC/2019/presentations/SMB/Sahlberg_Ronnie_Libsmb2_a_Userspace_SMB2_Client_for_all_Platforms.pdf) is a portable small footprint SMB2/3 C/C++ interface library developed by [Ronnie Sahlberg](https://www.samba.org/~sahlberg/)

```bash
git clone https://github.com/sahlberg/libsmb2
cd libsmb2 && mkdir build && cd build
cmake3 -DCMAKE_INSTALL_PREFIX=/usr ..
sudo make install
```

#### ✔️ libhdfs3 & libgsasl
the following libraries needs to be compiled from the provided source as official source does not support kerberos connection with hdfs3 library for some reason!

✨ a locked version of [libgsasl](https://www.gnu.org/software/gsasl/) by [Brett Rosen](https://github.com/bdrosen96)

```bash
$ sudo yum install autoconf gettext-devel libtool gtk-doc gengetopt gperf texlive-epstopdf ghostscript texinfo help2man http://mirror.centos.org/centos/7/os/x86_64/Packages/gperf-3.0.4-8.el7.x86_64.rpm
```

```bash
git clone https://github.com/bdrosen96/libgsasl
cd libgsasl
sed -i '31i AM_PROG_AR' configure.ac
sed -i '31i AM_PROG_AR' lib/configure.ac
make
find . -type f -exec sed -i -e 's/AM_PROG_MKDIR_P/AC_PROG_MKDIR_P/g' {} \;
autoreconf -iv
LDFLAGS="-lssl -lcrypto" ./configure --prefix=/usr  --with-gssapi-impl=mit
sudo make install
```

✨ a customized vestion of [libhdfs3](https://issues.apache.org/jira/browse/HDFS-6994) by [Brett Rosen](https://github.com/bdrosen96) which by originaly developed by Pivotal HD currently maintained by [Apache HAWQ](https://hawq.apache.org/)

```bash
$ sudo yum install libxml2 libxml2-devel protobuf protobuf-devel
```

```bash
git clone https://github.com/bdrosen96/libhdfs3
cd libhdfs3
sed -i 's/dumpversion/dumpfullversion/g' CMake/Platform.cmake # if gcc version >= 7
mkdir build && cd build
cmake3 -Wno-dev -DCMAKE_INSTALL_PREFIX=/usr ..
sudo make install
```

## Compiling the library

to compile and install a Release version of axon, with the `CMAKE_BUILD_TYPE=Release` a debug version will be built by default.


```bash
scl enable devtoolset-12 bash
export LD_LIBRARY_PATH=/usr/lib:/usr/lib64:/usr/local/lib64:$ORACLE_HOME/lib:$LD_LIBRARY_PATH

git clone https://github.com/thegeekyboy/axon
cd axon && mkdir build && cd build
cmake3 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr ..
sudo make install
```

