# axon

A C++17 middleware connectivity library providing a unified interface for file transfer, event streaming, database, and caching systems. axon is the foundational dependency for [hyperion](https://github.com/thegeekyboy/hyperion).

---

## Overview

axon abstracts the complexity of connecting to heterogeneous infrastructure behind a consistent API. Whether moving files between SFTP and S3, consuming Kafka or Kinesis streams with Avro deserialization, querying Oracle, ScyllaDB or SQLite — the calling code follows the same pattern.

All database connectors share `axon::database::connector` as a common base with a unified `resultset` cursor. All stream connectors share `axon::stream::connector` with a callback-based delivery model. Transfer connectors support the `>>` operator for zero-copy streaming between any two endpoints.

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
| Connector | Class | Backend |
|---|---|---|
| Apache Kafka (Avro / Schema Registry) | `axon::stream::kafka` | librdkafka + libserdes |
| AWS Kinesis (standard polling) | `axon::stream::kinesis` | AWS C++ SDK |
| Oracle OCN (Change Notification) | `axon::stream::ocn` | Oracle OCI |

All stream connectors derive from `axon::stream::connector` and deliver records via `axon::resultset` in a callback:

```cpp
source.add("my-topic", "my-topic", [](std::unique_ptr<axon::resultset> rs) {
    while (rs->next()) {
        std::string val;
        rs->get("FIELD", val);
    }
});
source.subscribe();
source.start();
```

### Databases
| Connector | Class | Backend |
|---|---|---|
| Oracle DB (OCI) | `axon::database::oracle` | Oracle OCI (RAII via `oci::connection`) |
| ScyllaDB / Cassandra | `axon::database::scylla` | DataStax C++ driver |
| SQLite | `axon::database::sqlite` | SQLite3 |

All database connectors derive from `axon::database::connector` and return results via `axon::resultset`:

```cpp
axon::database::connector &db = ora;   // or scylla, sqlite

db.query("SELECT ID, NAME FROM EMPLOYEES WHERE DEPT = :dept", std::string("TECH"));

axon::resultset rs(db);
while (rs.next()) {
    long id;
    std::string name;
    rs.get(0, id);
    rs.get("NAME", name);
}
rs.done();
```

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
- `axon::util::validator` — hostname, username, and TNS name regex validation
- `axon::util::uuid()` — thread-safe UUID v4 generation
- `axon::aes` — AES-128 ECB/CBC via OpenSSL EVP
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

| Library | Purpose |
|---|---|
| OpenSSL (`libssl`, `libcrypto`) | AES encryption, TLS |
| libssh2 | SFTP / SCP |
| libsmb2 | SMB2 / CIFS |
| libhdfs3 | HDFS |
| librdkafka | Kafka consumer |
| libserdes | Kafka Avro deserialisation |
| AWS C++ SDK (`kinesis`, `s3`, `core`) | AWS connectors |
| Boost (`regex`, `json`, `iostreams`, `filesystem`) | Regex, JSON, compression |
| libconfig++ | Configuration parsing |
| jansson | JSON |
| hiredis | Redis |
| librabbitmq | RabbitMQ |
| Oracle OCI (`libclntsh`) | Oracle DB and OCN |
| DataStax C++ driver (`libcassandra`) | ScyllaDB / Cassandra |
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

When included as a CMake submodule, axon builds as a **static library** automatically:

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

| Level | Effect |
|---|---|
| `DEBUG=0` (default) | no debug output |
| `DEBUG=1` | `DBGPRN` — debug prints |
| `DEBUG=2` | `WRNPRN` — warnings |
| `DEBUG=3` | `INFPRN` + `BENCHMARK` — function timing |
| `DEBUG=4` | AWS SDK trace logging |

```bash
cmake -DDEBUG=3 ..
```

---

## Usage

### File transfer — SFTP to S3

```cpp
#include <axon.h>
#include <axon/ssh.h>
#include <axon/s3.h>

axon::transfer::sftp src("sftp.example.com", "user", "pass", 22);
axon::transfer::s3   dst("s3.ap-southeast-1.amazonaws.com", "ACCESS_KEY", "SECRET");

src.connect();
dst.connect();
dst.chwd("/my-bucket/uploads/");

src.open("/data/report.csv", std::ios::in);
dst.open("report.csv",       std::ios::out);
src >> dst;
src.close();
dst.close();
```

### File listing with filter

```cpp
axon::transfer::sftp conn("sftp.example.com", "user", "pass");
conn.connect();
conn.chwd("/data/incoming");
conn.filter(".*\\.csv");   // regex

std::vector<axon::entry> files;
conn.list(files);
for (auto &f : files)
    std::cout << f.name << "  " << f.size << " bytes\n";
```

### Oracle database with resultset

```cpp
#include <axon/oracle.h>

axon::database::oracle ora;
ora.connect("DB_SID", "username", "password");
axon::database::connector &db = ora;

db.query("SELECT EID, NAME FROM TBL_EMPLOYEE WHERE UNIT = :unit",
         std::string("TECHNOLOGY"));

axon::resultset rs(db);
while (rs.next()) {
    long eid; std::string name;
    rs.get(0, eid);
    rs.get("NAME", name);
    std::cout << eid << " " << name << "\n";
}
rs.done();
```

### Oracle — named parameter binding via operator<<

```cpp
long dept_id = 10;
std::string status = "ACTIVE";

db << dept_id << status;
db.query("SELECT * FROM EMPLOYEES WHERE DEPT_ID = :d AND STATUS = :s");

axon::resultset rs(db);
while (rs.next()) { /* ... */ }
rs.done();
```

### Oracle — connect via hostname:port/service (no tnsnames.ora)

```cpp
#include <axon/oci.h>
#include <axon/oracle.h>

auto conn = std::make_shared<axon::database::oci::connection>();
conn->connect("myhost", "username", "password", 1521, "myservice");

axon::database::oracle ora(conn);
```

### ScyllaDB / Cassandra

```cpp
#include <axon/scylla.h>

axon::database::scylla db;
db[AXON_DATABASE_HOSTNAME] = "cass-node1";
db[AXON_DATABASE_USERNAME] = "cassandra";
db[AXON_DATABASE_PASSWORD] = "cassandra";
db[AXON_DATABASE_KEYSPACE] = "my_keyspace";
db.connect();

db.execute("INSERT INTO orders (id, status) VALUES (?, ?)",
           (int64_t) 1001, std::string("NEW"));

db.query("SELECT id, status, amount FROM orders WHERE id = ?", (int64_t) 1001);
axon::resultset rs(db);
while (rs.next()) {
    int64_t id; std::string status; double amount;
    rs.get("id",     id);
    rs.get("status", status);
    rs.get("amount", amount);
}
rs.done();
db.close();
```

### SQLite

```cpp
#include <axon/sqlite.h>

axon::database::sqlite db;
db[AXON_DATABASE_FILEPATH] = "/tmp/myapp.db";
db.connect();

db.execute("CREATE TABLE IF NOT EXISTS logs "
           "(id INTEGER PRIMARY KEY, msg TEXT, ts INTEGER)");

db.execute("INSERT INTO logs (msg, ts) VALUES (?, ?)",
           std::string("started"), (int64_t) time(nullptr));

db.query("SELECT id, msg, ts FROM logs ORDER BY id DESC");
axon::resultset rs(db);
while (rs.next()) {
    long id, ts; std::string msg;
    rs.get(0, id);
    rs.get(1, msg);
    rs.get(2, ts);
    std::cout << id << ": " << msg << "\n";
}
rs.done();
db.close();
```

### Kafka consumer (Avro / Schema Registry)

```cpp
#include <axon/kafka.h>

axon::stream::kafka source("broker:9092",
                           "http://schema-registry:8081",
                           "my-consumer-group");

source.add("my-topic", "my-topic",
    [](std::unique_ptr<axon::resultset> rs) {
        while (rs->next()) {
            std::string order_id, status;
            rs->get("ORDERID",      order_id);
            rs->get("TRANS_STATUS", status);
            std::cout << order_id << " => " << status << "\n";
        }
    });

source.subscribe();
source.start();

// Block until signal, then:
source.stop();
```

### Kafka — delete consumer group

```cpp
// Single group
axon::stream::kafka::del("broker:9092", "stale-consumer-group");

// Multiple groups at once
axon::stream::kafka::del("broker:9092",
    std::vector<std::string>{"group-a", "group-b"});
```

### AWS Kinesis consumer

```cpp
#include <axon/kinesis.h>

axon::stream::kinesis source("ap-southeast-1", "ACCESS_KEY", "SECRET_KEY");

source.add("my-stream", "my-stream",
    [](std::unique_ptr<axon::resultset> rs) {
        while (rs->next()) {
            std::vector<uint8_t> data;
            std::string seq, pkey;
            int64_t ts = 0;
            rs->get("data",            data);
            rs->get("sequence_number", seq);
            rs->get("partition_key",   pkey);
            rs->get("timestamp",       ts);
            std::cout << seq << " | "
                      << std::string(data.begin(), data.end()) << "\n";
        }
    });

source.subscribe();
source.start();
source.stop();
```

### Oracle OCN — Change Notification

```cpp
#include <axon/ocn.h>
#include <axon/oci.h>

// Option A — credential constructor (OS-assigned callback port)
axon::stream::ocn source("MYDB", "user", "pass");
source.connect();

// Option B — pre-built connection with fixed callback port
auto conn = std::make_shared<axon::database::oci::connection>(6667);
conn->connect("MYDB", "user", "pass");
axon::stream::ocn source(conn);
// source.connect() is a no-op when a pre-connected handle is passed

source.add("ORDERS",
    "SELECT * FROM MY_SCHEMA.ORDERS",
    [](std::unique_ptr<axon::resultset> rs) {
        while (rs->next()) {
            std::string order_id, status;
            rs->get("ORDER_ID", order_id);
            rs->get("STATUS",   status);
            std::cout << "changed: " << order_id << " => " << status << "\n";
        }
    });

source.subscribe();
source.start();
source.stop();
```

### resultset API

`axon::resultset` is the unified lazy cursor for all database and stream connectors.

```cpp
axon::resultset rs(db);          // pull from database connector
axon::resultset rs(db, 512);     // with explicit batch size

rs.next();                        // advance — returns false at EOF
rs.done();                        // release result set and free resources

// Schema inspection
rs.count();                       // number of columns
rs.name(i);                       // column name (string_view)
rs.type(i);                       // axon::column_type enum value
rs.rows();                        // rows consumed so far

// Get by position
long id;             rs.get(0, id);
std::string name;    rs.get(1, name);
double score;        rs.get(2, score);
std::vector<uint8_t> blob; rs.get(3, blob);

// Get by name — throws if column not found
rs.get("ORDER_ID", order_id);

// Returns false (not throws) if field is NULL — output is unchanged
bool has_value = rs.get("OPTIONAL_FIELD", val);

// Stream extraction — columns in schema order
rs >> id >> name >> score;

// Serialisation
std::string json = rs.to_json();
std::cout << rs;                  // prints current row
```

### Redis with pipelining

```cpp
#include <axon/redis.h>

axon::cache::redis redis("redis.example.com", 6379);

redis.pipeline_begin();
redis.pipeline_hincrby("stats:2026010815", "count",  1);
redis.pipeline_hincrby("stats:2026010815", "amount", 9900);
redis.pipeline_expire ("stats:2026010815", 86400);
auto replies = redis.pipeline_run();
```

### Kerberos authentication

```cpp
#include <axon/kerberos.h>

axon::authentication::kerberos krb("/etc/security/app.keytab",
                                   "/tmp/krb5cc_app",
                                   "EXAMPLE.COM",
                                   "serviceaccount");
if (!krb.isCacheValid())
    krb.authenticate();

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
logger << axon::log::info  << "started: " << filename << axon::log::endl;
logger << axon::log::error << "failed: "  << e.what() << axon::log::endl;
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

axon::stream::connector  (abstract base)
    ├── kafka       Apache Kafka + Avro / Schema Registry
    ├── kinesis     AWS Kinesis (standard polling)
    └── ocn         Oracle OCN (Change Notification)

axon::database::connector  (abstract base)
    ├── oracle      Oracle OCI
    ├── scylla      ScyllaDB / Cassandra
    └── sqlite      SQLite3

axon::resultset             unified lazy cursor (database + stream)

axon::database::oci         Oracle OCI RAII wrappers
    ├── environment     process-wide singleton (ref-counted)
    ├── error           OCIError handle
    ├── context         OCISvcCtx handle
    ├── server          OCIServer handle
    ├── session         OCISession handle
    ├── connection      clubs context + server + session
    └── statement       OCIStmt with bind support
```

Transfer connectors support `>>` and `<<` for zero-copy streaming:

```cpp
sftp_source  >> s3_destination;
s3_source    >> hdfs_destination;
local_file   >> sftp_destination;
```

---

## Environment Variables

All example programs read credentials from environment variables:

| Variable | Purpose |
|---|---|
| `AXON_HOSTNAME` | Server hostname or IP |
| `AXON_USERNAME` | Login username |
| `AXON_PASSWORD` | Login password |
| `AXON_DOMAIN` | Windows domain (Samba / NTLM) |
| `AXON_SID` | Oracle TNS name or Easy Connect string |
| `AXON_REGION` | AWS region (Kinesis, S3) |
| `AXON_ACCESS_KEY` | AWS access key id |
| `AXON_SECRET_KEY` | AWS secret access key |
| `AXON_BOOTSTRAP` | Kafka bootstrap servers |
| `AXON_SCHEMA_REGISTRY` | Confluent Schema Registry URL |
| `AXON_CONSUMER_GROUP` | Kafka consumer group id |
| `AXON_KEYSPACE` | ScyllaDB keyspace |
| `AXON_KRB5_KEYTAB` | Path to Kerberos keytab file |
| `AXON_KRB5_CACHEPATH` | Path to Kerberos credential cache |
| `http_proxy` | HTTP proxy for S3 / AWS SDK |

---

## Error Handling

All errors are reported via `axon::exception`, which carries the source file, line number, function name, and a human-readable message:

```cpp
try {
    conn.connect();
    conn.get("report.csv", "/tmp/report.csv");
} catch (axon::exception &e) {
    std::cerr << e.what() << "\n";   // file:line => function(): message
}
```

---

## License

Apache 2.0 — see [LICENSE](LICENSE).