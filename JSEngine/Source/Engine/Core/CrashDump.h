#pragma once

#include <Windows.h>

// SEH 필터 함수: __except() 안에서 GetExceptionInformation()과 함께 사용
// 크래시 발생 시 실행 파일 옆에 .dmp 파일을 생성합니다.
LONG WINAPI WriteCrashDump(EXCEPTION_POINTERS* ExceptionInfo);
void WriteCrashLog(EXCEPTION_POINTERS* ExceptionInfo);
int ReportCrash(EXCEPTION_POINTERS* ExceptionInfo);

struct FCrashHandler
{
    // 프로세스 전역 Unhandled Exception Filter를 등록
    static void Initialize();

    // OS가 감지한 미처리 예외에 대해 호출되는 최상위 SEH 콜백
    static LONG WINAPI handleException(EXCEPTION_POINTERS* ExceptionInfo);

    static void WriteCrashDump(EXCEPTION_POINTERS* ExceptionInfo);

	static void WriteCrashLog(EXCEPTION_POINTERS* ExceptionInfo);

    // 다음 크래시 발생 시 압축 덤프를 캡처하도록 구성
    // 내용 : 기본 스택 + 예외 정보 + 데이터 세그먼트만 포함
    //static void SetNextDumpType(MINIDUMP_TYPE InDumpType);
};