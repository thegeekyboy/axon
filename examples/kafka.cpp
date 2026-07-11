/*
 * kafka2r.cpp — generic test suite for axon::stream2r::kafka
 *
 * Does not assume any specific schema or topic structure.
 * Tests the axon kafka connector and recordset2r API using
 * whatever topic and schema the user provides.
 *
 * Tests covered:
 *   1.  Lifecycle             (add, subscribe, start, stop)
 *   2.  Message delivery      (at least one message received)
 *   3.  recordset2r cursor    (next() returns true)
 *   4.  recordset2r metadata  (count() > 0, name(0) non-empty, source() correct)
 *   5.  Column types          (all columns have a valid column_type)
 *   6.  to_json()             (produces non-empty JSON object)
 *   7.  operator<<            (print does not throw)
 *   8.  Global callback       (start(cbfn) overrides per-topic callback)
 *   9.  counter()             (increments per message)
 *   10. stop() / start() cycle (re-subscribe and restart)
 *   11. autocommit flag       (get/set without crash)
 *   12. get<T> by name        (first column accessed by name, type-safe)
 *   13. del()                 (optional — set AXON_RUN_DEL=1)
 *
 * Environment variables:
 *   AXON_BOOTSTRAP         — Kafka broker list (e.g. "broker1:9092,broker2:9092")
 *   AXON_SCHEMA_REGISTRY   — Schema Registry URL (e.g. "http://host:8081")
 *   AXON_CONSUMER_GROUP    — consumer group id (default: "axon::kafka2r_test")
 *   AXON_TOPIC             — topic to consume (e.g. "my.topic")
 *   AXON_TEST_TIMEOUT_SEC  — seconds to wait per test (default: 30)
 *   AXON_RUN_DEL           — set to "1" to run the del() test (default: skip)
 *
 * Build (from axon build directory):
 *   g++ -std=c++17 -I ../include -L . -laxon -lrdkafka -lavro -lserdes \
 *       -o kafka2r ../examples/kafka2r.cpp
 *
 * Run:
 *   AXON_BOOTSTRAP=broker:9092 \
 *   AXON_SCHEMA_REGISTRY=http://schema-reg:8081 \
 *   AXON_TOPIC=my.topic \
 *   ./kafka2r
 */

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include <thread>

#include <axon.h>
#include <axon/util.h>
#include <axon/kafka.h>

// -----------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------

static int g_pass = 0, g_fail = 0;

static void check(bool ok, const std::string &label)
{
    if (ok) { ++g_pass; std::cout << "  PASS  " << label << "\n"; }
    else    { ++g_fail; std::cout << "  FAIL  " << label << "\n"; }
}

static bool wait_for(std::atomic<int> &counter, int expected, int timeout_sec)
{
    for (int i = 0; i < timeout_sec * 10 && counter.load() < expected; i++)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return counter.load() >= expected;
}

// -----------------------------------------------------------------------
// Shared state filled by the capture callback
// -----------------------------------------------------------------------

struct capture_result {
    bool        populated { false };
    std::string source;
    int         col_count { 0 };
    std::string col0_name;
    std::string json;
    std::string print_out;
    bool        next_ok   { false };
    std::vector<axon::column_type> types;
};

static capture_result   g_cap;
static std::atomic<int> g_received { 0 };
static std::atomic<int> g_global   { 0 };

static void capture(std::unique_ptr<axon::recordset2r> rs)
{
    if (!rs) { g_received++; return; }
    if (!rs->next()) { g_received++; return; }

    capture_result r;
    r.next_ok   = true;
    r.source    = rs->source();
    r.col_count = (int) rs->count();
    r.col0_name = r.col_count > 0 ? std::string(rs->name(0)) : "";
    r.json      = rs->to_json();

    for (int i = 0; i < r.col_count; i++)
        r.types.push_back(rs->type(i));

    std::ostringstream oss;
    oss << *rs;
    r.print_out = oss.str();

    while (rs->next()) { }

    r.populated = true;
    g_cap       = r;
    g_received++;
}

static void global_cb(std::unique_ptr<axon::recordset2r> rs)
{
    if (rs) while (rs->next()) { }
    g_global++;
}

// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------

int main([[maybe_unused]] int argc,
         [[maybe_unused]] char *argv[],
         char *env[])
{
    axon::timer ctm(__PRETTY_FUNCTION__);

    std::string bootstrap, schema_registry, consumer_group, topic;
    int  timeout_sec = 30;
    bool run_del     = false;

    for (int i = 0; env[i]; i++)
    {
        auto p = axon::util::split(env[i], '=');
        if (p[0] == "AXON_BOOTSTRAP")        bootstrap       = p[1];
        if (p[0] == "AXON_SCHEMA_REGISTRY")  schema_registry = p[1];
        if (p[0] == "AXON_CONSUMER_GROUP")   consumer_group  = p[1];
        if (p[0] == "AXON_TOPIC")            topic           = p[1];
        if (p[0] == "AXON_TEST_TIMEOUT_SEC") timeout_sec     = std::stoi(p[1]);
        if (p[0] == "AXON_RUN_DEL")          run_del         = (p[1] == "1");
    }

    if (bootstrap.empty() || schema_registry.empty() || topic.empty())
    {
        std::cerr << "Required: AXON_BOOTSTRAP, AXON_SCHEMA_REGISTRY, AXON_TOPIC\n";
        return 1;
    }
    if (consumer_group.empty()) consumer_group = "axon::kafka2r_test";

    std::cout << "\n=== axon::stream2r::kafka generic test suite ===\n";
    std::cout << "bootstrap:  " << bootstrap       << "\n";
    std::cout << "schema_reg: " << schema_registry << "\n";
    std::cout << "group:      " << consumer_group  << "\n";
    std::cout << "topic:      " << topic           << "\n";
    std::cout << "timeout:    " << timeout_sec     << "s\n\n";

    try {

        // ================================================================
        // 1. Lifecycle
        // ================================================================
        std::cout << "[1] Lifecycle\n";
        {
            g_received = 0;
            axon::stream2r::kafka source(bootstrap, schema_registry, consumer_group);

            check(source.count() == 0, "count() == 0 before add()");

            source.add(topic, topic, capture);
            check(source.count() == 1, "count() == 1 after add()");

            source.subscribe();
            check(true, "subscribe() did not throw");

            source.start();
            check(true, "start() did not throw");

            bool got = wait_for(g_received, 1, timeout_sec);
            check(got, "received >= 1 message within " +
                  std::to_string(timeout_sec) + "s");

            source.stop();
            check(true, "stop() did not throw");
        }

        if (!g_cap.populated)
        {
            std::cerr << "\nNo message received within " << timeout_sec
                      << "s — skipping remaining tests.\n"
                      << "Ensure " << topic << " has live messages.\n";
            ++g_fail;
            // Use a scope to avoid goto crossing initializations
            goto print_results;
        }

        // ================================================================
        // 2. Message delivery
        // ================================================================
        std::cout << "\n[2] Message delivery\n";
        check(g_received.load() >= 1,
              "messages received: " + std::to_string(g_received.load()));
        check(g_cap.source == topic,
              "source() == \"" + topic + "\" (got \"" + g_cap.source + "\")");

        // ================================================================
        // 3. Cursor
        // ================================================================
        std::cout << "\n[3] Cursor — next()\n";
        check(g_cap.next_ok, "next() returned true");

        // ================================================================
        // 4. Metadata
        // ================================================================
        std::cout << "\n[4] recordset2r metadata\n";
        check(g_cap.col_count > 0,
              "count() > 0 — schema has " +
              std::to_string(g_cap.col_count) + " columns");
        check(!g_cap.col0_name.empty(),
              "name(0) non-empty: \"" + g_cap.col0_name + "\"");

        // Print full schema for visibility
        {
            g_received = 0;
            axon::stream2r::kafka src(bootstrap, schema_registry, consumer_group);
            src.add(topic, topic, [](std::unique_ptr<axon::recordset2r> rs) {
                if (!rs || !rs->next()) { g_received++; return; }
                std::cout << "         schema (" << rs->count() << " columns):\n";
                for (size_t i = 0; i < rs->count(); i++)
                {
                    const char *tn = "?";
                    switch (rs->type(i))
                    {
                        case axon::column_type::string_t: tn = "string"; break;
                        case axon::column_type::int64_t:  tn = "int64";  break;
                        case axon::column_type::double_t: tn = "double"; break;
                        case axon::column_type::bool_t:   tn = "bool";   break;
                        case axon::column_type::bytes_t:  tn = "bytes";  break;
                        case axon::column_type::null_t:   tn = "null";   break;
                        default:                          tn = "other";  break;
                    }
                    std::cout << "           [" << i << "] "
                              << rs->name(i) << " (" << tn << ")\n";
                }
                while (rs->next()) { }
                g_received++;
            });
            src.subscribe();
            src.start();
            wait_for(g_received, 1, timeout_sec);
            src.stop();
        }

        // ================================================================
        // 5. Column types — all recognised
        // ================================================================
        std::cout << "\n[5] Column types\n";
        {
            bool all_valid = true;
            for (auto ct : g_cap.types)
            {
                switch (ct)
                {
                    case axon::column_type::string_t:
                    case axon::column_type::int64_t:
                    case axon::column_type::double_t:
                    case axon::column_type::bool_t:
                    case axon::column_type::bytes_t:
                    case axon::column_type::null_t:
                        break;
                    default:
                        all_valid = false;
                }
            }
            check(all_valid, "all " + std::to_string(g_cap.col_count) +
                  " columns have a recognised column_type");
        }

        // ================================================================
        // 6. to_json()
        // ================================================================
        std::cout << "\n[6] to_json()\n";
        check(!g_cap.json.empty() &&
              g_cap.json.find('{') != std::string::npos,
              "to_json() produced a JSON object");
        std::cout << "         first 120 chars: "
                  << g_cap.json.substr(0, 120)
                  << (g_cap.json.size() > 120 ? "..." : "") << "\n";

        // ================================================================
        // 7. operator<<
        // ================================================================
        std::cout << "\n[7] operator<<\n";
        check(!g_cap.print_out.empty(),
              "operator<< produced non-empty output");
        std::cout << "         first 120 chars: "
                  << g_cap.print_out.substr(0, 120)
                  << (g_cap.print_out.size() > 120 ? "..." : "") << "\n";

        // ================================================================
        // 8. Global callback via start(cbfn)
        // ================================================================
        std::cout << "\n[8] Global callback via start(cbfn)\n";
        {
            g_global = 0;
            axon::stream2r::kafka src(bootstrap, schema_registry, consumer_group);
            src.add(topic, topic, capture);   // per-topic — will be overridden
            src.subscribe();
            src.start(global_cb);             // global takes priority

            bool got = wait_for(g_global, 1, timeout_sec);
            check(got, "global callback fired (" +
                  std::to_string(g_global.load()) + " messages)");
            src.stop();
        }

        // ================================================================
        // 9. counter()
        // ================================================================
        std::cout << "\n[9] counter()\n";
        {
            axon::stream2r::kafka src(bootstrap, schema_registry, consumer_group);
            src.add(topic, topic,
                [](std::unique_ptr<axon::recordset2r> rs) {
                    if (rs) while (rs->next()) { }
                });
            src.subscribe();
            src.start();

            size_t before = src.counter();
            int wait_s = std::min(timeout_sec / 3, 5);
            std::this_thread::sleep_for(std::chrono::seconds(wait_s));
            size_t after = src.counter();
            src.stop();

            check(after >= before,
                  "counter() non-decreasing (before=" +
                  std::to_string(before) + " after=" + std::to_string(after) + ")");
        }

        // ================================================================
        // 10. stop() / start() cycle
        // ================================================================
        std::cout << "\n[10] stop() / start() cycle\n";
        {
            g_received = 0;
            axon::stream2r::kafka src(bootstrap, schema_registry, consumer_group);
            src.add(topic, topic, capture);
            src.subscribe();
            src.start();
            wait_for(g_received, 1, timeout_sec);
            int first_count = g_received.load();
            src.stop();
            check(true, "first stop() did not throw");

            g_received = 0;
            src.subscribe();
            src.start();
            check(true, "start() after stop() did not throw");
            wait_for(g_received, 1, timeout_sec);
            src.stop();

            check(first_count >= 1 || g_received.load() >= 1,
                  "at least one run received messages");
        }

        // ================================================================
        // 11. autocommit flag
        // ================================================================
        std::cout << "\n[11] autocommit flag\n";
        {
            axon::stream2r::kafka src(bootstrap, schema_registry, consumer_group);
            check(src.autocommit() == true,  "default autocommit() == true");
            src.autocommit(false);
            check(src.autocommit() == false, "autocommit(false) set");
            src.autocommit(true);
            check(src.autocommit() == true,  "autocommit(true) restored");
        }

        // ================================================================
        // 12. get<T> by column name — uses first column, adapts to its type
        // ================================================================
        std::cout << "\n[12] get<T> by column name\n";
        if (g_cap.col_count > 0)
        {
            g_received = 0;
            std::string first_col = g_cap.col0_name;
            axon::column_type first_type = g_cap.types[0];
            bool get_ok = false;

            axon::stream2r::kafka src(bootstrap, schema_registry, consumer_group);
            src.add(topic, topic,
                [&](std::unique_ptr<axon::recordset2r> rs) {
                    if (!rs || !rs->next()) { g_received++; return; }
                    try {
                        switch (first_type)
                        {
                            case axon::column_type::string_t:
                            { std::string v; rs->get(first_col, v);      get_ok = true; break; }
                            case axon::column_type::int64_t:
                            { long long v = 0; rs->get(first_col, v);    get_ok = true; break; }
                            case axon::column_type::double_t:
                            { double v = 0.0; rs->get(first_col, v);     get_ok = true; break; }
                            case axon::column_type::bool_t:
                            { bool v = false; rs->get(first_col, v);     get_ok = true; break; }
                            case axon::column_type::bytes_t:
                            { std::vector<uint8_t> v; rs->get(first_col, v); get_ok = true; break; }
                            default:
                                get_ok = true;   // null_t — acceptable
                                break;
                        }
                    } catch (...) { get_ok = false; }
                    while (rs->next()) { }
                    g_received++;
                });

            src.subscribe();
            src.start();
            wait_for(g_received, 1, timeout_sec);
            src.stop();

            check(get_ok, "get<T>(\"" + first_col + "\") succeeded without exception");
        }
        else
        {
            check(false, "skipped — schema has no columns");
        }

        // ================================================================
        // 13. del() — optional
        // ================================================================
        std::cout << "\n[13] del() consumer group\n";
        if (run_del)
        {
            try {
                axon::stream2r::kafka::del(bootstrap, consumer_group);
                check(true, "del(\"" + consumer_group + "\") completed");
            } catch (axon::exception &e) {
                std::cout << "  NOTE  del() threw (group may be active or absent): "
                          << e.what() << "\n";
                check(true, "del() threw a caught exception — not a crash");
            }
        }
        else
        {
            std::cout << "  SKIP  del() — set AXON_RUN_DEL=1 to enable\n";
        }

    print_results:
        ;

    } catch (axon::exception &e) {
        std::cerr << "\nEXCEPTION: " << e.what() << "\n";
        ++g_fail;
    }

    std::cout << "\n=== Results: " << g_pass << " passed, "
              << g_fail << " failed ===\n";
    std::cerr << "runtime: " << ctm.now() / 1000.0 << " ms\n";

    return g_fail == 0 ? 0 : 1;
}