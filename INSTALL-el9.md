# How to install `axon`

this installation notes are tested in redhat 9.

1. [Preparing the Environment](#toc1)
	1. [Extra repos](#toc11)
	2. [Confluent](#toc12)
	3. [Build system](#toc13)
2. [Installing the dependencies](#toc2)
	1. [from repo](#toc21)
	2. [oracle](#toc22)
	3. [aws-c-sdk](#toc23)
	3. [libsmb2](#toc24)
	3. [libhdfs3 & libgsasl](#toc25)
1. [Compiling the library](#toc3)


## Preparing the Environment <a name="toc1"></a>

### ⭐Extra repos <a name="toc11"></a>

Some non-standard repos are needed to get the build system ready

```bash
$ sudo yum install epel-release dnf-plugins-core wget nano
```

### ⭐Confluent <a name="toc12"></a>

[confluent](https://www.confluent.io/) manages a repo of rpm packages to support their products. conveniently they have made it public domain and we can use them to avoid compiling from source.

detail guide [here](https://docs.confluent.io/platform/current/installation/overview.html)

```bash
$ sudo rpm --import https://packages.confluent.io/rpm/7.8/archive.key

$ sudo cat << EOF >> /etc/yum.repos.d/confluent.repo
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
⚠️ make sure when pasting the $releasever and $basearch is pasted correctly

### ⭐Build system <a name="toc13"></a>

Standard C++17 is need to compile from source

```bash
$ sudo yum install automake bison boost-devel cmake3 gcc-c++ gettext gettext-devel git gperf libtool readline readline-devel rpmdevtools rpmlint
```

## Installing the dependencies <a name="toc2"></a>

#### ✔️ from repo <a name="toc21"></a>
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
- [openldap](https://www.openldap.org/)
- [scylladb-cpp-driver](https://github.com/scylladb/cpp-driver)

```bash
$ sudo yum install bzip2 bzip2-devel bzip2-libs cpp-httplib-devel file file-devel jansson-devel krb5-devel krb5-libs libblkid-devel libconfig-devel libcurl-devel libgcrypt-devel libntlm-devel librdkafka1 librdkafka-devel libssh2-devel libuuid libuuid-devel libuv-devel libxml2-devel libzstd-devel nano openssl-devel protobuf protobuf-c protobuf-c-devel protobuf-devel sqlite-devel xz-devel openldap-devel
```

##### avro-c
⚠️ if _avro-c_ is missing or is not installed from above package. then install as following. if you want to compile from SNAPSHOT then clone https://github.com/apache/avro/

```bash
sudo yum install jansson-devel
wget https://github.com/apache/avro/archive/refs/tags/release-1.11.4.tar.gz -O avro-release-1.11.4.tar.gz
tar xf avro-1.11.4.tar.gz && cd avro-release-1.11.4/lang/c && mkdir build && cd build
cmake3 .. -DCMAKE_INSTALL_PREFIX=/usr
sudo make install
```

##### libserdes 
⚠️ _often it was noticed that when `confluent-libserdes-devel` package installs header files, the folder permission is incorrect. best check the permission after installing the package._

to install libserdes from source

```bash
sudo yum install libcurl-devel
wget https://github.com/confluentinc/libserdes/archive/refs/tags/v7.8.1.tar.gz -O confluent-libserdes-7.8.1.tar.gz
tar xf confluent-libserdes-7.8.1.tar.gz && cd libserdes-7.8.1
./configure --prefix=/usr
sudo make && make install
```


##### libsmb2 <a name="toc24"></a>

[libsmb2](https://www.snia.org/sites/default/files/SDC/2019/presentations/SMB/Sahlberg_Ronnie_Libsmb2_a_Userspace_SMB2_Client_for_all_Platforms.pdf) is a portable small footprint SMB2/3 C/C++ interface library developed by [Ronnie Sahlberg](https://www.samba.org/~sahlberg/)

```bash
#git clone https://github.com/sahlberg/libsmb2
wget https://github.com/sahlberg/libsmb2/archive/refs/tags/v4.0.0.tar.gz
tar xf v4.0.0.tar.gz
cd libsmb2-4.0.0 && mkdir build && cd build
cmake3 .. -DCMAKE_INSTALL_PREFIX=/usr
sudo make install
```

#### ✔️ scylladb's cpp-driver
A modern, feature-rich and shard-aware C/C++ client library for ScyllaDB using exclusively Cassandra's binary protocol and Cassandra Query Language v3. Forked from Datastax cpp-driver.

```bash
sudo yum install libuv-devel openssl-devel zlib-devel
wget https://github.com/scylladb/cpp-driver/archive/refs/tags/2.16.2b.tar.gz -O scylla-cpp-driver-2.16.2b.tar.gz
tar xf scylla-cpp-driver-2.16.2b.tar.gz && cd cpp-driver-2.16.2b/ && mkdir build && cd build 
cmake3 .. -DCMAKE_INSTALL_PREFIX=/usr
sudo make install
```

#### ✔️ aws-c-sdk <a name="toc23"></a>

if you face any issue compiling then, download a known [working release](https://github.com/aws/aws-sdk-cpp/archive/refs/tags/1.11.178.tar.gz)

```bash
# git clone https://github.com/aws/aws-sdk-cpp
# cd aws-sdk-cpp
wget https://github.com/aws/aws-sdk-cpp/archive/refs/tags/1.11.345.tar.gz -O aws-sdk-cpp-1.11.345.tar.gz
tar xf aws-sdk-cpp-1.11.345.tar.gz
cd aws-sdk-cpp-1.11.345 && ./prefetch_crt_dependency.sh
mkdir build && cd build
cmake3 .. -DCMAKE_INSTALL_PREFIX:PATH=/usr -DBUILD_SHARED_LIBS=ON -DBUILD_ONLY="core;s3;dynamodb;kafkaconnect;kafka;kinesis;sqs;clouddirectory" -DENABLE_TESTING=OFF -Wno-dev
sudo make install
```

#### ✔️ oracle <a name="toc22"></a>

Oralce provide OCI library to interface with oracle database engine. OCI is needed to read/write to database and DCN. Download RPMs instal client from [Oralce](https://www.oracle.com/database/technologies/instant-client/linux-x86-64-downloads.html) website. you will need the following

- Basic Package (must)
- SDK Package (must)
- JDBC Suppliment Package (optional)
- ODBC Package (optional)

for example if you want to download the v19.19 then run

```bash
$ sudo yum install https://download.oracle.com/otn_software/linux/instantclient/1919000/oracle-instantclient19.19-basic-19.19.0.0.0-1.x86_64.rpm https://download.oracle.com/otn_software/linux/instantclient/1919000/oracle-instantclient19.19-devel-19.19.0.0.0-1.x86_64.rpm
```

```bash
sudo yum install https://download.oracle.com/otn_software/linux/instantclient/2360000/oracle-instantclient-basic-23.6.0.24.10-1.el9.x86_64.rpm
sudo yum install https://download.oracle.com/otn_software/linux/instantclient/2360000/oracle-instantclient-devel-23.6.0.24.10-1.el9.x86_64.rpm
```

update the environment variable

```bash
cat << EOF >> ~/.bashrc
export ORACLE_BASE=/usr/lib/oracle
export ORACLE_HOME=/usr/lib/oracle/23/client64
EOF
```

#### ✔️ libhdfs3 & libgsasl <a name="toc25"></a>
the following libraries needs to be compiled from the provided source as official source does not support kerberos connection with hdfs3 library for some reason!

✨ a locked version of [libgsasl](https://www.gnu.org/software/gsasl/) by [Brett Rosen](https://github.com/bdrosen96)
```bash
$ sudo yum install krb5-devel krb5-libs libuuid libuuid-devel libntlm-devel openssl-devel
```

⚠️ due to new changes in the ssl apis the original libgsasl from Brett Rosen requires much changes to compile. fortunately [nokia](https://github.com/nokia/) has made the necessary changes which can be download from [here](https://github.com/nokia/libgsasl/releases/download/v1.8.2/libgsasl-1.8.2.tar.gz)

```bash
wget https://github.com/nokia/libgsasl/releases/download/v1.8.2/libgsasl-1.8.2.tar.gz
tar xf libgsasl-1.8.2.tar.gz && cd libgsasl-1.8.2
LDFLAGS="-lssl -lcrypto"  CFLAGS="-g -O2 -fPIC" ./configure --prefix=/usr  --with-gssapi-impl=mit
make
sudo make install
```

✨ a customized vestion of [libhdfs3](https://issues.apache.org/jira/browse/HDFS-6994) by [Brett Rosen](https://github.com/bdrosen96) which by originaly developed by Pivotal HD currently maintained by [Apache HAWQ](https://hawq.apache.org/)

```bash
$ sudo yum install libxml2 libxml2-devel protobuf protobuf-devel boost-devel libcurl-devel libgsasl
```

```bash
git clone https://github.com/thegeekyboy/libhdfs3
cd libhdfs3
sed -i 's/dumpversion/dumpfullversion/g' CMake/Platform.cmake # if gcc version >= 7
mkdir build && cd build
cmake3 .. -Wno-dev -DCMAKE_INSTALL_PREFIX=/usr
sudo make install
```

## Compiling the library <a name="toc3"></a>

to compile and install a Release version of axon, with the `CMAKE_BUILD_TYPE=Release`. A debug version will be built by default.


```bash
export LD_LIBRARY_PATH=/usr/lib:/usr/lib64:/usr/local/lib64:$ORACLE_HOME/lib:$LD_LIBRARY_PATH

git clone https://github.com/thegeekyboy/axon
cd axon && mkdir build && cd build
cmake3 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr ..
sudo make install
```
