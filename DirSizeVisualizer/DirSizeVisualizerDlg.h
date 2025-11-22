// DirSizeVisualizerDlg.h: 헤더 파일
//

#pragma once

#include <vector>            // std::vector
#include "DirectoryScanner.h"// DirectoryScanner, ScanUpdateInfo, WM_SCAN_* 정의

// CDirSizeVisualizerDlg 대화 상자
class CDirSizeVisualizerDlg : public CDialogEx
{
    // 생성입니다.
public:
    CDirSizeVisualizerDlg(CWnd* pParent = nullptr); // 표준 생성자입니다.

    // 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DIRSIZEVISUALIZER_DIALOG };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

    // 구현입니다.
protected:
    HICON m_hIcon;

    // 생성된 메시지 맵 함수
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();

    // 워커 스레드에서 보내는 사용자 정의 메시지 처리 함수
    afx_msg LRESULT OnScanUpdate(WPARAM wParam, LPARAM lParam);     // 파일/폴더 정보 수신 후 UI 갱신
    afx_msg LRESULT OnScanFinished(WPARAM wParam, LPARAM lParam);   // 탐색 완료/중단 시 UI 상태 변경

    // 리스트뷰 컬럼 헤더 클릭 시 정렬 처리 함수
    afx_msg void OnLvnColumnclickListFiles(NMHDR* pNMHDR, LRESULT* pResult);

    DECLARE_MESSAGE_MAP()

public:
    afx_msg void OnEnChangeEditPath();
    afx_msg void OnBnClickedBtnStart();
    afx_msg void OnBnClickedCancel();
    afx_msg void OnBnClickedBtnStop();
    afx_msg void OnLvnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg void OnStnClickedStaticStatus();
    afx_msg void OnStnClickedStaticPathLabel();

    CEdit         m_editPath;
    CButton       m_btnStart;
    CButton       m_btnStop;
    CProgressCtrl m_progress;
    CListCtrl     m_list;
    CStatic       m_staticStatus;
    CStatic       m_staticPathLabel;

private:
    // 멀티스레드 탐색을 위한 멤버 변수
    CWinThread* m_pScanThread = nullptr;  // 워커 스레드 포인터
    CString          m_strRootPath;           // 탐색 시작 루트 경로

    // 디렉토리 탐색 로직을 담당하는 스캐너
    DirectoryScanner m_scanner;

    // 리스트 정렬 관련 멤버 변수
    BOOL                    m_bSizeSortAscending = TRUE;   // 크기 정렬 방향 (TRUE = 오름차순, FALSE = 내림차순)
    std::vector<ULONGLONG>  m_itemSizes;                   // 각 항목의 크기(바이트)를 저장하는 배열

    // 크기 비교를 위한 정렬 콜백 함수
    static int CALLBACK CompareBySize(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

    // 워커 스레드 진입점
    static UINT ScanThreadProc(LPVOID pParam);
};
