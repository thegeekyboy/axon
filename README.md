# libraries needed to compile
## redhat el7
boost-devel
bzip2-devel bzip2-libs
confluent-libserdes confluent-libserdes-devel
cyrus-sasl-gssapi
jansson jansson-devel
java-1.8.0-openjdk-devel java-1.8.0-openjdk-headless
krb5-devel krb5-libs
libblkid-devel
libconfig libconfig-devel
libcurl-devel
librdkafka librdkafka-devel
libssh2-devel
libzstd-devel
openssl-devel openssl-static
sqlite-devel

## from source
libsmb2 - https://github.com/sahlberg/libsmb2/archive/refs/tags/v4.0.0.tar.gz# axon - the helper library for hexagon mediation

#### required libraries

- boost
- curl
- sqlite
- ssh2
- config++
- blkid

along with the above the "Development Tools" are required to compile axon.

- to install on Centos

```bash
sudo yum group install "Development Tools"
sudo yum install boost-devel bzip2-devel libcurl libcurl-devel sqlite-devel libssh2 libssh2-devel libconfig libconfig-devel libblkid libblkid-devel
```

#### copyright and licence

axon is provided under the Apache-2.0 license

#### contact

- Amirul <mark@binutil.net>