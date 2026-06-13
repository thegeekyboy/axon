# axon

A C++17 middleware connectivity library providing a unified interface for file transfer, streaming, database, and caching systems. axon is the foundational dependency for [hyperion](https://github.com/thegeekyboy/hyperion).

---

## Overview

axon abstracts the complexity of connecting to heterogeneous infrastructure behind a consistent API. Whether moving files between SFTP and S3, consuming Kafka or Kinesis streams, or querying Oracle and ScyllaDB — the calling code follows the same pattern. All connectors share a common base class, support operator-based stream piping (`>>`), and are thread-safe.

---

## Features

### File / Object Transfer
| Connector | Class | Protocol |
|---|---|---|
| Local filesystem | `axon::transfer::file` | POSIX |
| SFTP / SCP | `axon::transfer::sftp` | SSH2 via libssh2 |
| FTP | `axon::transfer::ftp` | RFC 959 |
| Amazon S3 / MinIO | `axon::transfer::s3` | AWS SDK |
| SMB / CIFS | `axon::transfer::samba` | SMB2 via libsmb2 |
| HDFS | `axon::transfer::hdfs` | libhdfs3 |
| Null sink | `axon::transfer::nothing` | — |

Every transfer connector implements the same interface: `connect`, `disconnect`, `chwd`, `pwd`, `mkdir`, `list`, `get`, `put`, `copy`, `ren`, `del`, `open`, `close`, `read`, `write`. Connectors can be piped directly:

```cpp
src.open("input.csv", std::ios::in);
dst.open("output.csv", std::ios::out);
src >> dst;   // or: dst << src
src.close();
dst.close();
```

### Event Streaming
| Connector | Class |
|---|---|
| Apache Kafka (Avro / Schema Registry) | `axon::stream::kafka` |
| AWS Kinesis | `axon::stream::kinesis` |
| Oracle CQN (Change Query Notification) | `axon::stream::cqn` |

Streams support both callback and polling consumption models.

### Databases
| Connector | Class |
|---|---|
| Oracle DB (OCI) | `axon::database::oracle` |
| ScyllaDB / Cassandra | `axon::database::scylladb` |
| SQLite | `axon::database::sqlite` |

### Caching & Messaging
| Connector | Class |
|---|---|
| Redis (pipelining, pub/sub) | `axon::cache::redis` |
| RabbitMQ | `axon::rabbit` |
| POSIX message queues | `axon::message` |

### Authentication
| Connector | Class |
|---|---|
| Kerberos (keytab / ccache) | `axon::authentication::kerberos` |
| LDAP | `axon::ldap` |

### Utilities
- `axon::log` — thread-safe structured logger with `<<` chaining and file rotation
- `axon::timer` — high-resolution timing, ISO8601 timestamps, epoch helpers
- `axon::util::magic` — MIME type detection (libmagic singleton)
- `axon::util::validator` — hostname and username regex validation
- `axon::util::uuid()` — thread-safe UUID v4 generation
- `axon::aes` — AES-128 ECB/CBC via OpenSSL EVP (thread-safe)
- `axon::dmi` — DMI/SMBIOS hardware identity reader
- `axon::mml` — MML (Man-Machine Language) session handler

---

## Requirements

### Compiler & Standard
- GCC 11+ (C++17 required)
- On RHEL/CentOS 7: enable via `devtoolset-11` or later

### Build System
- CMake ≥ 3.26

### Dependencies

All dependencies are available via the system package manager on RHEL/CentOS 8–9 or from the Confluent RPM repository.

| Library | Purpose |
|---|---|
| OpenSSL (`libssl`, `libcrypto`) | AES encryption, TLS |
| libssh2 | SFTP / SCP |
| libsmb2 | SMB2 / CIFS |
| libhdfs3 | HDFS |
| librdkafka | Kafka consumer |
| libserdes | Kafka Avro deserialisation |
| AWS C++ SDK (`s3`, `kinesis`, `sqs`, `dynamodb`) | AWS connectors |
| Boost (`regex`, `iostreams`, `filesystem`, `thread`) | Regex, compression |
| libconfig++ | Configuration parsing |
| jansson | JSON |
| hiredis | Redis |
| librabbitmq | RabbitMQ |
| Oracle OCI | Oracle DB |
| libcassandra | ScyllaDB / Cassandra |
| SQLite3 | SQLite |
| libmagic | MIME type detection |
| OpenLDAP | LDAP |
| MIT Kerberos (`krb5`) | Kerberos authentication |
| libgcrypt | Cryptographic primitives |
| bzip2, zstd | Compression |
| libcurl | HTTP |

> **Note:** `libhdfs3` and `libgsasl` must be compiled from the project's custom forks due to upstream Kerberos compatibility issues. See [INSTALL.md](INSTALL.md) for details.

---

## Building

axon enforces out-of-source builds.

```bash
git clone https://github.com/thegeekyboy/axon.git
cd axon
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Build as a submodule

When included as a CMake submodule in another project, axon automatically builds as a **static library** instead of a shared one:

```cmake
add_subdirectory(axon)
target_link_libraries(my_target PRIVATE axon)
```

### Install (standalone)

```bash
sudo make install
```

Installs the shared library to `${CMAKE_INSTALL_LIBDIR}`, headers to `${CMAKE_INSTALL_INCLUDEDIR}`, and a `pkg-config` file to `${CMAKE_INSTALL_DATADIR}/pkgconfig`.

### Debug builds

Pass `-DDEBUG=N` to enable progressively more verbose output:

| Level | Macro enabled |
|---|---|
| `DEBUG=0` (default) | none |
| `DEBUG=1` | `DBGPRN` — debug prints |
| `DEBUG=2` | `WRNPRN` — warnings |
| `DEBUG=3` | `INFPRN` + `BENCHMARK` — function timing |
| `DEBUG=4` | AWS SDK trace logging |

```bash
cmake -DDEBUG=3 ..
```

---

## Usage

All credentials and hostnames are passed via environment variables in the examples below, which is the recommended pattern for production use.

### File transfer

```cpp
#include <axon.h>
#include <axon/connection.h>
#include <axon/ssh.h>
#include <axon/s3.h>

// SFTP → S3 streaming transfer
axon::transfer::sftp src("sftp.example.com", "user", "pass", 22);
axon::transfer::s3   dst("s3.ap-southeast-1.amazonaws.com", "ACCESS_KEY", "SECRET_KEY");

src.connect();
dst.connect();
dst.chwd("/my-bucket/uploads/");

src.open("/data/report.csv", std::ios::in);
dst.open("report.csv",       std::ios::out);
src >> dst;
src.close();
dst.close();

src.disconnect();
dst.disconnect();
```

### Listing files with a filter

```cpp
axon::transfer::sftp conn("sftp.example.com", "user", "pass");
conn.connect();
conn.chwd("/data/incoming");
conn.filter(".*\\.csv");   // regex filter

std::vector<axon::entry> files;
conn.list(files);

for (auto &f : files)
	std::cout << f.name << "  " << f.size << " bytes\n";

conn.disconnect();
```

### Kafka consumer (Avro / Schema Registry)

```cpp
#include <axon/kafka.h>

axon::stream::kafka source("broker:9092", "https://schema-registry:8081", "my-consumer-group");
source.add("my-topic");
source.subscribe();

// Callback mode
source.start([](std::unique_ptr<axon::stream::recordset> rec) {
	std::string value;
	rec->get("FIELD_NAME", value);
	std::cout << value << "\n";
});

// Polling mode
// while (running) {
//     auto rec = source.next();
//     if (rec && !rec->is_empty()) { ... }
// }

source.stop();
```

### Kinesis consumer

```cpp
#include <axon/kinesis.h>

axon::stream::kinesis source("kinesis.ap-southeast-1.amazonaws.com", "ACCESS_KEY", "SECRET_KEY");
source.name() = "my-consumer";
source.add("my-stream", "my-stream", [](std::unique_ptr<axon::recordset> rec) {
	if (rec) std::cout << *rec << "\n";
});

source.subscribe();
source.start();

// block until signal, then:
source.stop();
```

### Oracle database

```cpp
#include <axon/oracle.h>

axon::database::oracle ora;
ora.connect("DB_SID", "username", "password");
axon::database::interface &db = ora;

db.query("SELECT EID, NAME FROM TBL_EMPLOYEE WHERE UNIT = :unit", std::string("TECHNOLOGY"));

while (db.next()) {
	int eid;
	std::string name;
	db.get(0, eid);
	db.get(1, name);
	std::cout << eid << " " << name << "\n";
}
db.done();
```

### Redis with pipelining

```cpp
#include <axon/redis.h>

axon::cache::redis redis("redis.example.com", 6379);

redis.pipeline_begin();
redis.pipeline_hincrby("entity:12345:1000:2025010815", "count", 1);
redis.pipeline_hincrby("entity:12345:1000:2025010815", "sum",   9900);
redis.pipeline_expire ("entity:12345:1000:2025010815", 262800);
auto replies = redis.pipeline_run();
```

### Kerberos authentication

```cpp
#include <axon/kerberos.h>

axon::authentication::kerberos krb("/etc/security/app.keytab",
								   "/tmp/krb5cc_app",
								   "EXAMPLE.COM",
								   "serviceaccount");

if (!krb.isCacheValid()) {
	krb.authenticate();
}

// Pass the refreshed cache to HDFS or SFTP connectors
axon::transfer::hdfs hdfs("namenode.example.com", "serviceaccount", "", 8020);
hdfs.set(AXON_TRANSFER_HDFS_AUTHTYPE,  "kerberos");
hdfs.set(AXON_TRANSFER_HDFS_CACHEPATH, "/tmp/krb5cc_app");
hdfs.connect();
```

### Logging

```cpp
#include <axon/log.h>

axon::log logger;
logger.fopen("/var/log/myapp.log");

logger << axon::log::info << "started processing " << filename << axon::log::endl;
logger << axon::log::error << "failed: " << e.what() << axon::log::endl;
```

---

## Architecture

```
axon::transfer::connection  (abstract base)
	├── file        local filesystem
	├── sftp        SSH2 (SFTP + SCP)
	├── ftp         FTP
	├── s3          AWS S3 / MinIO
	├── samba       SMB2 / CIFS
	├── hdfs        HDFS
	└── nothing     null sink

axon::stream::interface  (abstract base)
	├── kafka       Apache Kafka + Avro
	├── kinesis     AWS Kinesis
	└── cqn         Oracle Change Query Notification

axon::database::interface  (abstract base)
	├── oracle      Oracle OCI
	├── scylladb    ScyllaDB / Cassandra
	└── sqlite      SQLite3
```

All transfer connectors support the `>>` and `<<` operators for zero-copy streaming between any two connector types:

```cpp
sftp_source >> s3_destination;
s3_source   >> hdfs_destination;
local_file  >> sftp_destination;
```

---

## Environment Variables

All example programmes read credentials from environment variables to avoid hardcoding secrets:

| Variable | Purpose |
|---|---|
| `AXON_HOSTNAME` | Server hostname or IP |
| `AXON_USERNAME` | Login username |
| `AXON_PASSWORD` | Login password |
| `AXON_DOMAIN` | Windows domain (Samba / NTLM) |
| `AXON_BOOTSTRAP` | Kafka bootstrap servers |
| `AXON_SCHEMA_REGISTRY` | Confluent Schema Registry URL |
| `AXON_KAFKA_CONSUMER_GROUP` | Kafka consumer group ID |
| `AXON_KRB5_KEYTAB` | Path to Kerberos keytab file |
| `AXON_KRB5_CACHEPATH` | Path to Kerberos credential cache |
| `AXON_ORA_SID` | Oracle SID or TNS name |
| `AXON_SCYLLA_KEYSPACE` | ScyllaDB keyspace |
| `http_proxy` | HTTP proxy for S3 / AWS SDK |

---

## Error Handling

All errors are reported via `axon::exception`, which extends `std::exception` and carries the source file, line number, function name, and a human-readable message:

```cpp
try {
	conn.connect();
	conn.get("report.csv", "/tmp/report.csv");
} catch (axon::exception &e) {
	std::cerr << e.what() << "\n";   // file:line => function(): message
	std::cerr << e.msg()  << "\n";   // message only
}
```

---

## License

Apache 2.0 — see [LICENSE](LICENSE).
