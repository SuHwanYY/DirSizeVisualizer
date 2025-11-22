// DirSizeVisualizerDlg.cpp: 구현 파일
//

#include "pch.h"
#include "framework.h"
#include "DirSizeVisualizer.h"
#include "DirSizeVisualizerDlg.h"
#include "afxdialogex.h"
#include <cmath>   // 진행률 계산에 log10 사용

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 리스트뷰에 표시할 최대 항목 수
constexpr LONG kMaxListItems = 5000;  // 리스트뷰에 최대 5000개까지만 추가


// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.

class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();

#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif

protected:
    virtual void DoDataExchange(CDataExchange* pDX);

protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CDirSizeVisualizerDlg 대화 상자

CDirSizeVisualizerDlg::CDirSizeVisualizerDlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_DIRSIZEVISUALIZER_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

    // 크기 정렬 방향 기본값 (오름차순)
    m_bSizeSortAscending = TRUE;
}

void CDirSizeVisualizerDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_PATH, m_editPath);
    DDX_Control(pDX, IDC_BTN_START, m_btnStart);
    DDX_Control(pDX, IDC_BTN_STOP, m_btnStop);
    DDX_Control(pDX, IDC_PROGRESS, m_progress);
    DDX_Control(pDX, IDC_LIST_FILES, m_list);
    DDX_Control(pDX, IDC_STATIC_STATUS, m_staticStatus);
    DDX_Control(pDX, IDC_STATIC_PATH_LABEL, m_staticPathLabel);
}

BEGIN_MESSAGE_MAP(CDirSizeVisualizerDlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()

    ON_EN_CHANGE(IDC_EDIT_PATH, &CDirSizeVisualizerDlg::OnEnChangeEditPath)
    ON_BN_CLICKED(IDC_BTN_START, &CDirSizeVisualizerDlg::OnBnClickedBtnStart)
    ON_BN_CLICKED(IDCANCEL, &CDirSizeVisualizerDlg::OnBnClickedCancel)
    ON_BN_CLICKED(IDC_BTN_STOP, &CDirSizeVisualizerDlg::OnBnClickedBtnStop)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_FILES, &CDirSizeVisualizerDlg::OnLvnItemchangedList1)
    ON_STN_CLICKED(IDC_STATIC_STATUS, &CDirSizeVisualizerDlg::OnStnClickedStaticStatus)
    ON_STN_CLICKED(IDC_STATIC_PATH_LABEL, &CDirSizeVisualizerDlg::OnStnClickedStaticPathLabel)

    // 리스트뷰 컬럼 헤더 클릭 시 정렬
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST_FILES, &CDirSizeVisualizerDlg::OnLvnColumnclickListFiles)

    // 워커 스레드 → UI 사용자 정의 메시지
    ON_MESSAGE(WM_SCAN_UPDATE, &CDirSizeVisualizerDlg::OnScanUpdate)
    ON_MESSAGE(WM_SCAN_FINISHED, &CDirSizeVisualizerDlg::OnScanFinished)
END_MESSAGE_MAP()


BOOL CDirSizeVisualizerDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // 시스템 메뉴에 "정보..." 메뉴 항목 추가
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != nullptr)
    {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(bNameValid);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    // Edit 기본 경로
    m_editPath.SetWindowTextW(L"C:\\");

    // 버튼 텍스트 + 초기 활성 상태
    m_btnStart.SetWindowTextW(L"시작");
    m_btnStop.SetWindowTextW(L"중지");
    m_btnStart.EnableWindow(TRUE);
    m_btnStop.EnableWindow(FALSE);

    // 리스트뷰 스타일 + 컬럼 구성
    m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    m_list.InsertColumn(0, L"이름", LVCFMT_LEFT, 250);
    m_list.InsertColumn(1, L"유형", LVCFMT_LEFT, 80);
    m_list.InsertColumn(2, L"크기(Bytes)", LVCFMT_RIGHT, 120);
    m_list.InsertColumn(3, L"경로", LVCFMT_LEFT, 350);

    // ProgressBar 기본 설정
    m_progress.SetRange(0, 100);
    m_progress.SetPos(0);

    // 상태 텍스트 기본값
    m_staticStatus.SetWindowTextW(L"대기 중...");

    // 탐색 경로 라벨 기본 텍스트
    m_staticPathLabel.SetWindowTextW(L"탐색 경로:");

    return TRUE;
}

void CDirSizeVisualizerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialogEx::OnSysCommand(nID, lParam);
    }
}

void CDirSizeVisualizerDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this);

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }
}

HCURSOR CDirSizeVisualizerDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}


void CDirSizeVisualizerDlg::OnEnChangeEditPath()
{
    // 경로 Edit 변경 시 추가 동작이 필요하면 여기에 작성
}

void CDirSizeVisualizerDlg::OnBnClickedBtnStart()
{
    CString path;
    m_editPath.GetWindowTextW(path);

    if (path.IsEmpty())
    {
        AfxMessageBox(L"경로를 입력하세요.");
        return;
    }

    // 이미 스레드가 돌고 있다면 중복 실행 방지
    if (m_pScanThread != nullptr)
    {
        return;
    }

    // 탐색 상태 초기화
    m_strRootPath = path;

    // 정렬용 크기 배열 초기화
    m_itemSizes.clear();

    // 버튼 상태 토글
    m_btnStart.EnableWindow(FALSE);
    m_btnStop.EnableWindow(TRUE);

    // 리스트 및 ProgressBar 초기화
    m_list.DeleteAllItems();
    m_progress.SetPos(0);

    m_staticStatus.SetWindowTextW(L"스캔 준비 중...");

    // 워커 스레드 시작
    m_staticStatus.SetWindowTextW(L"스캔 중...");
    m_pScanThread = AfxBeginThread(ScanThreadProc, this);
    if (m_pScanThread == nullptr)
    {
        m_staticStatus.SetWindowTextW(L"스레드 생성에 실패했습니다.");
        m_btnStart.EnableWindow(TRUE);
        m_btnStop.EnableWindow(FALSE);
    }
}

void CDirSizeVisualizerDlg::OnBnClickedCancel()
{
    CDialogEx::OnCancel();
}

void CDirSizeVisualizerDlg::OnBnClickedBtnStop()
{
    if (m_pScanThread != nullptr)
    {
        // 스캐너에게 중단 요청
        m_scanner.RequestStop();
        m_staticStatus.SetWindowTextW(L"중지 요청됨...");
    }
    else
    {
        m_staticStatus.SetWindowTextW(L"현재 실행 중인 스캔이 없습니다.");
    }

    // 버튼 상태는 실제로 탐색이 끝난 후(OnScanFinished)에서 원복
}

void CDirSizeVisualizerDlg::OnLvnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    UNREFERENCED_PARAMETER(pNMLV);
    // 리스트 항목 선택 변경 시 필요한 동작이 있으면 여기에 작성
    *pResult = 0;
}

void CDirSizeVisualizerDlg::OnStnClickedStaticStatus()
{
    // 상태 텍스트 클릭 시 필요한 동작이 있으면 여기에 작성
}

void CDirSizeVisualizerDlg::OnStnClickedStaticPathLabel()
{
    // 탐색 경로 라벨 클릭 시 필요한 동작이 있으면 여기에 작성
}

// 워커 스레드 진입 함수
UINT CDirSizeVisualizerDlg::ScanThreadProc(LPVOID pParam)
{
    CDirSizeVisualizerDlg* pDlg = reinterpret_cast<CDirSizeVisualizerDlg*>(pParam);
    if (pDlg == nullptr)
    {
        return 0;
    }

    // DirectoryScanner를 사용해 실제 디렉터리 탐색 수행
    //  - 스캔 중 발견되는 항목은 WM_SCAN_UPDATE
    //  - 완료/중단 시 WM_SCAN_FINISHED 메시지가 자동으로 날아감
    pDlg->m_scanner.StartScan(pDlg->m_strRootPath, pDlg->m_hWnd);

    return 0;
}

// 워커 스레드에서 전달한 ScanUpdateInfo를 이용해 UI를 갱신하는 함수
LRESULT CDirSizeVisualizerDlg::OnScanUpdate(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);

    ScanUpdateInfo* pInfo = reinterpret_cast<ScanUpdateInfo*>(lParam);
    if (pInfo == nullptr)
    {
        return 0;
    }

    // 리스트뷰 항목 수가 너무 많아지지 않도록 최대 개수 제한
    int currentItems = m_list.GetItemCount();
    if (currentItems >= kMaxListItems)
    {
        // 리스트에는 더 이상 추가하지 않고, 탐색 경로 라벨만 갱신
        CString pathLabelText;
        pathLabelText.Format(L"탐색 경로: %s", pInfo->path.GetString());
        m_staticPathLabel.SetWindowTextW(pathLabelText);

        delete pInfo;
        return 0;
    }

    // 리스트뷰에 항목 추가
    int index = m_list.InsertItem(m_list.GetItemCount(), pInfo->name); // [0열] 이름
    m_list.SetItemText(index, 1, pInfo->type);                         // [1열] 유형

    // 크기 컬럼
    CString sizeStr;
    if (pInfo->size == 0 && pInfo->type == L"폴더")
    {
        sizeStr = L"-"; // 폴더는 크기를 "-"로 표시
    }
    else
    {
        sizeStr.Format(L"%llu", pInfo->size);
    }
    m_list.SetItemText(index, 2, sizeStr);                             // [2열] 크기
    m_list.SetItemText(index, 3, pInfo->path);                         // [3열] 전체 경로

    // 정렬에서 사용할 크기 정보 저장 및 ItemData 설정
    size_t logicalId = m_itemSizes.size();
    m_itemSizes.push_back(pInfo->size);                                // 폴더는 0, 파일은 실제 크기
    m_list.SetItemData(index, static_cast<DWORD_PTR>(logicalId));

    // 현재 탐색 경로 라벨 갱신
    CString pathLabelText;
    pathLabelText.Format(L"탐색 경로: %s", pInfo->path.GetString());
    m_staticPathLabel.SetWindowTextW(pathLabelText);

    // 처리한 항목 수를 기준으로 ProgressBar 위치 계산 (log 스케일, 0~99%)
    LONG count = m_scanner.GetItemCount();
    if (count > 0)
    {
        const double maxItemsForLog = 1000000.0;
        double logMax = std::log10(maxItemsForLog + 1.0);
        double logVal = std::log10(static_cast<double>(count) + 1.0);
        double ratio = logVal / logMax;

        int targetProgress = static_cast<int>(ratio * 99.0);
        if (targetProgress < 0)   targetProgress = 0;
        if (targetProgress > 99)  targetProgress = 99;

        int currentProgress = m_progress.GetPos();
        if (targetProgress > currentProgress && targetProgress < 100)
        {
            m_progress.SetPos(targetProgress);
        }
    }

    // 동적으로 할당한 메모리 해제
    delete pInfo;

    return 0;
}

// 탐색 완료/중단 시 호출
LRESULT CDirSizeVisualizerDlg::OnScanFinished(WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(wParam);
    UNREFERENCED_PARAMETER(lParam);

    // 스레드 포인터 정리
    m_pScanThread = nullptr;

    // 버튼 상태 복원
    m_btnStart.EnableWindow(TRUE);
    m_btnStop.EnableWindow(FALSE);

    if (m_scanner.IsStopRequested())
    {
        m_staticStatus.SetWindowTextW(L"탐색이 중단되었습니다.");
        // 중단 시 ProgressBar는 현재 위치 그대로 두기
    }
    else
    {
        ULONGLONG totalSize = m_scanner.GetTotalSize();

        CString doneMsg;
        doneMsg.Format(L"탐색이 완료되었습니다. (총 크기: %llu Bytes)", totalSize);
        m_staticStatus.SetWindowTextW(doneMsg);
        m_progress.SetPos(100); // 완료 시 100%
    }

    return 0;
}

// 리스트뷰 컬럼 헤더 클릭 시 정렬 처리
void CDirSizeVisualizerDlg::OnLvnColumnclickListFiles(NMHDR* pNMHDR, LRESULT* pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);

    const int SIZE_COLUMN_INDEX = 2; // 0: 이름, 1: 유형, 2: 크기, 3: 경로

    if (pNMLV->iSubItem == SIZE_COLUMN_INDEX)
    {
        // 크기 컬럼 클릭 시 정렬 방향 토글
        m_bSizeSortAscending = !m_bSizeSortAscending;

        // ItemData(논리 ID)를 기준으로 CompareBySize 호출
        m_list.SortItems(CompareBySize, reinterpret_cast<DWORD_PTR>(this));
    }

    *pResult = 0;
}

// 크기 비교용 정렬 콜백 함수
int CALLBACK CDirSizeVisualizerDlg::CompareBySize(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    CDirSizeVisualizerDlg* pDlg = reinterpret_cast<CDirSizeVisualizerDlg*>(lParamSort);
    if (pDlg == nullptr)
    {
        return 0;
    }

    size_t id1 = static_cast<size_t>(lParam1);
    size_t id2 = static_cast<size_t>(lParam2);

    if (id1 >= pDlg->m_itemSizes.size() || id2 >= pDlg->m_itemSizes.size())
    {
        return 0;
    }

    ULONGLONG size1 = pDlg->m_itemSizes[id1];
    ULONGLONG size2 = pDlg->m_itemSizes[id2];

    if (size1 == size2)
    {
        return 0;
    }

    if (pDlg->m_bSizeSortAscending)
    {
        return (size1 < size2) ? -1 : 1;  // 오름차순
    }
    else
    {
        return (size1 > size2) ? -1 : 1;  // 내림차순
    }
}
