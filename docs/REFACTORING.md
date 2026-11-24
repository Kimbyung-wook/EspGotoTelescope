# SimpleGoTo 리팩토링 계획

2026-11-24, v0.3.1(커밋 `dc3f8eb`) 코드를 처음부터 끝까지 읽고 정리했습니다. 기능은 바꾸지 않고 구조만 고치는 계획입니다. 지자기·방위(DES-MAG)를 넣기 전에 2~4단계를 해 두면 그 작업이 훨씬 작아집니다(4절).

## 1. 지켜야 할 것

- **동작은 그대로:** 화면 글자, 저장 형식, 모터 지령이 같아야 합니다. 단계마다 단위 테스트, 시뮬레이터 테스트, `protocol_check.py`, `gen_remote_page.py --check`, 펌웨어 빌드 두 가지(기본, 원격 UI)를 돌립니다.
- **저장 형식(EEPROM)은 손대지 않음:** `config/settings.cpp`의 `fields()` 순서와 형식은 그대로 둡니다. 이미 저장된 설정을 그대로 읽어야 합니다.
- **보드 제약:**
  - IRAM 30,343 / 32,768 B(원격 UI 30,507 B)라 여유가 2.2 KB뿐입니다. 단계마다 늘어난 양을 적습니다.
  - 동적 할당과 STL은 쓰지 않습니다(지금처럼 고정 크기 멤버).
- **PC에서 테스트 가능하게:** 하드웨어를 부르지 않는 코드는 지금처럼 순수 C++로 둡니다.
- **한 단계 = 한 커밋:** 각 단계는 혼자서 끝나고, 되돌리기 쉬워야 합니다.

## 2. 지금 구조 (요약)

| 영역 | 파일 | 줄 수 | 메모 |
|---|---|---|---|
| UI | `ui/screens.cpp`, `screens.h`, `screen.h`, `simplegoto_ui.*`, `screen_manager.*` | 720 + 260 + 88 + 106 + … | 화면 18개가 한 파일에 있음 |
| 앱 상태 | `ui/ui_context.h` | 131 | `Telemetry`(측정·계산값), `UiState`(UI가 바꾸는 값) |
| 천문 | `astro/astro.*`, `target_tracker.h`, `target_telemetry.h` | 449 + 151 + 66 + 139 | |
| 위치·시각 | `location/location.*`, `app/loop_logic.h` | 57 + 57 + 113 | |
| 설정 | `config/settings.*`, `settings_binding.h`, `eeprom_store.h` | 243 + 124 + 94 + … | |
| 추정·보정 | `estimation/*` | ~480 | |
| 장치 | `display/oled.*`, `gps/gps.*`, `sensors/*`, `input/*`, `motor/*`, `remote/*` | | 오래된 코드 스타일이 섞여 있음 |
| 메인 루프 | `app.ino`, `tools/ui_simulator/runner/firmware_ui.cpp` | 266 + 162 | 같은 흐름이 두 곳에 있음 |

## 3. 발견한 문제

### A. 의존 방향이 거꾸로 (높음)
- `astro/target_telemetry.h`가 `ui/ui_context.h`를 include합니다. 계산 모듈(천문)이 UI 헤더에 기대고 있습니다.
- 추적 로직(`trackingDuty`, `trackingRate`, `trackingSign`, `applyTracking`)이 `astro`에 있으면서 `UiState`를 직접 고칩니다. 천문 계산이 아니라 앱 정책입니다.
- `config/settings_binding.h`도 `ui/ui_context.h`를 include합니다.
- `Telemetry`와 `UiState`는 메인 루프, 천문, 설정, 모터가 다 쓰는 **앱 전체 상태**인데 `ui/` 폴더에 있습니다.

### B. 메인 루프가 두 벌 (높음)
- `app.ino`의 `updateTelemetry()`와 `loop()`, 시뮬레이터의 `fillTelemetry()`와 `FirmwareUi::frame()`이 같은 순서를 따로 구현합니다. `loop_logic.h`로 일부는 모았지만 아직 두 곳에 남은 것이 있습니다:
  - 폰 시각 적용
  - 전압·Wi-Fi 상태 채우기
  - 자세 추정 호출
  - 설정 요청 처리
  - 모터 지령
- 부팅 화면 글자도 두 벌입니다. `firmware_ui.cpp`의 `bootLines()`는 `"Acc +-16G"`, `"Gyro 250 dps"`처럼 값을 손으로 적어 두어서, 펌웨어 설정이 바뀌면 둘이 어긋납니다.
- 최근 기능(폰 시각, 전압, 원격 상태)마다 두 곳을 같이 고쳐야 했습니다.

### C. 상태 구조체가 평평하고, 설정이 세 벌 (높음)
- `Telemetry`에는 필드가 50개쯤 한 줄로 늘어서 있습니다:
  - GPS 원시값
  - 쓰는 위치·시각
  - 달, 대상
  - 경통, 북극성
  - IMU, 전원, 원격
  - 설정 상태, Health
- `UiState`는 세 종류가 섞여 있습니다:
  - 저장하는 설정(보정값, 위치, 모터, 안내 허용 범위)
  - 실행 중 상태(추적 on/off, 정렬 확인, 수동·폰 시계)
  - 메인 루프에 보내는 요청(`settings_*_request`, `motor_inhibit`, `display_off`)
- 저장하는 값이 세 곳에 있습니다: `config::Settings`, `UiState`, 그리고 `applyToUi()`/`collectFromUi()`의 필드 목록. 설정 하나를 더하면 다섯 곳을 고칩니다:
  - 구조체 둘
  - `fields()`
  - `defaults()`/`sanitize()`
  - 바인딩 두 함수
- 기본값도 두 벌입니다. `UiState`의 초기값과 `config::defaults()`가 같아야 합니다(`track_target = 1  // Mercury, as UiState`).
- 지자기 필드(`mag_offset`, `mag_scale`, `declination_deg`)는 `Settings`에만 있고 `UiState`에는 없습니다. DES-MAG에서 바로 걸릴 문제입니다.

### D. `screens.cpp` 한 파일에 화면이 다 있음 (중간)
- 정보 줄과 항목이 섞인 화면 다섯 개(Polar Align, Settings, Location, Time, Target)가 같은 그리기 반복문을 각자 씁니다. "줄 번호 → 항목 번호 → 커서 글자" 계산을 매번 다시 합니다.
- 자이로·가속도 보정 화면의 `onClick`(적용/버리기/다시/취소)이 거의 같습니다.
- 커서, 선택지, 화면 클래스가 `screen.h`와 `screens.h`에 섞여 있습니다.

### E. 매직 넘버와 중복 함수 (낮음, 쉬움)
- `t.loc_source != 0`, `t.time_source != 0`이 `loc::SRC_NONE`/`TIME_NONE` 대신 쓰입니다. 위치는 `target_telemetry.h`의 `detail::known`, `trackingSign`, `screens.cpp`의 Polar Align 두 곳입니다.
- `screens.cpp`의 `fixText()`가 `gps/gps_link.h`의 `gpsFixText()`와 똑같습니다.
- 각도 변환 상수(`57.2957…`, `D2R`, `3.14159…`)가 `app/src`에 20번쯤 흩어져 있습니다.
- `clampD()`, `daysInMonth()`(달력 계산)가 UI 파일 안에 있습니다. 달력은 `astro`의 시간 함수 쪽 일입니다.

### F. 오래된 스타일의 장치 코드 (중간)
- `OLED`, `GPS`, `Tictok_us`, `LPF/HPF/ComplementaryFilter`가 나중에 쓴 `Imu`, `RaMotor`, `Rotary`와 스타일이 다릅니다:
  - snake_case 메서드(`setup_display`, `read_all`)
  - public 멤버(`gps_device`, `msg`, `dt`)
  - `new`/`delete`, bool을 `int`로 돌려줌
  - 쓰지 않는 `dt` 인자
- `app.ino`의 `OLED* monitor = new OLED()`, `GPS* gps = new GPS(...)`도 그냥 객체면 됩니다.
- GPS 링크가 살아 있는지(`gps_connection`)는 계산하지만 쓰는 곳이 없습니다.

### G. 쓰지 않거나 낡은 API (낮음)
- 테스트에서만 쓰는 것:
  - `TargetTracker::update(now, has_fix, utc, obs)`(앱은 `updateJd`만 씀)
  - `MoonState`/`moonState`
  - `HPF`
- 그 밖에:
  - `OLED::writeln`은 주석으로 막혀 있습니다.
  - `filter.h`의 `setTau`, `getXhigh`, `getXlow`는 쓰는 곳이 없습니다.
  - `utcFromGps()`가 `astro/target_tracker.h`에 있지만 GPS·시각 쪽 일입니다.

### H. 화면 글자 (낮음, 동작 변경이라 따로)
- 6RA Speed의 `RRcmd …DEG`은 단위가 PWM duty인데 `DEG`로 나옵니다. 이건 글자를 바꾸는 일이라 이 계획에 넣지 않고 따로 정합니다.

## 4. 단계

순서는 위험이 작고 이득이 큰 것부터입니다. 2~4단계를 하면 DES-MAG가 상태 하나, 설정 하나, 화면 하나만 더하는 일이 됩니다.

### 0단계: 한 번에 다 검사하는 스크립트 (작음)
- `scripts/check_all.sh`에서 다음을 차례로 돌리고, 실패하면 멈춥니다:
  - 단위 테스트, 시뮬레이터 빌드와 테스트, `protocol_check.py`, `gen_remote_page.py --check`
  - 펌웨어 빌드 두 가지. IRAM/IROM 숫자는 표로 출력합니다.
- 지금은 매번 명령 여섯 개를 손으로 돌리고 있습니다. 이후 단계의 검사 기준이 됩니다.

### 1단계: 작은 정리 (작음, 위험 낮음)
- `loc::hasPosition(t)`, `loc::hasTime(t)`를 만들고 `!= 0` 비교를 바꿈(E).
- `fixText()`를 지우고 `gpsFixText()`를 씀(E).
- `util/angles.h`(`DEG2RAD`, `RAD2DEG`)를 만들고 흩어진 상수를 바꿈(E).
- `daysInMonth()`를 `astro`의 달력 함수로 옮김(E).
- 쓰지 않는 API를 지움(G). 테스트는 `updateJd`/`targetState`로 바꿉니다. `HPF`는 지울지 남길지 정합니다. 지금은 테스트 말고 쓰는 곳이 없습니다.
- `utcFromGps()`를 `location/`(또는 `gps/`)으로 옮김(G).

### 2단계: 앱 상태를 `ui/`에서 꺼내기 (중간)
- `ui/ui_context.h`를 `app/app_state.h`로 옮깁니다. 이름(`Telemetry`, `UiState`, `UiContext`)은 이 단계에서는 그대로 두어 변경 폭을 줄입니다.
- 추적 정책(`trackingDuty`, `trackingRate`, `trackingSign`, `applyTracking`)을 `astro/target_telemetry.h`에서 `app/tracking.h`로 옮깁니다.
- 남은 `updateMoonTelemetry`, `updateTargetTelemetry`, `updateScopeTelemetry`는 `app/sky.h`로 옮깁니다.
- 결과: `astro/`는 순수 천문 계산만 남고 `ui/`도 `app/`도 include하지 않습니다. `config/`의 바인딩은 `app/`으로 갑니다(A).

### 3단계: 상태 구조체 묶기, 설정 한 벌로 (중간~큼)
- `Telemetry`를 하위 구조체로 묶습니다: `gps`, `when_where`(쓰는 위치·시각), `moon`, `target`, `scope`(경통·북극성), `imu`, `power`, `link`(원격), `health`. 화면 코드는 `t.moon_ra_deg` → `t.moon.ra_deg`처럼 기계적으로 바뀝니다.
- `UiState`가 `config::Settings cfg`를 멤버로 갖게 해서, 저장하는 값은 그 한 곳에만 둡니다:
  - `applyToUi()`/`collectFromUi()`는 사라지고 `st.cfg = s` 한 줄이 됩니다.
  - 기본값은 `config::defaults()` 하나만 남습니다.
  - `Settings`의 `uint8_t` 참/거짓 필드는 `bool`로 바꾸고 직렬화에서만 1바이트로 씁니다. 저장 형식은 그대로입니다.
- 실행 중 상태와 메인 루프 요청은 `UiState` 안에서 따로 묶습니다(`run`, `requests`).
- 지자기 필드도 자연히 `st.cfg.mag_*`로 화면에서 쓸 수 있게 됩니다(C).
- 위험: 바뀌는 줄이 많습니다(화면, 테스트). 단위·시뮬레이터 테스트가 화면 글자를 다 확인하므로 기계적 변경의 실수는 잡힙니다. 저장 형식 테스트(`test_settings.cpp`)는 그대로 통과해야 합니다.

### 4단계: 메인 루프를 한 벌로 (중간)
- `app::Core`(가칭)를 만듭니다:
  - 자세 추정기, 대상 추적기 둘, `LastGpsRecorder`, `SimpleGoToUi`, `UiContext`, 설정을 가짐
  - `step(const Inputs&, Frame&)`이 프레임 하나를 돌리고 모터 지령과 화면 켜기/끄기를 돌려줌
- `Inputs`에 담는 것: 시각(ms), GPS 원시값, IMU 원시값, 로터리 delta/click, 폰 시각, 전압, 원격 상태, 측정 시간.
- `app.ino`는 하드웨어를 읽어 `Inputs`를 채우고 결과를 하드웨어에 쓰기만 합니다. `firmware_ui.cpp`도 가짜 센서로 같은 일만 합니다.
- 부팅 화면 글자 목록을 `app::bootLines()` 하나로 모읍니다. IMU 범위 글자는 인자로 받습니다(B).
- 확인: 시뮬레이터 테스트가 그대로 통과하면 두 루프가 같다는 뜻입니다.

### 5단계: 화면 파일 나누기와 공통 틀 (중간)
- `ui/screens/` 아래로 나눕니다:
  - `menu`(Menu, Sleep), `status`(GPS…Health), `calibration`, `polar_align`, `target`, `settings`(Settings, Location, Time), `widgets`(MenuCursor, PageCursor, Choice)
- `PageScreen` 기반 클래스를 만듭니다. `PageCursor`와 정보 줄 수를 갖고 그리기 반복문을 한 번만 구현하며, 각 화면은 `renderLine(f, line, index, mark, ctx)`만 씁니다(D).
- 보정 화면 둘의 "결과 → 적용/버리기, 실패 → 다시/나가기, 측정 중 → 취소" 흐름을 작은 도우미 하나로 묶습니다(D).
- `run_tests.sh`, `sources.sh`의 파일 목록을 고칩니다. 가상 함수가 늘어도 IRAM은 거의 그대로일 것으로 봅니다(코드는 flash). 측정해서 적습니다.

### 6단계: 오래된 장치 코드 정리 (작음~중간)
- `OLED`, `GPS`를 `Imu`/`RaMotor` 스타일로 바꿉니다:
  - `begin()`이 `bool`을 돌려줌
  - private 멤버, `new` 없이 멤버 객체
- GPS 원시값은 `GpsFix` 구조체로 돌려주어 `app.ino`가 `Adafruit_GPS` 필드를 직접 읽지 않게 합니다(F).
- `Tictok_us`를 `StopwatchUs`로 바꿉니다(const 조회).
- 필터의 쓰지 않는 `dt` 인자와 메서드를 정리합니다(F, G).
- 선택: GPS 링크 끊김을 Status > GPS에 보여 주기. 이건 화면 변경이라 따로 정합니다.

### 7단계(선택): 테스트 역할 나누기
- 단위 `test_ui.cpp`(594줄)와 시뮬레이터 `test_menu.cpp`(611줄)가 겹칩니다(메뉴 진입·복귀, 보정, 대상 화면).
- 단위 테스트는 화면 하나의 동작, 시뮬레이터 테스트는 프로토콜과 루프 전체(센서 → 계산 → 화면 → 모터)를 맡게 하고, 겹치는 확인은 한쪽에만 둡니다.

## 5. 하지 않는 것

- 저장 형식, 화면 글자, 메뉴 구조는 바꾸지 않습니다(바꿀 일은 3절 H처럼 따로 정함).
- `Arduino/archive/`(예전 코드 46개 파일)는 보관용이라 건드리지 않습니다. 지울지는 따로 정합니다.
- 원격 UI의 손으로 쓴 JSON 처리는 작고 테스트되어 있어 그대로 둡니다.

## 6. 크기 어림

| 단계 | 바뀌는 파일 | 어림 |
|---|---|---|
| 0 | 스크립트 1 | 작음 |
| 1 | 6~8 | 작음 |
| 2 | 10 안팎(include 경로) | 중간 |
| 3 | 화면, 테스트, 바인딩 대부분 | 큼 |
| 4 | `app.ino`, `firmware_ui.cpp`, 새 `app/core.*` | 중간 |
| 5 | `ui/` 전체, 빌드 목록 | 중간 |
| 6 | `display/`, `gps/`, `util/`, `estimation/filter.*` | 작음~중간 |
| 7 | 테스트 두 파일 | 중간 |
