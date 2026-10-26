#include <iostream>
#include <fstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <vector>
#include <string>
#include <filesystem>
#include <chrono>
#include <atomic>
#include <functional>

namespace fs = std::filesystem;

class ConcurrentFileProcessor {
private:
    std::mutex mtx;
    std::condition_variable cv;
    std::unordered_set<std::string> activeFiles;

public:
    void processFile(const std::string& filepath, std::function<void(const std::string&)> work) {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, [this, &filepath]() {
            return activeFiles.find(filepath) == activeFiles.end();
        });

        activeFiles.insert(filepath);
        std::cout << "Thread " << std::this_thread::get_id()
                  << " acquired lock for: " << filepath << std::endl;
        lock.unlock();

        try {
            work(filepath);
        } catch (...) {
            lock.lock();
            activeFiles.erase(filepath);
            lock.unlock();
            cv.notify_all();
            throw;
        }

        lock.lock();
        activeFiles.erase(filepath);
        std::cout << "Thread " << std::this_thread::get_id()
                  << " released lock for: " << filepath << std::endl;
        lock.unlock();
        cv.notify_all();
    }

    bool isFileLocked(const std::string& filepath) {
        std::lock_guard<std::mutex> lock(mtx);
        return activeFiles.find(filepath) != activeFiles.end();
    }
};

void processFileContent(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filepath << std::endl;
        return;
    }

    std::string line;
    int lineCount = 0;
    while (std::getline(file, line)) {
        lineCount++;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << "Thread " << std::this_thread::get_id()
              << " processed " << filepath << " (" << lineCount << " lines)" << std::endl;
}

int main() {
    fs::path testDir = "concurrent_test_files";
    fs::create_directories(testDir);

    std::vector<std::string> baseFiles = {"data1.txt", "data2.txt", "data3.txt", "data4.txt", "data5.txt"};
    for (const auto& name : baseFiles) {
        std::ofstream out(testDir / name);
        for (int i = 0; i < 10; ++i) {
            out << "Content line " << i << " for " << name << "\n";
        }
    }

    std::vector<std::string> workQueue;
    for (const auto& name : baseFiles) {
        std::string fullPath = (testDir / name).string();
        workQueue.push_back(fullPath);
        workQueue.push_back(fullPath);
    }

    ConcurrentFileProcessor processor;
    std::atomic<size_t> nextIndex{0};
    const size_t numThreads = 4;
    const size_t totalTasks = workQueue.size();

    auto worker = [&]() {
        while (true) {
            size_t idx = nextIndex.fetch_add(1);
            if (idx >= totalTasks) break;

            const std::string& filepath = workQueue[idx];
            processor.processFile(filepath, processFileContent);
        }
    };

    std::vector<std::thread> threads;
    for (size_t i = 0; i < numThreads; ++i) {
        threads.emplace_back(worker);
    }

    for (auto& t : threads) {
        t.join();
    }

    fs::remove_all(testDir);
    std::cout << "All file processing completed successfully." << std::endl;

    return 0;
}