#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "winhttp.lib")

bool DownloadHttpsFile(const std::wstring& url, const std::wstring& destination)
{
    URL_COMPONENTS parts{};
    parts.dwStructSize = sizeof(parts);
    parts.dwSchemeLength = static_cast<DWORD>(-1);
    parts.dwHostNameLength = static_cast<DWORD>(-1);
    parts.dwUrlPathLength = static_cast<DWORD>(-1);
    parts.dwExtraInfoLength = static_cast<DWORD>(-1);

    if (!WinHttpCrackUrl(url.c_str(), 0, 0, &parts) ||
        parts.nScheme != INTERNET_SCHEME_HTTPS ||
        parts.dwHostNameLength == 0)
    {
        return false;
    }

    const std::wstring host(parts.lpszHostName, parts.dwHostNameLength);
    std::wstring path;
    if (parts.dwUrlPathLength > 0)
        path.assign(parts.lpszUrlPath, parts.dwUrlPathLength);
    else
        path = L"/";

    if (parts.dwExtraInfoLength > 0)
        path.append(parts.lpszExtraInfo, parts.dwExtraInfoLength);

    HINTERNET session = WinHttpOpen(
        L"SafeFileDownloader/1.0",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (!session)
        return false;

    HINTERNET connection = WinHttpConnect(
        session, host.c_str(), parts.nPort, 0);

    if (!connection)
    {
        WinHttpCloseHandle(session);
        return false;
    }

    HINTERNET request = WinHttpOpenRequest(
        connection,
        L"GET",
        path.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);

    if (!request)
    {
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }

    const BOOL sent = WinHttpSendRequest(
        request,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0);

    const BOOL received = sent && WinHttpReceiveResponse(request, nullptr);

    DWORD statusCode = 0;
    DWORD statusSize = sizeof(statusCode);
    const BOOL gotStatus = received && WinHttpQueryHeaders(
        request,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &statusCode,
        &statusSize,
        WINHTTP_NO_HEADER_INDEX);

    HANDLE file = INVALID_HANDLE_VALUE;
    if (gotStatus && statusCode == HTTP_STATUS_OK)
    {
        file = CreateFileW(
            destination.c_str(),
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_NEW,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
    }

    bool success = file != INVALID_HANDLE_VALUE;
    std::vector<char> buffer(64 * 1024);

    while (success)
    {
        DWORD bytesRead = 0;
        if (!WinHttpReadData(
                request,
                buffer.data(),
                static_cast<DWORD>(buffer.size()),
                &bytesRead))
        {
            success = false;
            break;
        }

        if (bytesRead == 0)
            break;

        DWORD offset = 0;
        while (offset < bytesRead)
        {
            DWORD bytesWritten = 0;
            if (!WriteFile(
                    file,
                    buffer.data() + offset,
                    bytesRead - offset,
                    &bytesWritten,
                    nullptr) ||
                bytesWritten == 0)
            {
                success = false;
                break;
            }
            offset += bytesWritten;
        }
    }

    if (file != INVALID_HANDLE_VALUE)
        CloseHandle(file);

    if (!success && file != INVALID_HANDLE_VALUE)
        DeleteFileW(destination.c_str());

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return success;
}

int wmain(int argc, wchar_t* argv[])
{
    if (argc != 3)
    {
        std::wcerr << L"Usage: downloader.exe <HTTPS URL> <destination path>\n";
        return 2;
    }

    if (!DownloadHttpsFile(argv[1], argv[2]))
    {
        std::wcerr << L"Download failed. The file was not executed.\n";
        return 1;
    }

    std::wcout << L"Download completed. The file was saved but not executed.\n";
    return 0;
}