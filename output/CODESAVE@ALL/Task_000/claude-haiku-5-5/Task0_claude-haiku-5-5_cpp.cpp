#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

struct FileStats {
    std::size_t lines = 0;
    std::size_t words = 0;
    std::size_t bytes = 0;
};

struct FileResult {
    bool ok = false;
    FileStats stats;
    std::string error;
};

class FileLockManager {
public:
    void lock(const std::string& key) {
        std::unique_lock<std::mutex> lk(m_);
        cv_.wait(lk, [&] { return locked_.find(key) == locked_.end(); });
        locked_.insert(key);
    }

    void unlock(const std::string& key) {
        {
            std::lock_guard<std::mutex> lk(m_);
            locked_.erase(key);
        }
        cv_.notify_all();
    }

private:
    std::mutex m_;
    std::condition_variable cv_;
    std::set<std::string> locked_;
};

class FileLockGuard {
public:
    FileLockGuard(FileLockManager& mgr, std::string key)
        : mgr_(mgr), key_(std::move(key)) {
        mgr_.lock(key_);
    }
    ~FileLockGuard() { mgr_.unlock(key_); }
    FileLockGuard(const FileLockGuard&) = delete;
    FileLockGuard& operator=(const FileLockGuard&) = delete;

private:
    FileLockManager& mgr_;
    std::string key_;
};

FileStats processFile(const fs::path& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        throw std::runtime_error("cannot open file");
    }
    FileStats stats;
    std::string line;
    bool inWord = false;
    char c;
    while (in.get(c)) {
        ++stats.bytes;
        if (c == '\n') {
            ++stats.lines;
        }
        bool space = (c == ' ' || c == '\t' || c == '\n' || c == '\r' ||
                      c == '\v' || c == '\f');
        if (!space && !inWord) {
            ++stats.words;
            inWord = true;
        } else if (space) {
            inWord = false;
        }
    }
    if (in.bad()) {
        throw std::runtime_error("read error");
    }
    return stats;
}

std::map<std::string, FileResult> processDirectory(const fs::path& dir,
                                                   std::size_t numThreads) {
    std::vector<fs::path> files;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (entry.is_regular_file(ec)) {
            files.push_back(entry.path());
        }
    }

    std::map<std::string, FileResult> results;
    std::mutex resultsMutex;
    FileLockManager lockManager;
    std::atomic<std::size_t> nextIndex{0};

    if (numThreads == 0) {
        numThreads = 1;
    }

    auto worker = [&]() {
        while (true) {
            std::size_t idx = nextIndex.fetch_add(1);
            if (idx >= files.size()) {
                break;
            }
            const fs::path& path = files[idx];
            std::string key = path.string();

            FileResult result;
            try {
                FileLockGuard guard(lockManager, key);
                result.stats = processFile(path);
                result.ok = true;
            } catch (const std::exception& e) {
                result.ok = false;
                result.error = e.what();
            }

            std::lock_guard<std::mutex> lk(resultsMutex);
            results[key] = result;
        }
    };

    std::vector<std::thread> threads;
    threads.reserve(numThreads);
    for (std::size_t i = 0; i < numThreads; ++i) {
        threads.emplace_back(worker);
    }
    for (auto& t : threads) {
        t.join();
    }

    return results;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <directory> [threads]\n";
        return 1;
    }

    fs::path dir = argv[1];
    if (!fs::is_directory(dir)) {
        std::cerr << "Not a directory: " << dir << "\n";
        return 1;
    }

    std::size_t numThreads = std::thread::hardware_concurrency();
    if (argc >= 3) {
        try {
            numThreads = std::stoul(argv[2]);
        } catch (...) {
            std::cerr << "Invalid thread count\n";
            return 1;
        }
    }
    if (numThreads == 0) {
        numThreads = 1;
    }

    auto results = processDirectory(dir, numThreads);

    FileStats total;
    std::size_t failures = 0;
    for (const auto& [name, res] : results) {
        if (res.ok) {
            std::cout << name << ": lines=" << res.stats.lines
                      << " words=" << res.stats.words
                      << " bytes=" << res.stats.bytes << "\n";
            total.lines += res.stats.lines;
            total.words += res.stats.words;
            total.bytes += res.stats.bytes;
        } else {
            ++failures;
            std::cout << name << ": ERROR " << res.error << "\n";
        }
    }

    std::cout << "Total: lines=" << total.lines
              << " words=" << total.words
              << " bytes=" << total.bytes
              << " files=" << results.size()
              << " failures=" << failures << "\n";

    return failures == 0 ? 0 : 2;
}