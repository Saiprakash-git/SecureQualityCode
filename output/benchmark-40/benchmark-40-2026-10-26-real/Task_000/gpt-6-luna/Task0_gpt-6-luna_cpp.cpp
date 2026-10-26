#include <atomic>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <vector>

class ConcurrentFileProcessor {
public:
    explicit ConcurrentFileProcessor(std::size_t workerCount)
        : workerCount_(workerCount == 0 ? 1 : workerCount) {}

    void processDirectory(const std::filesystem::path& directory) {
        std::vector<std::filesystem::path> files = collectFiles(directory);
        std::queue<std::filesystem::path> jobs;
        for (const auto& file : files) {
            jobs.push(file);
        }

        std::mutex queueMutex;
        std::vector<std::thread> workers;
        workers.reserve(workerCount_);

        for (std::size_t i = 0; i < workerCount_; ++i) {
            workers.emplace_back([this, &jobs, &queueMutex]() {
                while (true) {
                    std::filesystem::path file;
                    {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        if (jobs.empty()) {
                            return;
                        }
                        file = std::move(jobs.front());
                        jobs.pop();
                    }
                    processOneFile(file);
                }
            });
        }

        for (auto& worker : workers) {
            worker.join();
        }
    }

private:
    struct FileResult {
        std::uint64_t bytes = 0;
        std::uint64_t hash = 14695981039346656037ULL;
    };

    std::size_t workerCount_;
    std::mutex lockMapMutex_;
    std::mutex outputMutex_;
    std::unordered_map<std::string, std::shared_ptr<std::mutex>> fileLocks_;

    static std::vector<std::filesystem::path> collectFiles(
        const std::filesystem::path& directory) {
        std::vector<std::filesystem::path> files;
        std::set<std::string> seen;
        std::error_code ec;

        std::filesystem::recursive_directory_iterator it(
            directory, std::filesystem::directory_options::skip_permission_denied, ec);
        const std::filesystem::recursive_directory_iterator end;

        while (!ec && it != end) {
            const auto status = it->symlink_status(ec);
            if (!ec && std::filesystem::is_regular_file(status)) {
                std::error_code pathError;
                auto path = std::filesystem::absolute(it->path(), pathError);
                if (!pathError) {
                    path = path.lexically_normal();
                    const std::string key = path.string();
                    if (seen.insert(key).second) {
                        files.push_back(std::move(path));
                    }
                }
            }

            it.increment(ec);
        }

        return files;
    }

    std::shared_ptr<std::mutex> mutexForFile(const std::filesystem::path& file) {
        const std::string key = file.lexically_normal().string();
        std::lock_guard<std::mutex> lock(lockMapMutex_);

        auto& fileMutex = fileLocks_[key];
        if (!fileMutex) {
            fileMutex = std::make_shared<std::mutex>();
        }
        return fileMutex;
    }

    static FileResult processFile(const std::filesystem::path& file) {
        std::ifstream input(file, std::ios::binary);
        if (!input) {
            throw std::runtime_error("unable to open file");
        }

        FileResult result;
        char buffer[8192];

        while (input) {
            input.read(buffer, sizeof(buffer));
            const std::streamsize count = input.gcount();
            result.bytes += static_cast<std::uint64_t>(count);

            for (std::streamsize i = 0; i < count; ++i) {
                result.hash ^= static_cast<unsigned char>(buffer[i]);
                result.hash *= 1099511628211ULL;
            }
        }

        if (!input.eof()) {
            throw std::runtime_error("error while reading file");
        }

        return result;
    }

    void processOneFile(const std::filesystem::path& file) {
        const auto fileMutex = mutexForFile(file);
        std::lock_guard<std::mutex> fileLock(*fileMutex);

        try {
            const FileResult result = processFile(file);
            std::lock_guard<std::mutex> outputLock(outputMutex_);
            std::cout << file.string()
                      << " bytes=" << result.bytes
                      << " fnv1a64=" << result.hash << '\n';
        } catch (const std::exception& error) {
            std::lock_guard<std::mutex> outputLock(outputMutex_);
            std::cerr << file.string() << ": " << error.what() << '\n';
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2 || argc > 3) {
        std::cerr << "Usage: " << argv[0] << " <directory> [thread-count]\n";
        return 1;
    }

    std::size_t threadCount = std::thread::hardware_concurrency();
    if (threadCount == 0) {
        threadCount = 1;
    }

    if (argc == 3) {
        try {
            const unsigned long long parsed = std::stoull(argv[2]);
            if (parsed == 0) {
                std::cerr << "Thread count must be greater than zero.\n";
                return 1;
            }
            threadCount = static_cast<std::size_t>(parsed);
        } catch (const std::exception&) {
            std::cerr << "Invalid thread count.\n";
            return 1;
        }
    }

    std::error_code ec;
    if (!std::filesystem::is_directory(argv[1], ec) || ec) {
        std::cerr << "The specified path is not a readable directory.\n";
        return 1;
    }

    ConcurrentFileProcessor processor(threadCount);
    processor.processDirectory(argv[1]);
    return 0;
}