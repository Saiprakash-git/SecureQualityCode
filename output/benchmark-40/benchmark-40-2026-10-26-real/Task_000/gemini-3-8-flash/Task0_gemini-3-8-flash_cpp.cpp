#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <queue>
#include <unordered_set>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <chrono>
#include <filesystem>
#include <sstream>
#include <iomanip>

namespace fs = std::filesystem;

class ThreadSafeLogger {
private:
    std::mutex log_mutex;

public:
    void log(const std::string& message) {
        std::lock_guard<std::mutex> lock(log_mutex);
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
        
        std::tm tm_buf;
#if defined(_WIN32) || defined(_WIN64)
        localtime_s(&tm_buf, &in_time_t);
#else
        localtime_r(&in_time_t, &tm_buf);
#endif
        std::cout << "[" << std::put_time(&tm_buf, "%H:%M:%S") << "." 
                  << std::setfill('0') << std::setw(3) << ms.count() << "] "
                  << message << std::endl;
    }
};

class FileLockManager {
private:
    std::mutex state_mutex;
    std::condition_variable cv;
    std::unordered_set<std::string> active_files;

public:
    void acquireLock(const std::string& filepath) {
        std::unique_lock<std::mutex> lock(state_mutex);
        cv.wait(lock, [this, &filepath]() {
            return active_files.find(filepath) == active_files.end();
        });
        active_files.insert(filepath);
    }

    void releaseLock(const std::string& filepath) {
        std::unique_lock<std::mutex> lock(state_mutex);
        active_files.erase(filepath);
        cv.notify_all();
    }
};

class FileGuard {
private:
    FileLockManager& manager;
    std::string filepath;

public:
    FileGuard(FileLockManager& mgr, std::string path)
        : manager(mgr), filepath(std::move(path)) {
        manager.acquireLock(filepath);
    }

    ~FileGuard() {
        manager.releaseLock(filepath);
    }

    FileGuard(const FileGuard&) = delete;
    FileGuard& operator=(const FileGuard&) = delete;
};

class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex queue_mutex;
    std::condition_variable cv;
    bool stop;

public:
    explicit ThreadPool(size_t threads) : stop(false) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this]() {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->cv.wait(lock, [this]() {
                            return this->stop || !this->tasks.empty();
                        });
                        if (this->stop && this->tasks.empty()) {
                            return;
                        }
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
        }
    }

    template <class F, class... Args>
    auto enqueue(F&& f, Args&&... args) 
        -> std::future<typename std::invoke_result<F, Args...>::type> {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            if (stop) {
                throw std::runtime_error("Enqueue called on stopped ThreadPool");
            }
            tasks.emplace([task]() { (*task)(); });
        }
        cv.notify_one();
        return res;
    }

    void shutdown() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        cv.notify_all();
        for (std::thread& worker : workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    ~ThreadPool() {
        shutdown();
    }
};

struct FileProcessResult {
    std::string filepath;
    size_t line_count;
    size_t word_count;
    size_t byte_count;
    bool success;
    std::string error_message;
};

FileProcessResult processSingleFile(const std::string& filepath, 
                                   FileLockManager& lock_mgr, 
                                   ThreadSafeLogger& logger) {
    FileProcessResult result{filepath, 0, 0, 0, false, ""};
    
    std::ostringstream ss_start;
    ss_start << "[Thread " << std::this_thread::get_id() << "] Waiting for access to: " << filepath;
    logger.log(ss_start.str());

    {
        FileGuard guard(lock_mgr, filepath);

        std::ostringstream ss_lock;
        ss_lock << "[Thread " << std::this_thread::get_id() << "] Lock acquired. Processing: " << filepath;
        logger.log(ss_lock.str());

        std::ifstream file(filepath, std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            result.error_message = "Failed to open file: " + filepath;
            return result;
        }

        std::string line;
        while (std::getline(file, line)) {
            result.line_count++;
            result.byte_count += line.length() + 1;
            
            std::istringstream line_stream(line);
            std::string word;
            while (line_stream >> word) {
                result.word_count++;
            }
        }
        file.close();

        // Simulate computationally intensive I/O operations
        std::this_thread::sleep_for(std::chrono::milliseconds(150));

        // Append processing log safely to the file
        std::ofstream outfile(filepath, std::ios::out | std::ios::app);
        if (outfile.is_open()) {
            outfile << "\n# Processed by thread " << std::this_thread::get_id() << "\n";
            outfile.close();
        }

        result.success = true;

        std::ostringstream ss_done;
        ss_done << "[Thread " << std::this_thread::get_id() << "] Completed. Releasing lock: " << filepath;
        logger.log(ss_done.str());
    }

    return result;
}

std::vector<FileProcessResult> processDirectoryConcurrently(const std::string& directory_path, 
                                                            size_t thread_count, 
                                                            int duplicate_task_factor,
                                                            ThreadSafeLogger& logger) {
    FileLockManager lock_manager;
    ThreadPool pool(thread_count);
    std::vector<std::string> file_paths;

    for (const auto& entry : fs::directory_iterator(directory_path)) {
        if (entry.is_regular_file()) {
            file_paths.push_back(entry.path().string());
        }
    }

    std::vector<std::future<FileProcessResult>> futures;

    for (int rep = 0; rep < duplicate_task_factor; ++rep) {
        for (const auto& path : file_paths) {
            futures.push_back(
                pool.enqueue(processSingleFile, path, std::ref(lock_manager), std::ref(logger))
            );
        }
    }

    std::vector<FileProcessResult> results;
    results.reserve(futures.size());
    for (auto& fut : futures) {
        results.push_back(fut.get());
    }

    pool.shutdown();
    return results;
}

void createSampleFiles(const std::string& directory_path, size_t file_count) {
    if (!fs::exists(directory_path)) {
        fs::create_directories(directory_path);
    }

    for (size_t i = 1; i <= file_count; ++i) {
        std::string filename = directory_path + "/sample_" + std::to_string(i) + ".txt";
        std::ofstream outfile(filename, std::ios::out | std::ios::trunc);
        if (outfile.is_open()) {
            for (int line = 1; line <= 50; ++line) {
                outfile << "Line " << line << " in file " << i 
                        << " contains several words for concurrent processing testing.\n";
            }
            outfile.close();
        }
    }
}

void cleanupSampleFiles(const std::string& directory_path) {
    if (fs::exists(directory_path)) {
        fs::remove_all(directory_path);
    }
}

int main() {
    const std::string test_dir = "./concurrent_processing_temp_dir";
    const size_t file_count = 5;
    const size_t num_threads = 4;
    const int duplicate_tasks = 2; // Tasks queued per file to verify concurrent exclusion
    
    ThreadSafeLogger logger;

    logger.log("Generating sample files for test run...");
    createSampleFiles(test_dir, file_count);

    logger.log("Dispatching concurrent file processing jobs...");
    std::vector<FileProcessResult> results = processDirectoryConcurrently(
        test_dir, 
        num_threads, 
        duplicate_tasks, 
        logger
    );

    logger.log("All processing tasks finalized. Aggregating results:");
    size_t successful_tasks = 0;
    for (const auto& res : results) {
        if (res.success) {
            successful_tasks++;
            std::ostringstream ss;
            ss << "SUCCESS | File: " << res.filepath 
               << " | Lines: " << res.line_count 
               << " | Words: " << res.word_count 
               << " | Bytes: " << res.byte_count;
            logger.log(ss.str());
        } else {
            logger.log("FAILED  | File: " + res.filepath + " | Error: " + res.error_message);
        }
    }

    std::ostringstream summary;
    summary << "Processed " << successful_tasks << " / " << results.size() << " tasks successfully.";
    logger.log(summary.str());

    logger.log("Cleaning up temporary directory...");
    cleanupSampleFiles(test_dir);
    logger.log("Execution complete.");

    return 0;
}