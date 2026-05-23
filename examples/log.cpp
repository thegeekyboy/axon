/**
 * log_example.cpp
 *
 * Usage examples for the axon::log class.
 *
 * auto generated code by claude
 */

#include <iostream>
#include <thread>
#include <vector>
#include <memory>

#include <axon.h>
#include <axon/log.h>
#include <axon/util.h>

// ─────────────────────────────────────────────────────────────
// 1. Basic usage — log to stderr (no file opened)
// ─────────────────────────────────────────────────────────────
void example_stderr()
{
    axon::log log;

    // print() with boost::format-style placeholders — %s, %d, %f etc.
    log.print(axon::level::info,    "application starting, version %s", "1.0.0");
    log.print(axon::level::debug,   "debug value: %d", 42);
    log.print(axon::level::warning, "disk usage at %d%%", 87);
    log.print(axon::level::error,   "failed to open file: %s", "/tmp/missing.txt");

    // convenience overload — always logs at info level
    log.print("quick info message with no format args");
}

// ─────────────────────────────────────────────────────────────
// 2. Log to a file — open(path, filename)
// ─────────────────────────────────────────────────────────────
void example_file()
{
    axon::log log;

    try
    {
        // path and filename as separate arguments
        log.open("/tmp", "axon_example.log");

        log.print(axon::level::info,  "log file opened successfully");
        log.print(axon::level::info,  "transfer started: %s -> %s", "source.csv", "dest.csv");
        log.print(axon::level::error, "connection timed out after %d seconds", 30);

        log.close();
    }
    catch (axon::exception& e)
    {
        std::cerr << "Failed to open log file: " << e.what() << std::endl;
    }
}

// ─────────────────────────────────────────────────────────────
// 3. Log to a file — open(fullpath)
// ─────────────────────────────────────────────────────────────
void example_file_fullpath()
{
    axon::log log;

    try
    {
        // full path as a single string — axon splits path/filename internally
        log.open("/tmp/axon_fullpath.log");

        log.print(axon::level::notice,   "service registered with pid %d", getpid());
        log.print(axon::level::critical, "database connection pool exhausted");

        log.close();
    }
    catch (axon::exception& e)
    {
        std::cerr << "Failed: " << e.what() << std::endl;
    }
}

// ─────────────────────────────────────────────────────────────
// 4. Stream operator (<<) usage
// ─────────────────────────────────────────────────────────────
void example_stream_operator()
{
    axon::log log;

    try
    {
        log.open("/tmp", "axon_stream.log");

        // set the level first, then stream values, flush with std::endl
        log << axon::level::info << "transfer complete: " << 1024 << " files" << std::endl;
        log << axon::level::warning << "retry attempt " << 3 << " of " << 5 << std::endl;
        log << axon::level::error << "errno=" << 13 << " permission denied" << std::endl;

        // bool values
        bool connected = false;
        log << axon::level::debug << "connected=" << connected << std::endl;

        // floating point
        double ratio = 98.76;
        log << axon::level::info << "success ratio: " << ratio << "%" << std::endl;

        log.close();
    }
    catch (axon::exception& e)
    {
        std::cerr << "Failed: " << e.what() << std::endl;
    }
}

// ─────────────────────────────────────────────────────────────
// 5. Operator[] — read/set path and filename at runtime
// ─────────────────────────────────────────────────────────────
void example_subscript_operator()
{
    axon::log log;

    // set path and filename via subscript before opening
    log[AXON_LOG_PATH]     = "/tmp";
    log[AXON_LOG_FILENAME] = "axon_subscript.log";

    try
    {
        log.open();

        log.print(axon::level::info, "log opened via subscript operator");

        // read back the current path and filename
        std::string path     = log[AXON_LOG_PATH];
        std::string filename = log[AXON_LOG_FILENAME];

        log.print(axon::level::debug, "logging to: %s/%s", path.c_str(), filename.c_str());

        log.close();
    }
    catch (axon::exception& e)
    {
        std::cerr << "Failed: " << e.what() << std::endl;
    }
}

// ─────────────────────────────────────────────────────────────
// 6. Shared log across multiple threads
// ─────────────────────────────────────────────────────────────
void worker(std::shared_ptr<axon::log> log, int id)
{
    for (int i = 0; i < 5; i++)
    {
        log->print(axon::level::info, "worker %d iteration %d", id, i);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void example_multithreaded()
{
    auto log = std::make_shared<axon::log>();

    try
    {
        log->open("/tmp", "axon_threaded.log");

        std::vector<std::thread> threads;

        for (int i = 0; i < 4; i++)
            threads.emplace_back(worker, log, i);

        for (auto& t : threads)
            t.join();

        log->close();
    }
    catch (axon::exception& e)
    {
        std::cerr << "Failed: " << e.what() << std::endl;
    }
}

// ─────────────────────────────────────────────────────────────
// 7. reset() — reuse a log object for a different file
// ─────────────────────────────────────────────────────────────
void example_reset()
{
    axon::log log;

    try
    {
        log.open("/tmp", "axon_first.log");
        log.print(axon::level::info, "writing to first log");
        log.close();

        // reset clears path, filename, and closes the file
        log.reset();

        log.open("/tmp", "axon_second.log");
        log.print(axon::level::info, "writing to second log after reset");
        log.close();
    }
    catch (axon::exception& e)
    {
        std::cerr << "Failed: " << e.what() << std::endl;
    }
}

// ─────────────────────────────────────────────────────────────
// 8. Copy constructor — two log objects sharing same config
// ─────────────────────────────────────────────────────────────
void example_copy()
{
    axon::log original;

    try
    {
        original.open("/tmp", "axon_copy.log");
        original.print(axon::level::info, "written from original");

        // copy constructor opens the same file independently
        axon::log copy(original);
        copy.print(axon::level::info, "written from copy");

        original.close();
        copy.close();
    }
    catch (axon::exception& e)
    {
        std::cerr << "Failed: " << e.what() << std::endl;
    }
}

// ─────────────────────────────────────────────────────────────
// 9. All log levels demonstrated
// ─────────────────────────────────────────────────────────────
void example_all_levels()
{
    axon::log log;

    // without opening a file, all output goes to stderr
    log.print(axon::level::emergency, "system is unusable");
    log.print(axon::level::alert,     "action must be taken immediately");
    log.print(axon::level::critical,  "critical condition detected");
    log.print(axon::level::error,     "error condition");
    log.print(axon::level::warning,   "warning condition");
    log.print(axon::level::notice,    "normal but significant condition");
    log.print(axon::level::info,      "informational message");
    log.print(axon::level::debug,     "debug-level message");
}

// ─────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────
int main()
{
    std::cout << "=== 1. stderr (no file) ===" << std::endl;
    example_stderr();

    std::cout << "\n=== 2. file: open(path, filename) ===" << std::endl;
    example_file();

    std::cout << "\n=== 3. file: open(fullpath) ===" << std::endl;
    example_file_fullpath();

    std::cout << "\n=== 4. stream operator << ===" << std::endl;
    example_stream_operator();

    std::cout << "\n=== 5. subscript operator [] ===" << std::endl;
    example_subscript_operator();

    std::cout << "\n=== 6. multithreaded shared log ===" << std::endl;
    example_multithreaded();

    std::cout << "\n=== 7. reset and reuse ===" << std::endl;
    example_reset();

    std::cout << "\n=== 8. copy constructor ===" << std::endl;
    example_copy();

    std::cout << "\n=== 9. all log levels ===" << std::endl;
    example_all_levels();

    std::cout << "\nDone. Check /tmp/axon_*.log for file output." << std::endl;

    return 0;
}