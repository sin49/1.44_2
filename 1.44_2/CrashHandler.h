#pragma once
#include <windows.h>
#include <DbgHelp.h>

#pragma comment(lib, "DbgHelp.lib")

namespace CrashHandler {
    // 크래시 덤프 핸들러 초기화 (프로그램 시작 시 wWinMain에서 호출)
    void Initialize();

    // 사용자가 원할 때 수동으로 현재 시점의 메모리 덤프 파일을 생성하는 함수
    bool CreateManualDump(const wchar_t* customPrefix = nullptr);
}
