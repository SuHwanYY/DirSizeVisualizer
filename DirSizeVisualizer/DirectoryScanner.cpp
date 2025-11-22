// DirectoryScanner.cpp

#include "pch.h"              // MFC 기본 pch
#include "DirectoryScanner.h"
#include <cmath>              // log10 사용

// 대규모 탐색 시 UI 부하를 줄이기 위한 상수들 (샘플링용)
constexpr LONG kEarlyFullUpdateCount = 500;   // 처음 500개 항목은 모두 UI에 표시
constexpr LONG kUpdateInterval = 500;   // 이후에는 500개마다 한 번씩만 UI 업데이트

DirectoryScanner::DirectoryScanner()
{
    m_bStopRequested = false;
    m_totalSize = 0;
    m_itemCount = 0;
}

void DirectoryScanner::RequestStop()
{
    m_bStopRequested = true;
}

bool DirectoryScanner::IsDotOrDotDot(const wchar_t* name)
{
    return (wcscmp(name, L".") == 0 || wcscmp(name, L"..") == 0);
}

ULONGLONG DirectoryScanner::GetFileSizeULL(const WIN32_FIND_DATAW& fd)
{
    return (static_cast<ULONGLONG>(fd.nFileSizeHigh) << 32) |
        static_cast<ULONGLONG>(fd.nFileSizeLow);
}

void DirectoryScanner::StartScan(const CString& rootPath, HWND hNotifyWnd)
{
    // 새 탐색 시작 시 상태 초기화
    m_bStopRequested = false;
    m_totalSize = 0;
    m_itemCount = 0;

    // 실제 디렉터리 탐색
    ScanDirectory(rootPath, hNotifyWnd);

    // 탐색이 끝났거나, 중단 요청으로 종료되었을 때 UI에 알림
    ::PostMessage(hNotifyWnd, WM_SCAN_FINISHED, 0, 0);
}

void DirectoryScanner::ScanDirectory(const CString& folderPath, HWND hNotifyWnd)
{
    if (m_bStopRequested)
    {
        return;
    }

    // 검색 패턴 (예: "C:\폴더\*")
    CString searchPath = folderPath;
    if (searchPath.Right(1) != L"\\")
    {
        searchPath += L"\\";
    }
    searchPath += L"*";

    WIN32_FIND_DATAW fd;
    HANDLE hFind = ::FindFirstFileW(searchPath, &fd);
    if (hFind == INVALID_HANDLE_VALUE)
    {
        return;
    }

    do
    {
        if (m_bStopRequested)
        {
            break;  // 중단 요청 시 루프 탈출
        }

        CString name = fd.cFileName;

        // "." 및 ".." 는 무시
        if (IsDotOrDotDot(name.GetString()))
        {
            continue;
        }

        // 전체 경로 조합
        CString fullPath = folderPath;
        if (fullPath.Right(1) != L"\\")
        {
            fullPath += L"\\";
        }
        fullPath += name;

        // 재분석 포인트(심볼릭 링크/정션)는 재귀 안 들어감 (무한루프 방지)
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
        {
            continue;
        }

        const bool isDirectory = (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        // 파일인 경우 전체 크기 누적
        if (!isDirectory)
        {
            ULONGLONG fileSize = GetFileSizeULL(fd);
            m_totalSize += fileSize;   // 전체 파일 크기 합산
        }

        // 파일/폴더 처리 개수 카운트 (UI 업데이트 샘플링용)
        ++m_itemCount;

        // UI 업데이트 샘플링:
        //  - 처음 kEarlyFullUpdateCount개까지는 모두 UI에 전달
        //  - 이후에는 kUpdateInterval개마다 한 번씩만 UI에 전달
        bool needUpdate =
            (m_itemCount <= kEarlyFullUpdateCount) ||
            (m_itemCount % kUpdateInterval == 0);

        if (needUpdate)
        {
            // UI 스레드에 넘길 정보 구조체 동적 할당 (UI에서 delete 해줌)
            ScanUpdateInfo* pInfo = new ScanUpdateInfo;
            pInfo->name = name;
            pInfo->type = isDirectory ? L"폴더" : L"파일";
            pInfo->path = fullPath;
            pInfo->size = isDirectory ? 0 : GetFileSizeULL(fd);

            ::PostMessage(hNotifyWnd, WM_SCAN_UPDATE, 0,
                reinterpret_cast<LPARAM>(pInfo));
        }

        // 하위 폴더 재귀 탐색
        if (isDirectory)
        {
            ScanDirectory(fullPath, hNotifyWnd);
        }

    } while (::FindNextFileW(hFind, &fd));

    ::FindClose(hFind);
}
