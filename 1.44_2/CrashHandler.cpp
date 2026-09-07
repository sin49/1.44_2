#include "CrashHandler.h"
#include <string>
#include <filesystem>
#include <ctime>

namespace CrashHandler {

    static std::wstring GetDumpFilePath(const wchar_t* prefix) {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        std::filesystem::path basePath = exePath;
        std::filesystem::path dir = basePath.parent_path() / L"Dumps";

        // Dumps 디렉터리가 없으면 자동 생성
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);

        // 타임스탬프 생성 (YYYY-MM-DD_HH-MM-SS)
        time_t now = time(nullptr);
        tm localTm;
        localtime_s(&localTm, &now);

        wchar_t timeStr[64];
        wcsftime(timeStr, sizeof(timeStr) / sizeof(wchar_t), L"%Y-%m-%d_%H-%M-%S", &localTm);

        std::wstring filename = (prefix ? prefix : L"CrashDump_");
        filename += timeStr;
        filename += L".dmp";

        return (dir / filename).wstring();
    }

    static LONG WINAPI UnhandledCrashFilter(EXCEPTION_POINTERS* pExceptionPointers) {
        std::wstring dumpPath = GetDumpFilePath(L"CrashDump_");

        HANDLE hFile = CreateFileW(
            dumpPath.c_str(),
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (hFile != INVALID_HANDLE_VALUE) {
            MINIDUMP_EXCEPTION_INFORMATION expInfo;
            expInfo.ThreadId = GetCurrentThreadId();
            expInfo.ExceptionPointers = pExceptionPointers;
            expInfo.ClientPointers = FALSE;

            // 크래시 원인을 완벽 분석할 수 있도록 풍부한 정보 캡처
            MINIDUMP_TYPE dumpType = (MINIDUMP_TYPE)(
                MiniDumpWithDataSegs |
                MiniDumpWithIndirectlyReferencedMemory |
                MiniDumpWithThreadInfo |
                MiniDumpWithHandleData |
                MiniDumpWithUnloadedModules
            );

            BOOL success = MiniDumpWriteDump(
                GetCurrentProcess(),
                GetCurrentProcessId(),
                hFile,
                dumpType,
                &expInfo,
                nullptr,
                nullptr
            );

            CloseHandle(hFile);

            if (success) {
                wchar_t msg[1024];
                swprintf_s(msg, 1024,
                    L"프로그램 실행 중 예외가 발생하여 크래시 덤프 파일이 생성되었습니다.\n\n"
                    L"📁 덤프 파일 위치:\n%s\n\n"
                    L"Visual Studio에서 위 .dmp 파일을 열면 크래시 발생 지점과 콜스택을 즉시 확인할 수 있습니다.",
                    dumpPath.c_str()
                );
                MessageBoxW(nullptr, msg, L"1.44MB Multi-Game - Crash Dump Saved", MB_OK | MB_ICONERROR);
            }
        }

        return EXCEPTION_EXECUTE_HANDLER;
    }

    void Initialize() {
        // 미처리 예외 필터 등록
        SetUnhandledExceptionFilter(UnhandledCrashFilter);

        // CRT 잘못된 매개변수 핸들러
        _set_invalid_parameter_handler([](const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t) {
            RaiseException(0xE0000001, EXCEPTION_NONCONTINUABLE, 0, nullptr);
        });

        // CRT 순수 가상 함수 호출 핸들러
        _set_purecall_handler([]() {
            RaiseException(0xE0000002, EXCEPTION_NONCONTINUABLE, 0, nullptr);
        });
    }

    bool CreateManualDump(const wchar_t* customPrefix) {
        std::wstring dumpPath = GetDumpFilePath(customPrefix ? customPrefix : L"ManualDump_");

        HANDLE hFile = CreateFileW(
            dumpPath.c_str(),
            GENERIC_WRITE,
            0,
            nullptr,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr
        );

        if (hFile == INVALID_HANDLE_VALUE) return false;

        MINIDUMP_TYPE dumpType = (MINIDUMP_TYPE)(
            MiniDumpWithDataSegs |
            MiniDumpWithIndirectlyReferencedMemory |
            MiniDumpWithThreadInfo |
            MiniDumpWithHandleData
        );

        BOOL success = MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hFile,
            dumpType,
            nullptr,
            nullptr,
            nullptr
        );

        CloseHandle(hFile);
        return (success == TRUE);
    }
}
