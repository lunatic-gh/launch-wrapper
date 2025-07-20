#include <algorithm>
#include <windows.h>
#include <sstream>
#include <vector>
#include <iostream>

void set_env(const char* key, const char* value) {
    _putenv_s(key, value);
}

BOOL exec_command(const std::wstring& cmd, DWORD& code) {
    PROCESS_INFORMATION processInformation = { nullptr };
    STARTUPINFO startupInfo = { 0 };
    startupInfo.cb = sizeof(startupInfo);

    std::vector<wchar_t> cmdLineBuffer(cmd.begin(), cmd.end());
    cmdLineBuffer.push_back(L'\0');

    BOOL result = CreateProcessW(
        nullptr,
        cmdLineBuffer.data(),
        nullptr,
        nullptr,
        FALSE,
        NORMAL_PRIORITY_CLASS,
        nullptr,
        nullptr,
        reinterpret_cast<LPSTARTUPINFOW>(&startupInfo),
        &processInformation
    );
    if (!result) {
        LPVOID lpMsgBuf;
        const DWORD dw = GetLastError();
        FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, dw,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPWSTR>(&lpMsgBuf),
            0, nullptr
        );

        std::wstringstream msg;
        msg << L"CreateProcess failed!\nCommand: " << cmd
            << L"\nError: " << static_cast<LPWSTR>(lpMsgBuf);

        LocalFree(lpMsgBuf);
        return FALSE;
    }

    WaitForSingleObject(processInformation.hProcess, INFINITE);
    result = GetExitCodeProcess(processInformation.hProcess, &code);
    CloseHandle(processInformation.hProcess);
    CloseHandle(processInformation.hThread);

    if (!result) {
        std::wcerr << L"Executed command but couldn't get exit code.\nCommand="
                   << cmd << L"\n";
        return FALSE;
    }
    return TRUE;
}

std::wstring utf8_to_wstring(const std::string& str) {
    if (str.empty()) {
        return std::wstring();
    }
    const int size_needed = MultiByteToWideChar(
        CP_UTF8, 0,
        str.data(), static_cast<int>(str.size()),
        nullptr, 0
    );
    if (size_needed == 0) {
        return std::wstring();
    }

    std::wstring w_str(size_needed, 0);
    MultiByteToWideChar(
        CP_UTF8, 0,
        str.data(), static_cast<int>(str.size()),
        &w_str[0], size_needed
    );
    return w_str;
}

std::string trim(const std::string& s) {
    auto start = std::find_if_not(s.begin(), s.end(), [](unsigned char ch) {
        return std::isspace(ch);
    });
    auto end = std::find_if_not(s.rbegin(), s.rend(), [](unsigned char ch) {
        return std::isspace(ch);
    }).base();
    if (start >= end) return "";
    return std::string(start, end);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    int argc;
    LPWSTR* argvW = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (!argvW) {
        return 1;
    }

    // Convert wide argv to UTF-8 std::string without the trailing NUL
    std::vector<std::string> argv(argc);
    for (int i = 0; i < argc; ++i) {
        // bytes needed including the terminating NUL
        int needed = WideCharToMultiByte(
            CP_UTF8, 0,
            argvW[i], -1,
            nullptr, 0,
            nullptr, nullptr
        );
        if (needed > 0) {
            // allocate only the actual chars (drop the trailing '\0')
            argv[i].resize(needed - 1);
            WideCharToMultiByte(
                CP_UTF8, 0,
                argvW[i], -1,
                argv[i].data(), needed - 1,
                nullptr, nullptr
            );
        }
    }
    LocalFree(argvW);

    if (argc < 2) {
        return 1;
    }

    int idx = 1;
    for (; idx < argc; ++idx) {
        const auto& arg = argv[idx];
        if (arg == "--") {
            ++idx;
            break;
        }

        auto pos = arg.find('=');
        if (pos == std::string::npos) {
            std::cerr << "Invalid env var format (expected key=value): "
                      << arg << std::endl;
            return 1;
        }
        std::string key = arg.substr(0, pos);
        std::string value = arg.substr(pos + 1);
        set_env(key.c_str(), value.c_str());
    }

    if (idx >= argc) {
        return 1;
    }

    // build the command line
    std::wstringstream cmdLineStream;
    for (; idx < argc; ++idx) {
        std::wstring warg = utf8_to_wstring(argv[idx]);
        bool needsQuotes = (warg.find(L' ') != std::wstring::npos
                            || warg.find(L'\t') != std::wstring::npos);
        if (needsQuotes) {
            cmdLineStream << L'"' << warg << L'"';
        } else {
            cmdLineStream << warg;
        }
        if (idx + 1 < argc) {
            cmdLineStream << L' ';
        }
    }

    std::wstring cmdLine = cmdLineStream.str();
    DWORD exitCode = 0;
    if (!exec_command(cmdLine, exitCode)) {
        return 1;
    }
    return static_cast<int>(exitCode);
}