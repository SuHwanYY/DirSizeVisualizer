# 프로젝트 소개
- 원하는 경로 안의 파일의 크기를 모두 탐색해 주는 **디렉터리 크기 시각화 툴**
- 시작 버튼을 통해 탐색을 시작하며 중지 버튼을 통해 중단할 수 있다.
- `progress bar`를 통해 진행도를 보여준다.
- 탐색이 끝난 후에는 해당 경로의 총 크기를 `Byte`단위로 볼 수 있고, 파일 목록을 볼 수 있다.
- 크기를 클릭할 때마다 파일 크기가 높은 순, 낮은 순으로 번갈아가며 정렬한다.

---

# 프로젝트 개요

- **프로젝트 이름 :** DirSizeVisualizer

- **목표**
  - windows 환경에서 특정 경로(기본값: `c:\`)를 **재귀적으로 탐색**
  - 각 디렉터리 / 파일의 크기를 계산하고 `ListView`와 `ProgressBar`를 통해 시각적으로 보여주는 도구를 만드는 것.
  - 단순한 콘솔 프로그램이 아닌 **MFC 기반 GUI**로 제작.
  - C++의 메모리 관리를 경험해보기 위해 **스마트 포인터**를 사용하지 않고 **메모리를 직접 생성, 소멸**하는 데 집중.
  - 규모가 큰 디렉터리를 빠르게 탐색할 수 있도록 **멀티스레드**를 사용.

 ---

 # UI 구성 요소

**1. 경로 입력 Edit**
  - 기본값:`c:\`
  - 사용자가 다른 경로로 변경 가능하며, Start를 눌러서 해당 경로를 탐색한다.

**2. Start 버튼**
  - 기존 탐색이 있다면 정리한다.
  - 새 워커 스레드 생성, `ProgressBar` 초기화, `ListView` 초기화

**3. Stop 버튼**
  - 워커 스레드 내부에 **중단 플래그**를 세우도록 구현
  - 워커 스레드는 각 디렉터리 진입/파일 처리 시 플래그를 확인하고 안전하게 종료
  - 종료 후 `WM_SCAN_FINISHED`를 통해 UI에서 상태 정리

**4. ProgressBar**
  - 단순한 **선형 스케일**(전체 파일 수 대비 얼마나 처리했는지)이 아닌, **로그 스케일 기반으로 값 갱신**
  - 작은 디렉터리도 진행도가 어느 정도 보이게 하며,
  - 큰 디렉터리(`C:\`)는 초반에 너무 안움직이는 느낌을 줄이기 위해 **로그 스케일**을 사용

**5. ListView**
  - 컬럼 구성
    - 이름(Name)
    - 타입(Type: Directory / File)
    - 크기(Size)
    - 경로(Full Path)
  - 탐색 중간중간 `WM_SCAN_UPDATE` 메시지를 받아 한 줄씩 추가
  - 정렬기능 : 크기(Size) 클릭시 **숫자 크기 기준**으로 오름차순/내림차순 정렬

**6. 상태 표시 라벨**
  - 예: `"탐색 경로: C:\Windows\System32` 형태
  - `WM_SCAN_UPDATE`를 받을 때마다 "현재 처리 중인 경로"를 갱신
  - 너무 잦은 갱신으로 깜빡임/부하가 생기지 않도록 적절히 조절

---

# 내부 구조 설계
💡이 프로젝트는 **디렉토리 탐색 로직과 UI & 스레드 제어**를 명확히 분리하는 것을 목표로 설계했다.

## ✔️CDirSizeVisualizerDlg(UI & 제어 담당)
  - **하는 일**
    - 경로 입력 에딧, 시작/중비 버튼, 리스트뷰, ProgressBar, 상태 텍스트, `탐색 경로:` 라벨 등 **UI 전체 관리**
    - **"시작" 버튼 클릭 시**
      - 입력된 경로를 읽고,
      - `DirectoryScanner`를 생성한 뒤,
      - `AfxBeginThread`로 워커 스레드를 만들어 탐색 시작.
    - **"중지" 버튼 클릭 시**
      - 워커 스레드가 사용 중인 `DirectoryScanner에 **중지 요청 플래그**를 세움.
    - 워커 스레드에서 보내는
      - `WM_SCAN_UPDATE` 메시지 ➡ 리스트뷰/ProgressBar 갱신
      - `WM_SCAN_FINISHED` 메시지 ➡ 버튼 상태 복구, "완료/중지됨" 텍스트 출력, 스레드 정리.
    - **한 줄 정리**
      - **"UI + 스레드 라이프사이클 + 표시/정렬만 담당하는 레이어".**
   

## ✔️DirectoryScanner(디렉터리 탐색 담당)
  - 실제 디렉터리/파일을 읽고 크기를 계산하는 **핵심 로직 클래스**
  - **하는 일**
    - 시작 경로(기본 `C:\`)부터 **DFS 기반 재귀 탐색**으로 모든 하위 폴더/파일 순회.
    - 각 항목에 대해
      - 이름, 타입(파일/디렉터리), 크기, 전체 경로를 계산해 `ScanResult` 구조체에 담음.
    - 일정 개수의 `ScanResult`가 모이면
      - `ScanUpdateInfo`를 `new`로 할당해서
      - `WM_SCAN_UPDATE` 메시지로 UI 스레드에 전달.
    - 중지 요청 플래그를 주기적으로 확인해 **안전하게 탐색을 빠져나올 수 있게**함.
    - 탐색이 끝나면 마지막으로 `WM_SCAN_FINISHED` 메시지 전송.
 
## 💡공용 데이터 구조(간략하게)

```cpp
struct ScanResult {
  std::wstring name;      // 파일/폴더 이름
  std::wstring type;      // "File" or "Directory"
  ULONGLONG size;         // 바이트 단위 크기
  std::wstring fullPath;  // 전체 경로
  };

struct ScanUpdateInfo {
  std::vector<ScanResult> results;   // 이번 배치에 추가된 항목들
  ULONGLONG processedCount = 0;      // 지금까지 처리한 전체 개수
};
```
- **워커 스레드** : `ScanUpdateInfo* info = new ScanUpdateInfo;` ➡ 데이터 채우고 ➡ `PostMessage`
- **UI 스레드** : 메시지 핸들러에서 `ScanUpdateInfo*`를 받아 사용 후 반드시 `delete`

## 💡클래스를 2개만 분리한 이유

처음에는 `ListViewManager`, `ProgressStatus`, `ScanController`같은 클래스를 더 쪼갤까도 고민했다.
하지만 실제로 구현을 진행하면서,
- 프로젝트 규모가 **"MFC 다이얼로그 + DFS 탐색기"** 수준이고,
- 멀티스레드/메시지 처리 흐름만 잘 잡으면 구조가 이미 충분히 명확했으며,
- 클래스를 더 쪼개면
  - 파일 수는 늘어나는데,
  - **실제 이해도/가독성은 크게 좋아지지 않고,**
  - 오히려 흐름을 한 번 더 타고 들어가 봐야 하는 레벨이 생겨서 디버깅이 불편해질 수 있었다.
그래서 최종적으로는,
- **UI/스레드 제어/표시 =** `CDirSizeVisualizerDlg`
- **실제 디렉터리 스캔 로직 =** `DirectoryScanner`
라는 **2계층 구조**로 정리했다.

- 위와 같은 분리는
  - "UI 코드와 비즈니스 로직 분리"라는 목적을 달성하면서도,
  - 과도한 추상화 없이 자연스럽다고 판단했다.

---

# 주요 구현(수동 메모리, 멀티스레드, DFS 재귀)

## ✔️스마트 포인터 대신 수동 메모리 관리

이번 과제에서는 **스마트 포인터를 일부러 쓰지 않고,**  
멀티스레드 환경에서 `new`/`delete`를 직접 관리해 보는 연습을 했다.
<br>
핵심 패턴은 다음 두 가지였다.
<br>
**1. 워커 스레드 ➡ UI 스레드로 데이터를 넘길 때**
```cpp
// DirectoryScanner 내부 (워커 스레드)
void DirectoryScanner::PostBatch(const std::vector<ScanResult>& batch) {
    if (batch.empty() || m_pNotifyWnd == nullptr) return;

    ScanUpdateInfo* info = new ScanUpdateInfo;
    info->results = batch;
    info->processedCount = m_processedCount;

    m_pNotifyWnd->PostMessage(WM_SCAN_UPDATE, 0, reinterpret_cast<LPARAM>(info));
}
```
```cpp
// CDirSizeVisualizerDlg 메시지 핸들러 (UI 스레드)
LRESULT CDirSizeVisualizerDlg::OnScanUpdate(WPARAM, LPARAM lParam)
{
    ScanUpdateInfo* info = reinterpret_cast<ScanUpdateInfo*>(lParam);
    if (info == nullptr) return 0;

    // 리스트뷰 / ProgressBar 업데이트
    UpdateListView(info->results);
    UpdateProgressBar(info->processedCount);

    delete info;  // 여기서 반드시 delete
    return 0;
}
```
- **소유권 규칙**을 명확히 정했다.
  - `new`는 워커 스레드에서만.
  - `delete`는 UI 스레드 메시지 핸들러에서만.
- 이 규칙 덕분에
  - 이중 해제나 "누가 지우는지 애매한 포인터" 문제를 피할 수 있었다.
 
**2. 스캐너 객체와 스레드 포인터 관리**
```cpp
// 다이얼로그 멤버
DirectoryScanner* m_scanner = nullptr;
CWinThread*       m_pScanThread = nullptr;
bool              m_isScanning = false;
```
- 시작 버튼:
  - `m_scanner = new DirectoryScanner(...);`
  - `m_pScanThread = AfxBeginThread(ScanThreadProc, this);`
- WM_SCAN_FINISHED`에서:
  - 스레드가 끝난 뒤 `delete m_scanner; m_scanner = nullptr;`
  - `m_pScanThread`는 MFC가 관리하지만, 필요 시 nullptr로 초기화.  
**스마트 포인터가 없는 상태에서 이런 수동 규칙을 직접 잡아보는 게 이번 과제의 핵심 중 하나였다.**

## ✔️멀티스레드 구조

- **1. 스레드 시작**
```cpp
// 시작 버튼 핸들러
void CDirSizeVisualizerDlg::OnBnClickedButtonStart()
{
    if (m_isScanning) return;

    CString path;
    m_editPath.GetWindowText(path);
    if (path.IsEmpty()) {
        path = L"C:\\";
    }

    // 기존 스캐너 정리
    delete m_scanner;
    m_scanner = nullptr;

    // 새 스캐너 생성
    m_scanner = new DirectoryScanner(path, this);

    // UI 상태 초기화
    ResetUIForStart(path);

    // 워커 스레드 시작
    m_isScanning = true;
    m_pScanThread = AfxBeginThread(&CDirSizeVisualizerDlg::ScanThreadProc, this);
}
```
```cpp
// 정적 스레드 함수
UINT CDirSizeVisualizerDlg::ScanThreadProc(LPVOID pParam)
{
    auto* pDlg = reinterpret_cast<CDirSizeVisualizerDlg*>(pParam);
    if (pDlg == nullptr || pDlg->m_scanner == nullptr) return 0;

    pDlg->m_scanner->StartScan();
    // Scan이 끝나면 DirectoryScanner 내부에서 WM_SCAN_FINISHED 전송

    return 0;
}
```

- **2. 중지 버튼 &  안전한 종료**
```cpp
void CDirSizeVisualizerDlg::OnBnClickedButtonStop()
{
    if (!m_isScanning || m_scanner == nullptr) return;

    m_scanner->RequestStop();   // stop 플래그만 세움
    // 실제 종료는 워커 스레드가 재귀를 빠져나오면서 자연스럽게 끝남
}
```
```cpp
// DirectoryScanner 내부
void DirectoryScanner::RequestStop()
{
    m_stopRequested = true;
}

bool DirectoryScanner::IsStopRequested() const
{
    return m_stopRequested;
}
```
```cpp
void DirectoryScanner::ScanRecursive(const std::wstring& path)
{
    if (IsStopRequested()) return;

    // FindFirstFile ~ FindNextFile 루프
    // 각 항목 처리 전에/후에 stop 체크
}
```
- 강제로 스레드를 Kill하는 방식이 아니라 **"협조적인 종료(cooperative cancel)"를 선택해서,  
  **리소스 정리**나 **메모리 해제 순서**가 꼬이지 않도록 했다.

## ✔️DFS 기반 재귀 탐색

탐색은 **DFS(Depth-First Search)를 재귀로 구현했다.**
```cpp
void DirectoryScanner::StartScan()
{
    m_processedCount = 0;
    ScanRecursive(m_rootPath);

    // 탐색 종료 알림
    if (m_pNotifyWnd) {
        m_pNotifyWnd->PostMessage(WM_SCAN_FINISHED, 0, 0);
    }
}
```
```cpp
void DirectoryScanner::ScanRecursive(const std::wstring& path)
{
    if (IsStopRequested()) return;

    WIN32_FIND_DATA findData;
    std::wstring searchPath = path + L"\\*";

    HANDLE hFind = ::FindFirstFile(searchPath.c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return;

    std::vector<ScanResult> batch;

    do {
        if (IsStopRequested()) break;

        const std::wstring name = findData.cFileName;
        if (name == L"." || name == L"..") continue;

        std::wstring fullPath = path + L"\\" + name;
        bool isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        ScanResult r;
        r.name = name;
        r.fullPath = fullPath;
        r.type = isDir ? L"Directory" : L"File";
        r.size = isDir ? 0 : (static_cast<ULONGLONG>(findData.nFileSizeHigh) << 32) | findData.nFileSizeLow;

        batch.push_back(r);
        ++m_processedCount;

        // 일정 개수마다 UI로 전송
        if (batch.size() >= m_batchSize) {
            PostBatch(batch);
            batch.clear();
        }

        // 디렉터리라면 재귀 호출
        if (isDir) {
            ScanRecursive(fullPath);
        }

    } while (::FindNextFile(hFind, &findData));

    ::FindClose(hFind);

    // 남은 배치 처리
    if (!batch.empty()) {
        PostBatch(batch);
    }
}
```
별도의 거대한 트리를 만들지 않고,  
"**방문하는 디렉터리에서 바로바로 처리하고 내려가고, 스택에서 빠지면 끝나면 구조**"로 구현해서  
  - 메모리 사용량을 줄이고,
  - 탐색 로직을 단순하게 유지했다.

---

# 구현하면서 겪은 어려움과 해결 과정

## ✔️리스트뷰에 모든 항목을 다 넣었을 때의 성능 문제

**문제**
- 처음 구현은
  - 워커 스레드에서 오는 `ScanUpdateInfo`의 `results`를 **모두 리스트뷰에 추가**하는 방식이었다.
- `C:\` 전체를 탐색하면
  - 수만~수십만 개의 항목이 들어올 수 있고,
  - 리스트뷰에 항목을 계속 추가하다 보니
    - UI가 심하게 버벅이고,
    - 탐색이 끝난 뒤에도 스크롤이 답답할 정도로 무거워졌다.

**해결**
- "모든 항목을 보여주는 게 중요한 게 아니라, **대략적인 분포와 큰 파일만 보여주면 된다"는 방향으로 바꿨다.
  - **처음 500개 항목** ➡ 전부 표시
  - 그 이후부터는
    - `processedCount` 기준으로
    - **일정 간격(예:500개마다)** 하나씩만 샘플링해서 표시
  - 리스트뷰 최대 행 수
    - `m_maxDisplayCount = 5000` 정도로 제한

```cpp
void CDirSizeVisualizerDlg::UpdateListView(const std::vector<ScanResult>& results)
{
    for (const auto& r : results) {
        if (m_displayedCount < 500) {
            AddListRow(r);
            ++m_displayedCount;
        } else if (m_displayedCount < m_maxDisplayCount) {
            if (m_totalProcessed % m_sampleInterval == 0) {
                AddListRow(r);
                ++m_displayedCount;
            }
        } else {
            break;
        }

        ++m_totalProcessed;
    }
}
```
➡ 이 샘플링과 제한을 적용한 뒤에는
- UI가 훨씬 가볍게 유지되면서도,
- "어느 폴더에 큰 파일이 많은지"같은 **탐색 도구로서의 목적은 충분히 달성**할 수 있었다.

## ✔️ProgressBar가 끝까지 안 차는 것처럼 보이는 문제

**문제**
- 처음엔 `processedCount`를 그대로 퍼센트로 환산해서 ProgressBar를 업데이트하려고 했다.
  - 하지만 전체 파일 개수를 정확히 알 수 없고,
  - 디렉터리 깊이에 따라 처리 시간이 크게 달라져서,
  - 초반에는 빠르게 올라가다가 **어느 순간 거의 안 움직이는 것처럼 느껴지는** 문제가 있었다.
**해결**
- 정확한 %를 맞추는 것보다
  - **"계속 진행되고 있다"는 피드백이 더 중요**하다고 보고,
- 진행률을 **로그 스케일 느낌**으로 바꿨다.

예시)
```cpp
int CDirSizeVisualizerDlg::CalcProgress(ULONGLONG processed)
{
    if (processed == 0) return 0;

    double v = std::log10(static_cast<double>(processed) + 1.0);
    int progress = static_cast<int>(v * 20); // 값은 프로젝트에 맞게 튜닝
    if (progress > 100) progress = 100;
    return progress;
}
```
- 정확한 수치는 프로젝트를 진행하며 여러 번 찍어보면서 조정했다.
- 핵심은
  - 초반에 너무 빨리 90%까지 가버리지 않고,
  - 끝날 때쯤 100% 근처로 모이게 만드는 감각적인 튜닝이었다.
➡ 이렇게 바꾸고 나서는  
탐색이 오래 걸려도 ProgressBar가 조금씩 꾸준히 움직여서,  
사용자 경험이 훨씬 낫다고 느꼈다.

## ✔️워커 스레드와 UI 스레드 사이의 메모리 관리

**문제**
- 멀티스레드에서 `new`/`delete`를 직접 쓰다 보니,
  - "이 포인터를 누가 지우지?"
  - "여기서 지우면 다른 스레드에서 또 건드리는 건 아닐까"같은 걱정이 계속 들었다.
- 특히 `ScanUpdateInfo`같은 구조체는
  - 워커 스레드에서 생성해서
  - UI 스레드에서 해제해야 하므로
  - **소유권 규칙을 조금만 헷갈리면 메모리 누수나 double free 위험**이 있다.
 
**해결**
- "스레드 간 데이터를 건널 때는 항상 `ScanUpdateInfo*`로 보내고,  
  `new`는 워커 한 번, `delete`는 UI 한 번"같이 단순하게 정리했다.
- 구현 패턴을 **항상 같은 형태**로 유지

```cpp
// 워커
ScanUpdateInfo* info = new ScanUpdateInfo;
// 데이터 채우기
PostMessage(..., reinterpret_cast<LPARAM>(info));

// UI
LRESULT OnScanUpdate(WPARAM, LPARAM lParam)
{
    auto* info = reinterpret_cast<ScanUpdateInfo*>(lParam);
    // 사용
    delete info;
}
```
- 다른 곳에서 `ScanUpdateInfo*`를 멤버로 보관하거나,  
  여러 번 `delete` 할 수 있는 여지를 애초에 만들지 않았다.
➡ 이 규칙을 스스로 강제하면서  
**"멀티스레드 + 수동 메모리 관리에서 가장 위험한 부분을 직접 컨트롤해봤다"는 경험을 얻을 수 있었다.**

## ✔️중지 기능 구현 방식 선택

**문제**
- 중지를 구현할 때
  - `TerminateThread`같은 함수를 써서 강제로 끊어버릴 수도 있지만,
  - 그렇게 하면
    - 현재 실행 중인 함수가 어디까지 갔는지 모르는 상태에서 끊기기 때문에
    - 핸들/파일/포인터 정리가 꼬일 수 있다.
  - DFS 재귀 구조라서,
    - 깊은 디렉터리 안쪽에서 멈추면 스택이 어떻게 남는지 신경 쓸 포인트가 많았다.

**해결**
- 강제 종료는 사용하지 않고,
  - `DirectoryScanner` 내부에 `m_stopRequested` 플래그를 두고,
  - 각 재귀 진입/루프마다 `IsStopRequesTed()`를 체크하여 **자연스럽게 상위로 빠져나오도록** 설계.

- 정리 순서
  - 중지 버튼 ➡ `RequestStop()` ➡ 재귀가 차례차례 종료 ➡ `StartScan()`이 끝 ➡  
    `WM_SCAN_FINISHED` 전송 ➡ UI 스레드에서 버튼 상태 복구 및 스캐너 삭제.

➡ 이 과정 덕분에,  
"멀티스레드에서 강제 종료 대신 플래그로 종료를 설계하는 패턴"을 직접 체감할 수 있었다.

## ✔️그 외 자잘한 시행착오들

- **경로 라벨(`탐색 경로:`)과 실제 입력 경로 동기화**
  - 시작 버튼을 누를 때마다 라벨을 현재 입력 경로로 갱신해 줘야 해서,
  - 초기화/다시 시작 시점마다 라벨 업데이트를 누락하지 않는 것이 은근히 신경 쓸 포인트였다.
- **Start 버튼 중복 클릭 방지**
  - 스캔 중일 때 다시 Start를 눌러서 스레드가 두 개 뜨지 않도록
    - `m_isScanning` 플래그로 버튼을 막고,
    - UI에서도 아예 Start 버튼을 비활성화하는 이중 안전장치를 뒀다.
- **리스트뷰 정렬**
  - 특히 "크기" 컬럼 정렬 시
    - 문자열 기준이 아닌 **숫자 기준**으로 비교해야 해서
    - `LVITEM`에서 데이터를 읽어와 `ULONGLONG`으로 비교하는 커스텀 비교 함수를 작성했다.
  - 정렬 토글(오름차순/내림차순)을 구현하며
    - 현재 정렬 상태를 멤버로 들고 있다가 헤더 클릭 시 반전시키도록 설계했다. 

---
# 회고

## 느낀 점
이번 과제는 처음으로 만들어본 MFC 기반 윈도우 툴이라, 시작할 때는 솔직히 "이걸 내가 끝까지 만들 수 있나?" 싶은 마음이 컸다.  
대화형 콘솔 프로그램이랑은 다르게, 버튼·리스트뷰·ProgressBar 같은 UI 컨트롤을 배치하고, 메시지로 서로 연결해 주는 흐름이 처음에는 잘 안 잡혀서 많이 헤맸다.  

특히 멀티스레드를 붙이면서  
- UI 스레드와 워커 스레드를 분리해야 하고,
- `postMessage`로 데이터 구조를 넘긴 다음,
- 어디에서 `new` 하고 어디에서 `delete` 해야 하는지  
계속 신경 쓰다 보니, 코드 작성이 수월하지 않았다.  

하지만 목표했던 기능들을 완성하고 보니 처음 접하는 기술에 대한 막연한 두려움이 많이 사라졌고,  
앞으로도 새로운 개발 분야를 공부해보고 싶을 때 자신감을 가지고 임할 수 있을 것 같다.
 
## 앞으로 개선해 보고 싶은 점
- 현재는 `new`/`delete` 기반으로 잘 동작하지만,
  같은 구조를 **스마트 포인터**로 리팩터링해 보면서
  두 방식의 차이를 비교해 보고 싶다.

- UI 측면에서
  - 특정 크기 이상인 파일만 필터링해서 보여주는 기능과
  - 그래프로 시각화하는 기능도 확장해 보고싶다.
