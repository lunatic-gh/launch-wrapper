#include <windows.h>
#include <sstream>
#include <vector>
#include <iostream>

void set_env(const char* key, const char* value) {
  _putenv_s(key, value);
}

BOOL exec_command(const std::wstring& cmd, DWORD& code) {
  PROCESS_INFORMATION processInformation = {nullptr};
  STARTUPINFO startupInfo = {0};
  startupInfo.cb = sizeof(startupInfo);

  std::vector<wchar_t> cmdLineBuffer(cmd.begin(), cmd.end());
  cmdLineBuffer.push_back(L'\0');

  BOOL result = CreateProcessW(nullptr, cmdLineBuffer.data(), nullptr, nullptr, FALSE, NORMAL_PRIORITY_CLASS, nullptr, nullptr,
                               reinterpret_cast<LPSTARTUPINFOW>(&startupInfo), &processInformation);
  if (!result) {
    LPVOID lpMsgBuf;
    const DWORD dw = GetLastError();
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, dw,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPWSTR>(&lpMsgBuf), 0, nullptr);
    std::wcerr << L"::executeCommandLine() failed at CreateProcess()\n"
               << L"Command=" << cmd << L"\n"
               << L"Message=" << static_cast<LPWSTR>(lpMsgBuf) << L"\n\n";
    LocalFree(lpMsgBuf);
    return FALSE;
  }
  WaitForSingleObject(processInformation.hProcess, INFINITE);
  result = GetExitCodeProcess(processInformation.hProcess, &code);
  CloseHandle(processInformation.hProcess);
  CloseHandle(processInformation.hThread);
  if (!result) {
    std::wcerr << L"Executed command but couldn't get exit code.\nCommand=" << cmd << L"\n";
    return FALSE;
  }
  return TRUE;
}

std::wstring utf8_to_wstring(const std::string& str) {
  if (str.empty()) {
    return std::wstring();
  }
  const int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);
  if (size_needed == 0) {
    return std::wstring();
  }
  std::wstring w_str(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), &w_str[0], size_needed);
  return w_str;
}

int main(const int argc, const char* argv[]) {
  set_env("QT_QPA_PLATFORM_PLUGIN_PATH", "./");

  if (argc < 1) {
    return -1;
  }

  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " ENV_VAR=VALUE ENV_VAR_2=VALUE_2 ... -- program --program-arg-1 --program-arg-2 ..." << std::endl;
    return 1;
  }

  int i = 1;
  for (; i < argc; ++i) {
    if (std::string(argv[i]) == "--") {
      ++i;
      break;
    }
    std::string arg(argv[i]);
    const size_t pos = arg.find('=');
    if (pos == std::string::npos) {
      std::cerr << "Invalid env var format (expected key=value): " << arg << std::endl;
      return 1;
    }
    std::string key = arg.substr(0, pos);
    std::string value = arg.substr(pos + 1);
    set_env(key.c_str(), value.c_str());
  }
  if (i >= argc) {
    std::cerr << "No command specified after '--'" << std::endl;
    return 1;
  }
  std::wstringstream cmdLineStream;
  for (; i < argc; ++i) {
    if (std::string arg = argv[i]; arg.find(' ') != std::string::npos || arg.find('\t') != std::string::npos) {
      cmdLineStream << L'"' << utf8_to_wstring(arg) << L'"';
    } else {
      cmdLineStream << utf8_to_wstring(arg);
    }
    if (i + 1 < argc) cmdLineStream << L' ';
  }

  const std::wstring cmdLine = cmdLineStream.str();

  DWORD exitCode = 0;

  if (const BOOL success = exec_command(cmdLine, exitCode); !success) {
    std::cerr << "Failed to execute command." << std::endl;
    return 1;
  }
  return static_cast<int>(exitCode);
}
