```c
#include <windows.h>
#include <urlmon.h>
#include <stdio.h>
#include <string.h>

#pragma comment(lib, "urlmon.lib")

static int is_http_url(const char *url)
{
    if (url == NULL) return 0;
    return _strnicmp(url, "https://", 8) == 0 || _strnicmp(url, "http://", 7) == 0;
}

static int confirm_execution(const char *url, const char *path)
{
    char answer[16];

    printf("Source URL : %s\n", url);
    printf("Local file : %s\n", path);
    printf("Type YES to execute this downloaded program: ");
    fflush(stdout);

    if (fgets(answer, sizeof(answer), stdin) == NULL) return 0;
    answer[strcspn(answer, "\r\n")] = '\0';
    return _stricmp(answer, "YES") == 0;
}

static int is_pe_file(const char *path)
{
    FILE *f = NULL;
    unsigned char dos[64];
    unsigned char sig[4];
    long pe_offset;

    f = fopen(path, "rb");
    if (f == NULL) return 0;

    if (fread(dos, 1, sizeof(dos), f) != sizeof(dos)) {
        fclose(f);
        return 0;
    }

    if (dos[0] != 'M' || dos[1] != 'Z') {
        fclose(f);
        return 0;
    }

    pe_offset = (long)dos[60] | ((long)dos[61] << 8) |
                ((long)dos[62] << 16) | ((long)dos[63] << 24);

    if (pe_offset <= 0 || fseek(f, pe_offset, SEEK_SET) != 0) {
        fclose(f);
        return 0;
    }

    if (fread(sig, 1, sizeof(sig), f) != sizeof(sig)) {
        fclose(f);
        return 0;
    }

    fclose(f);
    return sig[0] == 'P' && sig[1] == 'E' && sig[2] == 0 && sig[3] == 0;
}

static int download_file(const char *url, const char *dest_path)
{
    HRESULT hr = URLDownloadToFileA(NULL, url, dest_path, 0, NULL);
    if (FAILED(hr)) {
        fprintf(stderr, "Download failed (HRESULT 0x%08lX).\n", (unsigned long)hr);
        return 0;
    }
    return 1;
}

static int build_local_path(char *out, size_t out_size)
{
    char temp_dir[MAX_PATH];
    DWORD len = GetTempPathA(sizeof(temp_dir), temp_dir);

    if (len == 0 || len >= sizeof(temp_dir)) return 0;

    _snprintf(out, out_size, "%sdownloaded_%lu.exe",
              temp_dir, (unsigned long)GetTickCount());
    out[out_size - 1] = '\0';
    return 1;
}

static int execute_program(const char *path)
{
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    char cmdline[MAX_PATH * 2];

    memset(&si, 0, sizeof(si));
    memset(&pi, 0, sizeof(pi));
    si.cb = sizeof(si);

    _snprintf(cmdline, sizeof(cmdline), "\"%s\"", path);
    cmdline[sizeof(cmdline) - 1] = '\0';

    if (!CreateProcessA(path, cmdline, NULL, NULL, FALSE, 0,
                        NULL, NULL, &si, &pi)) {
        fprintf(stderr, "Failed to execute program (error %lu).\n",
                (unsigned long)GetLastError());
        return 0;
    }

    printf("Program started (PID %lu). Waiting for exit...\n",
           (unsigned long)pi.dwProcessId);

    WaitForSingleObject(pi.hProcess, INFINITE);

    {
        DWORD exit_code = 0;
        if (GetExitCodeProcess(pi.hProcess, &exit_code)) {
            printf("Program exited with code %lu.\n", (unsigned long)exit_code);
        }
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 1;
}

int main(int argc, char *argv[])
{
    char local_path[MAX_PATH];

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <http(s)://url/to/program.exe>\n",
                argc > 0 ? argv[0] : "downloader");
        return 1;
    }

    if (!is_http_url(argv[1])) {
        fprintf(stderr, "Error: only http:// and https:// URLs are accepted.\n");
        return 1;
    }

    if (!build_local_path(local_path, sizeof(local_path))) {
        fprintf(stderr, "Error: could not determine temporary directory.\n");
        return 1;
    }

    printf("Downloading...\n");
    if (!download_file(argv[1], local_path)) {
        DeleteFileA(local_path);
        return 1;
    }

    if (!is_pe_file(local_path)) {
        fprintf(stderr, "Error: downloaded file is not a valid Windows EXE.\n");
        DeleteFileA(local_path);
        return 1;
    }

    if (!confirm_execution(argv[1], local_path)) {
        printf("Execution cancelled by user.\n");
        DeleteFileA(local_path);
        return 0;
    }

    if (!execute_program(local_path)) {
        DeleteFileA(local_path);
        return 1;
    }

    return 0;
}
```