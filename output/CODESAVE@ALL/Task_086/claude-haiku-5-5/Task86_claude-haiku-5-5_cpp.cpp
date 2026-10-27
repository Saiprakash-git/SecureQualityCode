#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <urlmon.h>
#include <iostream>
#include <string>
#include <cstring>
#include <algorithm>
#include <cctype>

#pragma comment(lib, "urlmon.lib")

static bool isValidHttpUrl(const std::string& url) {
    std::string lower = url;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return lower.rfind("http://", 0) == 0 || lower.rfind("https://", 0) == 0;
}

static std::string getFileNameFromUrl(const std::string& url) {
    std::string path = url;
    size_t q = path.find_first_of("?#");
    if (q != std::string::npos) path = path.substr(0, q);
    size_t s = path.find_last_of('/');
    std::string name = (s == std::string::npos) ? path : path.substr(s + 1);
    if (name.empty() || name.find(':') != std::string::npos) name = "downloaded.exe";
    if (name.size() < 4 || _stricmp(name.c_str() + name.size() - 4, ".exe") != 0) {
        name += ".exe";
    }
    return name;
}

static bool buildTempPath(const std::string& fileName, std::string& outPath) {
    char tempDir[MAX_PATH];
    DWORD len = GetTempPathA(MAX_PATH, tempDir);
    if (len == 0 || len > MAX_PATH) return false;
    outPath = std::string(tempDir) + fileName;
    return true;
}

static bool downloadFile(const std::string& url, const std::string& destPath) {
    HRESULT hr = URLDownloadToFileA(nullptr, url.c_str(), destPath.c_str(), 0, nullptr);
    return SUCCEEDED(hr);
}

static bool isPortableExecutable(const std::string& path) {
    HANDLE h = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    char mz[2] = {0, 0};
    DWORD read = 0;
    BOOL ok = ReadFile(h, mz, 2, &read, nullptr);
    CloseHandle(h);
    return ok && read == 2 && mz[0] == 'M' && mz[1] == 'Z';
}

static bool executeFile(const std::string& path, DWORD& exitCode) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);

    std::string cmd = "\"" + path + "\"";
    std::string mutableCmd = cmd;
    std::string workDir = path.substr(0, path.find_last_of("\\/"));

    BOOL created = CreateProcessA(nullptr, &mutableCmd[0], nullptr, nullptr, FALSE,
                                  0, nullptr, workDir.c_str(), &si, &pi);
    if (!created) return false;

    WaitForSingleObject(pi.hProcess, INFINITE);
    exitCode = 0;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return true;
}

static bool confirmExecution(const std::string& url, const std::string& path) {
    std::cout << "About to download from:\n  " << url << "\n"
              << "Saved to:\n  " << path << "\n"
              << "Download and execute this program? (y/N): ";
    std::string answer;
    std::getline(std::cin, answer);
    return !answer.empty() && (answer[0] == 'y' || answer[0] == 'Y');
}

int main(int argc, char* argv[]) {
    std::string url;
    if (argc >= 2) {
        url = argv[1];
    } else {
        std::cout << "Enter URL of EXE file: ";
        std::getline(std::cin, url);
    }

    url.erase(0, url.find_first_not_of(" \t\r\n"));
    url.erase(url.find_last_not_of(" \t\r\n") + 1);

    if (!isValidHttpUrl(url)) {
        std::cerr << "Error: URL must begin with http:// or https://\n";
        return 1;
    }

    std::string fileName = getFileNameFromUrl(url);
    std::string localPath;
    if (!buildTempPath(fileName, localPath)) {
        std::cerr << "Error: could not determine temporary directory\n";
        return 1;
    }

    if (!confirmExecution(url, localPath)) {
        std::cout << "Aborted by user.\n";
        return 0;
    }

    std::cout << "Downloading...\n";
    if (!downloadFile(url, localPath)) {
        std::cerr << "Error: download failed\n";
        return 1;
    }

    if (!isPortableExecutable(localPath)) {
        std::cerr << "Error: downloaded file is not a valid EXE (missing MZ header)\n";
        DeleteFileA(localPath.c_str());
        return 1;
    }

    std::cout << "Executing...\n";
    DWORD exitCode = 0;
    if (!executeFile(localPath, exitCode)) {
        std::cerr << "Error: failed to execute file (error " << GetLastError() << ")\n";
        return 1;
    }

    std::cout << "Program exited with code " << exitCode << "\n";
    return 0;
}