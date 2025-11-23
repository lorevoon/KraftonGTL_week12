# Mundi Engine Symbol Server Setup Guide

## Overview

Mundi 프로젝트는 Symbol Server를 사용하여 팀원들이 크래시 덤프를 디버깅할 수 있도록 지원합니다.

**Symbol Server 주소**: `\\172.21.11.115\symbols`

---

## 자동 설정 (권장)

프로젝트 루트 디렉토리에서 다음 스크립트를 실행하세요:

```cmd
SetupSymbolServer.bat
```

이 스크립트는 다음을 자동으로 수행합니다:
- 네트워크 연결 테스트
- Symbol Server 접근 권한 확인
- Visual Studio 설정 안내

---

## 수동 설정

### Visual Studio 2022 Symbol Server 설정

1. Visual Studio 2022 실행
2. **Tools** → **Options** → **Debugging** → **Symbols** 메뉴 진입
3. "Symbol file (.pdb) locations" 섹션에서 폴더 아이콘 클릭
4. 다음 경로 추가:
   ```
   \\172.21.11.115\symbols
   ```
5. (선택사항) Microsoft Symbol Servers 체크박스 활성화
6. (선택사항) "Cache symbols in this directory" 설정: `C:\SymbolCache`
7. **OK** 클릭하여 저장

### 네트워크 접근 권한 확인

Symbol Server에 접근하려면 네트워크 공유 폴더에 대한 읽기 권한이 필요합니다:

```cmd
# 연결 테스트
dir \\172.21.11.115\symbols

# 접근 불가 시 네트워크 드라이브 매핑
net use Z: \\172.21.11.115\symbols /persistent:yes
```

---

## 빌드 시 Symbol 자동 게시

프로젝트를 빌드하면 Post-Build 이벤트에서 자동으로 PDB 파일을 Symbol Server에 게시합니다.

### 빌드 출력 예시

```
[SymStore] Publishing symbols to C:\Users\NanSu\symbols...
SYMSTORE: Number of files stored = 1
SYMSTORE: Number of errors = 0
SYMSTORE: Number of files ignored = 0
```

### 지원되는 Configuration

모든 빌드 설정에서 심볼이 자동으로 게시됩니다:
- **Debug** (x64)
- **Debug_StandAlone** (x64)
- **Release** (x64)
- **Release_StandAlone** (x64)

---

## Crash Dump 디버깅

### 1. 크래시 덤프 파일 찾기

크래시 발생 시 로컬에 덤프 파일이 저장됩니다:

```
Binaries\{Configuration}\Mundi_Crash_YYYY-MM-DD_HH-MM-SS.dmp
```

### 2. Visual Studio에서 덤프 파일 열기

1. Visual Studio 2022 실행
2. **File** → **Open** → **File...**
3. 덤프 파일 선택 (.dmp)
4. 우측 상단 **Debug with Native Only** 클릭
5. Symbol Server에서 자동으로 PDB 파일을 다운로드하여 디버깅 시작

### 3. Symbol 로딩 확인

디버깅 중 심볼이 제대로 로드되지 않으면:

1. **Debug** → **Windows** → **Modules** 창 열기
2. Mundi.exe 우클릭 → **Symbol Load Information** 확인
3. Symbol 경로가 올바른지 확인:
   - `\\172.21.11.115\symbols`가 검색 경로에 포함되어야 함

---

## 문제 해결

### Symbol Server 접근 불가

**증상**: "Cannot access \\172.21.11.115\symbols"

**해결책**:
1. 네트워크 연결 확인
   ```cmd
   ping 172.21.11.115
   ```
2. 공유 폴더 권한 확인
3. Windows Firewall 설정 확인
4. 다른 팀원에게 접근 권한 요청

### Symbol이 로드되지 않음

**증상**: Visual Studio에서 "Symbols not loaded"

**해결책**:
1. Visual Studio 심볼 경로 재확인 (Tools → Options → Debugging → Symbols)
2. Symbol Cache 삭제 후 재시도
   ```cmd
   del /s /q "C:\SymbolCache\*.*"
   ```
3. 해당 빌드가 Symbol Server에 게시되었는지 확인
4. 빌드 버전과 덤프 버전이 일치하는지 확인

### 빌드 시 symstore.exe 오류

**증상**: "symstore.exe is not recognized"

**해결책**:
1. Windows Debugging Tools 설치 확인
   - 기본 경로: `C:\Program Files (x86)\Windows Kits\10\Debuggers\x64\`
2. Windows SDK 재설치
3. vcxproj의 symstore.exe 경로 수정

---

## 서버 관리 (Symbol Server 호스트 전용)

### Symbol Store 디렉토리 구조

```
C:\Users\NanSu\symbols\
├── 000Admin\           # 관리 정보
├── Mundi.pdb\          # PDB 파일별 폴더
│   ├── {GUID1}\        # 각 버전별 GUID
│   └── {GUID2}\
└── pingme.txt          # 연결 테스트 파일
```

### 공유 폴더 권한 설정

```cmd
# 읽기/쓰기 권한 부여 (빌드 머신용)
icacls "C:\Users\NanSu\symbols" /grant Everyone:(OI)(CI)F

# Windows 공유 설정
net share symbols=C:\Users\NanSu\symbols /grant:everyone,FULL
```

### 오래된 Symbol 정리

Symbol Store는 시간이 지남에 따라 용량이 커집니다. 주기적으로 정리하세요:

```cmd
# 90일 이상 된 파일 삭제 (예시)
forfiles /p "C:\Users\NanSu\symbols" /s /d -90 /c "cmd /c del @path"
```

---

## 추가 정보

### Symbol Server 원리

1. 빌드 시 `symstore.exe`가 PDB 파일을 Symbol Server에 업로드
2. 각 PDB는 고유한 GUID로 식별됨 (빌드마다 다름)
3. 디버거가 덤프를 열 때 PDB의 GUID를 읽음
4. Symbol Server에서 일치하는 GUID의 PDB를 자동 다운로드
5. 로컬 Symbol Cache에 저장하여 재사용

### 관련 문서

- [Microsoft Symbol Server 공식 문서](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/symbol-stores-and-symbol-servers)
- [Visual Studio 디버깅 가이드](https://learn.microsoft.com/en-us/visualstudio/debugger/specify-symbol-dot-pdb-and-source-files-in-the-visual-studio-debugger)

---

## 연락처

Symbol Server 관련 문제가 발생하면 팀 리더에게 문의하세요.

**서버 호스트**: NanSu (172.21.11.115)
