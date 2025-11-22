// DirectoryScanner.h

#pragma once

// MFC 프로젝트에서는 대부분 pch.h가 먼저 include 되기 때문에
// CString, WIN32 API 타입들을 별도로 include 하지 않아도 된다.
// 그래도 안전하게 쓰고 싶으면 pch.h 안에 <afxwin.h> 포함되어 있는지 확인하면 된다.

// UI 업데이트용 사용자 정의 메시지
constexpr UINT WM_SCAN_UPDATE = WM_APP + 1;  // 워커 스레드 → UI 업데이트 메시지
constexpr UINT WM_SCAN_FINISHED = WM_APP + 2;  // 워커 스레드 → 탐색 종료/중단 알림 메시지

// 워커 스레드가 UI에 전달할 정보 구조체
// (기존 DirSizeVisualizerDlg.h 안에 있던 것 그대로 옮긴 것)
struct ScanUpdateInfo
{
    CString   name;  // 파일/폴더 이름
    CString   type;  // "파일" / "폴더"
    CString   path;  // 전체 경로
    ULONGLONG size;  // 바이트 단위 크기
};

// 디렉토리 탐색 전용 로직을 담당하는 클래스
class DirectoryScanner
{
public:
    DirectoryScanner();

    // 새 스캔 시작
    //  - rootPath   : 탐색 시작 루트 경로
    //  - hNotifyWnd : UI 윈도우 핸들 (여기로 WM_SCAN_UPDATE / WM_SCAN_FINISHED를 보냄)
    void StartScan(const CString& rootPath, HWND hNotifyWnd);

    // 중단 요청 (UI 스레드에서 호출)
    void RequestStop();

    // 현재 stop 요청 상태
    bool IsStopRequested() const { return m_bStopRequested; }

    // 전체 파일 크기 합계
    ULONGLONG GetTotalSize() const { return m_totalSize; }

    // 처리한 항목 수 (ProgressBar 계산 등에 사용)
    LONG GetItemCount() const { return m_itemCount; }

private:
    void ScanDirectory(const CString& folderPath, HWND hNotifyWnd);

    static bool IsDotOrDotDot(const wchar_t* name);
    static ULONGLONG GetFileSizeULL(const WIN32_FIND_DATAW& fd);

private:
    bool       m_bStopRequested = false; // 스캔 중단 요청 플래그
    ULONGLONG  m_totalSize = 0;     // 전체 파일 크기 누적 (Bytes)
    LONG       m_itemCount = 0;     // 처리한 항목 수 (샘플링/프로그레스용)
};
