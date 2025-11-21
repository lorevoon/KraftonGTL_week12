#pragma once
#include <Windows.h>

/**
 * 크래시 발생 시 MiniDump를 생성하는 플랫폼별 유틸리티
 */
struct FPlatformCrashHandler
{
    /**
     * 크래시 핸들러를 초기화하고 전역 예외 필터를 등록합니다.
     * 프로그램 시작 시 한 번만 호출해야 합니다.
     */
    static void InitializeCrashHandler();

    /**
     * 현재 시점의 MiniDump를 생성합니다.
     * 크래시 없이 수동으로 덤프를 생성할 때 사용합니다.
     * @return 덤프 생성 성공 여부
     */
    static bool GenerateMiniDump();

    /**
     * 랜덤한 위치에서 크래시를 발생시킵니다.
     * 실행 중인 UObject를 랜덤하게 삭제하여 나중에 접근 시 크래시가 발생하도록 합니다.
     */
    static void CauseIntentionalCrash();

    /**
     * 매 프레임마다 랜덤한 UObject를 삭제합니다.
     * CauseIntentionalCrash가 호출되면 활성화됩니다.
     */
    static void TickCrashMode();

    /**
     * 크래시 모드 활성화 여부
     */
    static bool bContinuousCrashMode;

private:
    /**
     * 처리되지 않은 예외 발생 시 호출되는 필터 함수
     * @param ExceptionInfo 예외 정보 구조체
     * @return 예외 처리 결과
     */
    static LONG WINAPI UnhandledExceptionFilter(EXCEPTION_POINTERS* ExceptionInfo);

    /**
     * MiniDump 파일을 생성하는 내부 함수
     * @param ExceptionInfo 예외 정보 (nullptr이면 현재 스레드 정보 사용)
     * @param FilePath 덤프 파일 경로
     * @return 덤프 생성 성공 여부
     */
    static bool WriteMiniDump(EXCEPTION_POINTERS* ExceptionInfo, const wchar_t* FilePath);

    /**
     * 현재 시간을 기반으로 덤프 파일 경로를 생성합니다.
     * 예: "Mundi_Crash_2025-11-16_14-30-25.dmp"
     */
    static void GetDumpFilePath(wchar_t* OutPath, size_t PathSize);
};
