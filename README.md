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
  - 워커 스레드는 폴더 탐색 / 파일 처리 중에 중지 요청 플래그를 계속 확인하다가, 요청이 들어오면 작업을 정리하고 종료
  - 종료 후에는 `WM_SCAN_FINISHED` 메시지로 UI에 알려서 버튼 상태나 진행 표시를 다시 정리

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

## ✔️DirectoryScanner(디렉터리 탐색 담당)
  - 실제 디렉터리/파일을 읽고 크기를 계산하는 **핵심 로직 클래스**
  - **하는 일**
    - 시작 경로(기본값은 `C:\`)에서 출발해서  
      **재귀 호출**로 폴더를 계속 파고들어 모든 하위 폴더/파일을 돌며 검사한다.
      
    - 각 항목에 대해
      - 이름, 타입(파일/디렉터리), 크기, 전체 경로를 계산해서  
        `ScanResult`구조체 하나에 담는다.
        
    - `ScanResult`가 어느 정도 쌓이면
      - `ScanUpdateInfo`를 `new`로 하나 만들고
      - `WM_SCAN_UPDATE` 메시지와 함께 UI 스레드로 보내 화면을 갱신한다.
        
    - 탐색 도중에 중지 플래그를 계속 확인해서
      중지 요청이 들어오면 바로 빠져나올 수 있게 만든다.
      
    - 모든 탐색이 끝나면 마지막으로 `WM_SCAN_FINISHED` 메시지를 보내서
      UI 쪽에서 "끝났다"는 걸 알 수 있게 한다.
 
## 💡공용 데이터 구조(간략하게)

```cpp
struct ScanResult {
  std::wstring name;      // 파일 및 폴더 이름
  std::wstring type;      // 타입
  ULONGLONG size;         // 크기(byte)
  std::wstring fullPath;  // 경로
  };

struct ScanUpdateInfo {
  std::vector<ScanResult> results;   // 추가된 항목 배열
  ULONGLONG processedCount = 0;      // 처리한 수
};
```
- **워커 스레드** : `ScanUpdateInfo* info = new ScanUpdateInfo;` ➡ 데이터 채우고 ➡ `PostMessage`
- **UI 스레드** : 메시지 핸들러에서 `ScanUpdateInfo*`를 받아 사용 후 반드시 `delete`

## 💡클래스를 2개만 분리한 이유

처음에는 `ListViewManager`, `ProgressStatus`, `ScanController`같은 클래스를 더 쪼갤까도 고민했다.
하지만 실제로 구현을 진행하면서,
- 프로젝트 규모가 큰 수준이 아니고
- 클래스를 더 쪼개면
  - 파일 수는 늘어나는데,
  - **실제 이해도/가독성은 크게 좋아지지 않고,**
  - 오히려 흐름을 한 번 더 타고 들어가 봐야 하는 일이 생겨서 디버깅이 불편해질 수 있었다.
그래서 최종적으로는,
- **UI/스레드 제어/표시 =** `CDirSizeVisualizerDlg`
- **실제 디렉터리 스캔 로직 =** `DirectoryScanner`
라는 **2계층 구조**로 정리했다.

---

# 주요 구현(수동 메모리, 멀티스레드, DFS 재귀)

## ✔️스마트 포인터 대신 수동 메모리 관리

이번 과제에서는 **스마트 포인터를 일부러 쓰지 않고,**  
멀티스레드 환경에서 `new`/`delete`를 직접 관리해 보는 연습을 했다.

핵심 패턴은 다음 두 가지였다.

**1. 워커 스레드 ➡ UI 스레드로 데이터를 넘길 때**
```cpp
// DirectoryScanner 쪽 코드 (작업용 스레드에서 동작)
void DirectoryScanner::PostBatch(const std::vector<ScanResult>& batch) {
    if (batch.empty() || m_pNotifyWnd == nullptr) return;

    ScanUpdateInfo* info = new ScanUpdateInfo;
    info->results = batch;
    info->processedCount = m_processedCount;

    m_pNotifyWnd->PostMessage(WM_SCAN_UPDATE, 0, reinterpret_cast<LPARAM>(info));
}
```
```cpp
// CDirSizeVisualizerDlg에서 WM_SCAN_UPDATE를 받을 때 호출되는 함수(UI 스레드)
LRESULT CDirSizeVisualizerDlg::OnScanUpdate(WPARAM, LPARAM lParam)
{
    ScanUpdateInfo* info = reinterpret_cast<ScanUpdateInfo*>(lParam);
    if (info == nullptr) return 0;

    // 하면에 리스트와 progressbar를 반영
    UpdateListView(info->results);
    UpdateProgressBar(info->processedCount);

    delete info;  // 다 쓴 info는 여기서 메모리 해제
    return 0;
}
```
- **포인터를 누가 책임지는지** 규칙을 정해 두었다.
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


### 🔍delete 유무에 따른 메모리 사용 비교
위 `OnScanUpdate`에서 `delete info;` 한 줄을 기준으로 메모리 사용량이 어떻게 달라지는지 프로파일러로 비교해 보았다.  
경로 탐색을 한 번 실행했을 때도 차이가 있었지만,  
경로 탐색을 반복해서 실행할 때 마다 메모리 사용량의 차이가 크게 보였다.

**delete 호출 전**
<img width="1473" height="603" alt="delete 지운 후" src="https://github.com/user-attachments/assets/98b43979-38df-499b-8902-9d593d4986ec" />

**delete 호출 후**
<img width="1473" height="602" alt="delete 지우기 전" src="https://github.com/user-attachments/assets/e8c64314-e3c8-4b09-9967-c9f8eff94558" />

**경로 탐색은 3회를 반복해서 진행하며 탐색이 끝날 때 마다 메모리 사용량의 차이가 보이는 것을 알 수 있었다**

## ✔️멀티스레드 구조

- **1. 스레드 시작**
```cpp
// 시작 버튼을 눌렀을 때 호출되는 핸들러
void CDirSizeVisualizerDlg::OnBnClickedButtonStart()
{
    if (m_isScanning) return;  // 이미 탐색 중이면 무시

    CString path;
    m_editPath.GetWindowText(path);
    if (path.IsEmpty()) {
        path = L"C:\\";
    }

    // 혹시 남아 있을 수 있는 이전 스캩너 정리
    delete m_scanner;
    m_scanner = nullptr;

    // 새 스캐너 생성
    m_scanner = new DirectoryScanner(path, this);

    // UI 상태를 처음 상태로 초기화
    ResetUIForStart(path);

    // 워커 스레드 시작
    m_isScanning = true;
    m_pScanThread = AfxBeginThread(&CDirSizeVisualizerDlg::ScanThreadProc, this);
}
```
```cpp
// 워커 스레드 진입 함수(정적 함수여야 함)
UINT CDirSizeVisualizerDlg::ScanThreadProc(LPVOID pParam)
{
    auto* pDlg = reinterpret_cast<CDirSizeVisualizerDlg*>(pParam);
    if (pDlg == nullptr || pDlg->m_scanner == nullptr) return 0;

    // 실제 탐색은 DirectoryScanner에 맡긴다
    pDlg->m_scanner->StartScan();
    // 탐색이 끝나면 DirectoryScanner 내부에서 WM_SCAN_FINISHED를 전송

    return 0;
}
```

- **2. 중지 버튼 &  안전한 종료**
```cpp
// 중지 버튼을 눌렀을 때 호출되는 핸들러
void CDirSizeVisualizerDlg::OnBnClickedButtonStop()
{
    if (!m_isScanning || m_scanner == nullptr) return;

    // 바로 스레드를 죽이지 않고, 그만하라는 신호를 보냄
    m_scanner->RequestStop();
    // 실제 종료는 워커 스레드가 재귀를 빠져나오면서 자연스럽게 종료
}
```
```cpp
// DirectoryScanner 내부 - 중지 요청을 기록
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
// 재귀적으로 폴더를 도는 핵심 함수
void DirectoryScanner::ScanRecursive(const std::wstring& path)
{
    // 먼저 중지 요청이 들어왔는지 확인
    if (IsStopRequested()) return;

    // FindFirstFile ~ FindNextFile 루프
    // 각 항목을 처리하기 전/후에 stop 플래그를 다시 확인해서
    // 요청이 들어오면 그 지점에서 깔끔하게 빠져나오도록 한다
}
```
- 스레드를 강제로 종료하는 대신,
  **중지 신호만 보내고 스레드가 스스로 정리하면서 끝나도록 하는 방식**을 사용했다.

## ✔️DFS 기반 재귀 탐색

탐색은 **DFS(Depth-First Search)를 재귀 호출로 구현했다.**
```cpp
void DirectoryScanner::StartScan()
{
    m_processedCount = 0;
    ScanRecursive(m_rootPath);

    // 탐색 종료 시에 UI에 알림
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

        // 폴더라면 안으로 한 번 더 들어감
        if (isDir) {
            ScanRecursive(fullPath);
        }

    } while (::FindNextFile(hFind, &findData));

    ::FindClose(hFind);

    // 남은 항목도 한 번 더 보내기
    if (!batch.empty()) {
        PostBatch(batch);
    }
}
```
💡 따로 거디한 트리를 만들어서 모두 들고 있지 않고,  
  디렉터리를 방문할 때마다 바로 처리하고 안쪽으로 내려갔다가  
  재귀가 끝나면 그대로 스택에서 빠져 나오게 만드는 방식이라  
  메모리를 덜 쓰고, 전체 로직도 비교적 단순하게 유지할 수 있었다.

---

# 구현하면서 겪은 어려움과 해결 과정

## ✔️리스트뷰에 모든 항목을 다 넣었을 때의 성능 문제

**문제**
- 처음에는
  - 워커 스레드에서 오는 `ScanUpdateInfo::results`안에 있는 항목들을 **모두 리스트뷰에 추가**하는 방식이었다.
- 그런데 `C:\` 처럼 큰 경로를 돌리면
  - 수만~수십만 개의 항목이 들어올 수 있고,
  - 리스트뷰에 항목을 계속 추가하다 보니
    - UI가 심하게 버벅이고,
    - 탐색이 끝난 뒤에도 스크롤이 너무 무거워지는 문제가 생겼다.

**해결**
- "모든 항목을 보여주는 게 중요한 게 아니라, **대략적인 분포와 큰 파일**만 보여주면 된다"는 방향으로 바꿨다.
  - **처음 500개 항목** ➡ 전부 표시
  - 그 이후부터는
    - `processedCount` 기준으로
    - **예를 들어 500개마다 하나씩만** 하나씩만 샘플링해서 리스트에 추가했다.
  - 리스트뷰 최대 행 수도
    - `m_maxDisplayCount = 5000` 정도로 제한을 두었다.

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
➡ 이렇게 **샘플링 + 최대 개수 제한**을 걸어 준 뒤에는
- UI가 훨씬 가볍게 유지되면서도,
- "어느 폴더에 큰 파일이 많은지",
  "어디에 큰 파일이 있는지"같은 정보는 충분히 확인할 수 있었다.

## ✔️ProgressBar가 끝까지 안 차는 것처럼 보이는 문제

**문제**
- 처음엔 `processedCount`를 그대로 비율로 계산해서 ProgressBar를 업데이트하려고 했다.
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
    int progress = static_cast<int>(v * 20); // 프로젝트 상황에 맞게 튜닝
    if (progress > 100) progress = 100;
    return progress;
}
```
- 정확한 수치는 프로젝트를 진행하며 여러 번 찍어보면서 조정했다.
- 핵심은
  - 초반에 너무 빨리 90%까지 가버리지 않고,
  - 끝날 때쯤 100% 근처로 모이게 만드는 것이었다.

➡ 이렇게 바꾸고 나서는  
탐색이 오래 걸려도 ProgressBar가 조금씩 꾸준히 움직이게 되었다.

## ✔️워커 스레드와 UI 스레드 사이의 메모리 관리

**문제**
- 멀티스레드에서 `new`/`delete`를 직접 쓰다 보니,
  - "이 포인터를 누가 지우지?"
  - "여기서 지우면 다른 스레드에서 또 건드리는 건 아닐까"같은 걱정이 계속 들었다.

- 특히 `ScanUpdateInfo`처럼
  - 워커 스레드에서 만들고
  - UI 스레드에서 해제해야 하는 구조체는
  - **소유권 규칙을 조금만 헷갈리면 메모리 누수나 double free가 날 수 있는** 부분이었다.
 
**해결**
- 스레드 사이에 넘기는 데이터는 항상 `ScanUpdateInfo*` 한 방향으로만 흐르도록 정리했다.
  - `new`는 워커 스레드에서만,
  - `delete`는 UI 스레드의 메시지 핸들러에서만 넘기도록 했다.

- 패턴은 항상 같은 모양을 유지했다.
```cpp
// 워커 스레드 쪽
ScanUpdateInfo* info = new ScanUpdateInfo;
// 데이터 채우기
PostMessage(..., reinterpret_cast<LPARAM>(info));

// UI 스레드 쪽
LRESULT OnScanUpdate(WPARAM, LPARAM lParam)
{
    auto* info = reinterpret_cast<ScanUpdateInfo*>(lParam);
    if (!info) return 0;

    // 데이터 사용
    UpdateListView(info->results);
    UpdateProgressBar(info->processedCount);

    delete info;  // 여기서 한 번만 해제
    return 0;
}
```
- `ScanUpdateInfo*`를 다른 멤버 변수에 따로 보관해서 여러 군데에서 건드리거나,
  두 번 이상 `delete`할 수 있는 여지를 만들지 않았다.

➡ 이러한 과정을 통해서 **멀티스레드**와 **수동 메모리 관리**에서 신경써야 될 부분을 체감해 볼 수 있었다.

## ✔️중지 기능 구현 방식 선택

**문제**
- 중지를 구현할 때
  - `TerminateThread`같은 함수를 써서 강제로 끊어버릴 수도 있지만,
  - 그렇게 하면
    - 현재 실행 중인 함수가 어디까지 갔는지 모르는 상태에서 끊기기 때문에
    - 핸들/파일/포인터 정리가 꼬일 수 있다.

  - 게다가 DFS 재귀 구조라서,
    - 깊은 폴더 안쪽에서 갑자기 끊어지면 **스택이 어떤 상태로 남는지도** 신경써야 했다.

**해결**
- 강제 종료는 사용하지 않고,
  - `DirectoryScanner` 안에 `m_stopRequested`라는 플래그를 두고,
  - 재귀 진입과 루프 안에서 `IsStopResquested()`를 확인하는 식으로,
  - **스레드가 스스로 깔끔하게 빠져나오도록** 만들었다.

➡ 이 과정 덕분에,  
멀티스레드에서 스레드를 억지로 죽이기 보다, **스레드가 스스로 정리하고 끝나게 만드는 방식**의 중요성을 체감해보았다.

## ✔️그 외 자잘한 시행착오들

- **경로 라벨(`탐색 경로:`)과 실제 입력 경로 동기화**
  - 시작 버튼을 누를 때마다 라벨을 현재 입력 경로로 갱신해 줘야 해서,
  - 초기화/다시 시작 시점마다 라벨 업데이트를 빠뜨리지 않는 게 은근히 신경 쓰이는 부분이었다.

- **Start 버튼 중복 클릭 방지**
  - 스캔 중일 때 Start를 눌러서 스레드가 두 개 이상 떠 버리지 않도록
    - 코드에서는 `m_isScanning` 플래그로 한 번 막고,
    - UI 쪽에서도 Start 버튼 자체를 비활성화해서 막아 두었다.

- **리스트뷰 정렬**
  - 특히 "크기" 기준 정렬 시
    - 문자열 기준이 아닌 **숫자 기준**으로 비교해야 해서
    - `LVITEM`에서 값을 꺼내 `ULONGLONG`으로 비교하는 커스텀 비교 함수를 작성했다.
  - 정렬 방향(오름차순/내림차순)은
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
