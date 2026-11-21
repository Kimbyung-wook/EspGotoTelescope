# OLED UI / UX 설계

SSD1306 OLED(128×64)와 로터리 엔코더 하나로 망원경을 조작하는 화면 구성입니다.
코드 위치: `Arduino/app/src/ui/` (화면 로직), `Arduino/app/src/display/oled.*` (출력). 구조는 아래 "10. 코드 구조"를 보세요.

## 1. 하드웨어 제약

| 항목 | 값 |
|---|---|
| 디스플레이 | SSD1306 128×64, I2C 0x3C, 400 kHz |
| 글자 | 기본 폰트 6×8 px, 텍스트 크기 1 |
| 화면 크기 | **8줄 × 21자** (`DISP_LINE_COLUMN` = 8, `DISP_LINE_ROW` = 22, 널 문자 포함) |
| 입력 | 로터리 엔코더(S1=D6, S2=D7) + 누름 버튼(KEY=D4), 모두 인터럽트로 처리 |
| 갱신 주기 | 메인 루프 10 Hz. 매 프레임 화면 전체를 다시 그림 |

## 2. 화면 레이아웃

모든 화면은 세 영역으로 나뉩니다.

```
줄0   T XX:XX:XX.X VXX.XX     ← 상태바: GPS 시각(0.1초 단위) + 전원 전압
줄1   *1RA Speed              ← 제목줄: '*' + 화면 번호/이름
줄2~7 ...                     ← 내용 영역 6줄 (메뉴 항목 또는 측정값)
```

- **상태바(줄0)**: 어느 화면에서든 늘 보입니다. 전압은 아직 고정값 `12.60`이고, 측정 회로가 연결되면 실제 값으로 바뀔 자리입니다.
- **제목줄(줄1)**: 메인 메뉴 문자열을 그대로 재사용하고, 커서 자리에 `*`를 넣어 지금 어느 화면에 있는지 표시합니다.
- **내용 영역(줄2~7)**: 한 번에 6줄을 보여줍니다. 이보다 긴 목록은 스크롤 창으로 보여줍니다.

## 3. 조작 규칙

| 입력 | 동작 |
|---|---|
| 로터리 회전 | 커서 위/아래 이동. 값 편집 모드에서는 값 증감 |
| 버튼 클릭 | 선택한 항목으로 들어가기 / 편집 확정 / 정보 화면에서 나가기 |

- **커서 표시**: 선택된 줄 맨 앞에 `>`가 붙습니다. 매 프레임 `>`와 `-`를 번갈아 그려 깜박이므로(10 Hz 루프 → 5 Hz 깜박임) 작은 화면에서도 커서 위치가 잘 보입니다.
- **편집 모드**: 값 앞의 커서가 깜박이는 상태가 편집 중이라는 뜻입니다. 로터리 1틱마다 값이 1씩 바뀝니다(`ra_speed_select`).
- **스크롤**: 메인 메뉴는 10개 항목 가운데 6개만 보입니다. 커서가 맨 아래 줄에 닿은 뒤에도 계속 돌리면 목록이 한 칸씩 올라갑니다.
- **위치 기억**: 하위 화면에서 나오면 커서가 방금 들어갔던 항목 위로 돌아갑니다. 화면을 스택으로 관리하고 각 화면이 자기 커서를 기억합니다.
- **하위 메뉴 진입**: 하위 메뉴에 들어가면 커서가 첫 줄에서 시작합니다.
- **나가기**: 항목이 없는 정보 화면은 버튼 클릭 한 번으로 메인 메뉴로 돌아갑니다. 항목이 있는 화면은 `Exit`/`Quit` 항목을 선택해서 나갑니다.

## 4. 부팅 화면

전원을 켜면 장치를 하나씩 초기화하면서 결과를 한 줄씩 쌓아 보여줍니다. 실패한 장치는 `>>`로 시작하는 줄로 표시됩니다.

```
1. Display is ON
2. IMU is ON          (실패: >> IMU failed)
Acc +-16G
Gyro 250 dps
Mag 10 Hz
3. GPS is ON          (실패: >> GPS is timeout)
4. Buttons are ON
5. GPIO/PWM is ON
```

## 5. 메뉴 구조

```
Main Menu
├─ 1 RA Speed ─────── RRcmd(클릭 → 속도 편집 모드) / RRate / Exit
├─ 2 Status-GPS
├─ 3 Status-Acc/Gyro
├─ 4 Status-Mag/Alt
├─ 5 Status-IMU Calib
├─ 6 Do IMU Calib. ── Acc Bias / Acc Scale / Gyro Bias / Gyro Scale / Quit
├─ 7 Att.Estimator    (상태 + 필터 파라미터, 스크롤)
├─ 8 Health
├─ 9 Axis for Moon
└─ Sleep
```

## 6. 화면별 설계

수치 칸은 `XXXX.XXXX`(`%9.4f`)를 기본 폭으로 맞춥니다.

### Main Menu
```
T XX:XX:XX.X VXX.XX
Main Menu
>1RA Speed
 2Status-GPS
 3Status-Acc/Gyro
 4Status-Mag/Alt
 5Status-IMUCalib
 6Do IMU Calib.
 7Att.Estimator
 8Health
 9Axis for Moon
 Sleep
```
6줄 스크롤 창으로 보여줍니다.

### 1 RA Speed — 적경 축 모터 속도
```
T XX:XX:XX.X VXX.XX
*1RA Speed
>RRcmd XXXX.XXXXDEG     ← 클릭하면 편집 모드
>RRate XXXX.XXXXDEG     ← 자이로로 잰 실제 회전 속도
>Exit
 ROL XXXX.XXXX   DEG
 LAT NXXX.XXXXXXDEG
 PIT XXXX.XXXX   DEG
```
위 3줄은 선택할 수 있는 항목이고, 아래 3줄은 극축 정렬 참고값(롤, 위도, 피치)입니다. 극축 정렬을 하려면 피치를 위도에 맞춥니다.

### 2 Status-GPS
```
T XX:XX:XX.X VXX.XX
*2Status-GPS
Date 20XX/XX/XX
Time   HH:MM:SS.SSS
XXXXX Sat XX            ← Fix 상태(NoFix/GPS/DGPS) + 위성 수

 N XXX.XXXXXXXX DEG
 E XXX.XXXXXXXX DEG
```

### 3 Status-Acc/Gyro
```
T XX:XX:XX.X VXX.XX
*3Status-Acc/Gyro
Ax    XXXX.XXXX mpss
Ay    XXXX.XXXX mpss
Az    XXXX.XXXX mpss
Wx    XXXX.XXXX dps
Wy    XXXX.XXXX dps
Wz    XXXX.XXXX dps
```

### 4 Status-Mag/Alt
```
T XX:XX:XX.X VXX.XX
*4Status-Mag/Alt
Mx    XXXX.XXXX uT
My    XXXX.XXXX uT
Mz    XXXX.XXXX uT
HEAD  XXXX.XXXX DEG     ← 지자기 방위
```

### 5 Status-IMU Calib — 현재 바이어스 값
```
T XX:XX:XX.X VXX.XX
*5Status-IMU Calib
bAx   XXXX.XXXX mpss
bAy   XXXX.XXXX mpss
bAz   XXXX.XXXX mpss
bWx   XXXX.XXXX dps
bWy   XXXX.XXXX dps
bWz   XXXX.XXXX dps
```

### 6 Do IMU Calib.
```
T XX:XX:XX.X VXX.XX
*6Do IMU Calib.
>Acc. Bias Calib
 Acc. Scale Calib
 Gyro Bias Calib
 Gyro Scale Calib

 Quit
```
처음 설계는 `Exit`만 있는 화면이었고, 코드에는 보정 항목 4개가 들어가 있습니다.
Gyro Bias 보정은 정지 상태에서 진행합니다. 0–1초에 초기값을 잡고, 1–6초에는 α=0.3, 6–16초에는 α=0.01로 지수 평균을 낸 뒤 `Done`을 표시합니다.

### 7 Att.Estimator — 자세 추정기 상태와 튜닝 값
```
T XX:XX:XX.X VXX.XX
*7Att.Estimator
 Roll  XXXX.XXXXDEG
 PITCH XXXX.XXXXDEG
 YAW   XXXX.XXXXDEG
 R_R   XXXX.XXXX        ← 측정 잡음 R (Roll/Pitch/Yaw)
 R_P   XXXX.XXXX
 R_Y   XXXX.XXXX
 Q_R   XXXX.XXXX        ← 프로세스 잡음 Q
 Q_P   XXXX.XXXX
 Q_Y   XXXX.XXXX
 K_R   XXXX.XXXX        ← 칼만 게인 K
 K_P   XXXX.XXXX
 K_Y   XXXX.XXXX
>Exit
```
화면 하나에 15줄이 필요하므로 메인 메뉴처럼 스크롤 창을 씁니다. 나중에 R/Q를 로터리로 바로 튜닝할 수 있게 하는 것을 염두에 둔 구성입니다.

### 8 Health — 구간별 실행 시간 [µs]
```
T XX:XX:XX.X VXX.XX
*8Health
Loop XXXXXX/XXX.XHz     ← 한 주기 전체 시간 / 목표 주기
MAIN XXXXXX
DISP XXXXXX
GPS  XXXXXX
IMU  XXXXXX
EST  XXXXXX
```

### 9 Axis for Moon — 마운트 자세와 목표(달) 좌표
```
T XX:XX:XX.X VXX.XX
*9Axis for Moon
Roll  XXXX.XXXX DEG
Pitch XXXX.XXXX DEG
Yaw   XXXX.XXXX DEG
-- Moon --------
R.A.  XXXX.XXXX DEG
DEC.  XXXX.XXXX DEG
```

### Sleep
```
T XX:XX:XX.X VXX.XX
(Sleep)
```
상태바만 남기고 내용 영역을 비웁니다. 버튼을 누르면 메인 메뉴로 돌아갑니다.

## 7. 설계와 현재 코드의 차이

| 화면 | 설계 | 현재 코드 (`app.ino`) |
|---|---|---|
| 숫자 서식 | `XXXX.XXXX` (`%9.4f`) | `%10.5f` (`ws/menu.ino`는 `%9.4f`) |
| 4 Mag/Alt | HEAD(방위) | 방위 대신 온도(Temp) 표시 |
| 5 IMU Calib | 바이어스 값 | 자이로 바이어스는 Gyro Bias Calib 결과를 표시. 가속도 바이어스는 보정 기능이 없어 0. 측정값에는 아직 적용하지 않음 |
| 7 Att.Estimator | Roll/Pitch/Yaw + R/Q/K + Exit, 스크롤 | 가속도 기반 Roll/Pitch와 Wx/Wy만 표시. 추정기 미구현 |
| 9 Axis for Moon | 추정 자세 + 달 RA/Dec | 롤/피치는 가속도 값, Yaw·RA·Dec는 임시 고정값 |
| Sleep | `(Sleep)` 표시 | 빈 화면. 디스플레이 전원은 끄지 않음. 나오면 커서가 Sleep 항목에 있음 |
| 1 RA Speed | RRcmd/RRate 단위 DEG | RRate는 dps로 표시. RRcmd는 PWM 듀티 값 |

## 8. 이후 구조 구상 (`Arduino/ws/menu_display.h`, 미완성)

`switch` 문 하나에 몰려 있는 메뉴 로직을 `MenuDisplay` 클래스로 옮기려던 초안입니다. 계층형 메뉴로 다시 구성하는 안이 담겨 있습니다.

```
Main View / Motor Settings / Sensor Status / Filter Status / Sensor Calibration / Statics
Sensor 하위: Show Acc/Gyro/Mag Raw, Show Acc/Gyro Bias, Show Mag Calib,
             Calib Acc Bias, Calib Gyro Bias, Calib Mag Iron
```
메뉴 단계(`menu_level[2]`: 계층, 항목)와 공용 항목 `[Back]`, `[To Top]`을 두는 설계입니다. 현재는 문법 오류가 있어 컴파일되지 않습니다.

## 9. UI의 변천

1. `ws/gps_imu_oled_button.cpp`: 버튼 하나로 GPS → IMU → EST → PERFORM 페이지를 차례로 넘김
2. `ws/menu.ino`, `app/app.ino`: 로터리 + 버튼 기반 메뉴, 상태바/제목줄/내용 영역 레이아웃 정립
3. `ws/menu_display.h`: 계층형 메뉴 클래스로 일반화하는 구상

## 10. 코드 구조

| 파일 | 역할 |
|---|---|
| `src/ui/frame.h` | 한 화면 분량(8줄×22바이트) 버퍼. `set(row, fmt, ...)`은 줄 너비를 넘으면 잘라냄 |
| `src/ui/ui_context.h` | `Telemetry`(UI가 보여줄 값, 메인 루프가 매 프레임 채움), `UiState`(UI가 바꾸는 값: RA 속도 명령, 바이어스) |
| `src/ui/screen.h` | `Screen` 인터페이스(`onEnter`/`onRotate`/`onClick`/`tick`/`render`)와 `MenuCursor`(선택·스크롤 창) |
| `src/ui/screen_manager.*` | 화면 스택(push = 들어가기, pop = 나가기)과 커서 깜박임 |
| `src/ui/screens.*` | 각 화면 클래스 |
| `src/ui/telescope_ui.*` | 화면들을 묶은 `TelescopeUi`. 메인 루프는 프레임마다 `ui.step(회전량, 클릭, ctx, frame)`만 호출 |

UI 코드는 하드웨어에 의존하지 않아서 PC에서도 빌드됩니다. 단위 테스트(`Arduino/test/test_ui.cpp`)와 UI 시뮬레이터가 같은 코드를 씁니다.

화면을 추가하려면 `Screen`을 상속한 클래스를 만들고, `TelescopeUi`에 멤버로 넣은 뒤 메뉴 항목에 연결하면 됩니다.
