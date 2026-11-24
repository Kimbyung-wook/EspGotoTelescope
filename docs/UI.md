# OLED UI / UX 설계

SimpleGoTo(망원경 적도의 제어 펌웨어)를 SSD1306 OLED(128×64)와 로터리 엔코더 하나로 조작하는 화면 구성입니다.
코드 위치: `Arduino/app/src/ui/` (화면 로직), `Arduino/app/src/display/oled.*` (출력). 구조는 아래 "10. 코드 구조"를 보세요.
메뉴 구성의 근거는 [REQUIREMENTS.md](REQUIREMENTS.md) DES-UI-01입니다.

## 1. 하드웨어 제약

| 항목 | 값 |
|---|---|
| 디스플레이 | SSD1306 128×64, I2C 0x3C, 400 kHz |
| 글자 | 기본 폰트 6×8 px, 텍스트 크기 1 |
| 화면 크기 | **8줄 × 21자** (`Frame::LINES` = 8, `Frame::COLS` = 22, 널 문자 포함) |
| 입력 | 로터리 엔코더(S1=D6, S2=D7) + 누름 버튼(KEY=D4), 모두 인터럽트로 처리 |
| 갱신 주기 | 메인 루프 10 Hz. 매 프레임 화면 전체를 다시 그림 |

## 2. 화면 레이아웃

모든 화면은 세 영역으로 나뉩니다.

```
줄0   T XX:XX:XX.X V XX.XXW   ← 상태바: 시각 출처 + 시각(0.1초 단위) + 전원 전압 + Wi-Fi 표시
줄1   *1Target                ← 제목줄: '*' + 화면 이름
줄2~7 ...                     ← 내용 영역 6줄 (메뉴 항목 또는 측정값)
```

- **상태바(줄0)**: 어느 화면에서든 늘 보입니다.
  - 첫 글자는 시각 출처입니다: `T` GPS, `M` 손으로 맞춘 시각(DES-LOC-04). 시각이 없으면 `T  0: 0: 0.0`입니다.
  - 시각은 UTC입니다(지역 시각은 5 Settings → Time 화면에서 봅니다).
  - 전압은 아직 고정값 `12.60`이고, 측정 회로가 연결되면 실제 값으로 바뀔 자리입니다.
  - 마지막 칸은 폰 원격 UI 표시입니다: 원격 UI를 끈 빌드는 비어 있고, AP만 켜져 있으면 `w`, 폰이 붙어 있으면 `W`입니다.
- **제목줄(줄1)**: 메인 메뉴 문자열 앞에 `*`를 붙여 지금 어느 화면에 있는지 표시합니다. 대상 화면은 대상 이름과 상태(`computing...`, `no loc/time`, `ON TARGET`)를 보여 줍니다.
- **내용 영역(줄2~7)**: 한 번에 6줄을 보여줍니다. 이보다 긴 화면은 스크롤 창으로 보여줍니다.

## 3. 조작 규칙

| 입력 | 동작 |
|---|---|
| 로터리 회전 | 커서 위/아래 이동. 값 편집 모드에서는 값 증감 |
| 버튼 클릭 | 선택한 항목으로 들어가기 / 편집 확정 / 정보 화면에서 나가기 |

- **커서 표시**: 선택된 줄 맨 앞에 `>`가 붙습니다. 매 프레임 `>`와 `-`를 번갈아 그려 깜박이므로(10 Hz 루프 → 5 Hz 깜박임) 작은 화면에서도 커서 위치가 잘 보입니다.
- **편집 모드**: 값 앞의 커서가 깜박이는 상태가 편집 중이라는 뜻입니다. 한 번 더 클릭하면 끝납니다. 위치 입력처럼 자리를 옮기며 고치는 값은 고치는 줄 앞에 `*`가 붙고 제목줄에 지금 자리(`*Lat step 0.1`)를 보여 줍니다.
- **두 가지 고르기**: 보정 결과처럼 `Apply`/`Discard`, `Retry`/`Quit`를 한 줄에 두는 곳은 다이얼을 돌려 고르고 클릭합니다.
- **스크롤**: 메뉴는 커서가 맨 아래 줄에 닿은 뒤에도 계속 돌리면 목록이 한 칸씩 올라갑니다. 정보 줄과 항목이 섞인 화면(Target, Polar Align, Settings, Location, Time)은 커서를 따라 화면 전체가 스크롤되고, 첫 항목에서 위로 더 돌리면 맨 위 정보 줄까지 올라갑니다. 이때는 선택된 항목이 없고 클릭해도 아무 일이 없습니다(`PageCursor`).
- **위치 기억**: 하위 화면에서 나오면 커서가 방금 들어갔던 항목 위로 돌아갑니다. 화면을 스택으로 관리하고 각 화면이 자기 커서를 기억합니다.
- **하위 메뉴 진입**: 하위 메뉴에 들어가면 커서가 첫 줄에서 시작합니다(대상 목록은 지금 고른 대상에서 시작).
- **나가기**: 정보 화면은 버튼 클릭 한 번으로 나갑니다. 항목이 있는 화면은 `Back`/`Quit` 항목을 선택해서 나갑니다.
- **모터 정지**(DES-SYS-02): 보정 화면과 Polar Align 화면이 맨 위에 있는 동안 RA 모터 출력은 0입니다. 나가면 그 전 상태(추적, RRcmd)로 돌아갑니다.

## 4. 부팅 화면

전원을 켜면 첫 줄에 이름과 버전(`app/src/version.h`)을 보여주고, 장치를 하나씩 초기화하면서 결과를 한 줄씩 쌓아 보여줍니다. 실패한 장치는 `>>`로 시작하는 줄로 표시됩니다. 화면이 차면(원격 UI를 켠 빌드는 9줄) 첫 줄을 남기고 위로 밀어 올립니다.

```
SimpleGoTo v0.2.0     ← 이름과 버전
1. Display ON 160MHz  ← CPU 클럭 (ESP.getCpuFreqMHz)
2. IMU is ON          (실패: >> IMU failed)
Acc +-16G
Gyro 250 dps
Mag 10 Hz
3. GPS is ON          (실패: >> GPS is timeout)
4. Buttons are ON
5. GPIO/PWM is ON
6. Settings loaded    ← 저장된 설정 (처음: Settings new, 손상: >> Settings bad->def)
7. WiFi AP SimpleGoTo ← REMOTE_UI_ENABLE 1일 때만
```

## 5. 메뉴 구조

```
Main Menu
├─ 1Target ────────── Moon / Mercury / Venus / Mars / Jupiter / Saturn / Quit
│                     └─ 대상 화면: 대상·경통 좌표, dRA/dDEC 안내, Track / Rate / Offset / Back
├─ 2Polar Align ───── 고도 오차, 북극성 위치, Confirm aligned / Back
├─ 3Status ────────── GPS / Acc/Gyro / Mag/Temp / IMU Calib / Att.Estimator / Health / Back
├─ 4Calibration ───── Gyro Bias / Accel 6-pos / Back
├─ 5Settings ──────── RAduty = RRcmd / Location > / Time > / Motor dir / Guide tol / Save / Reset defaults / Back
│                     ├─ Location: Mode / Lat / Lon / Alt / Copy GPS fix / Save / Back
│                     └─ Time: Year … Second / UTC offset / Set clock / Back
├─ 6RA Speed ──────── RRcmd(클릭 → 속도 편집) / RRate / Quit
└─ Sleep
```

쓰는 순서대로 묶었습니다: 대상 고르기와 추적이 맨 앞, 처음 한 번 하는 일(극축 정렬, 보정, 설정)이 그 뒤, 모터를 손으로 돌리는 RA Speed는 모터 보정용으로 뒤에 둡니다.

## 6. 화면별 설계

### Main Menu
```
T 12: 0: 0.0 V 12.60
Main Menu
>1Target
 2Polar Align
 3Status
 4Calibration
 5Settings
 6RA Speed
 Sleep                  ← 스크롤해서 보임
```

### 1Target — 대상 선택, 안내, 추적
대상 목록(7개, 6줄 스크롤 창). 들어가면 커서가 지금 고른 대상 위에 있습니다. 추적 중인 대상 옆에 `TRACKING`이 붙습니다.
```
*1Target
 Moon
>Mercury
 Venus
 Mars     TRACKING
 Jupiter
 Saturn
 Quit                   ← 스크롤해서 보임
```
다른 대상을 고르면 추적이 꺼지고 추적 방식이 그 대상의 기본값(달은 `Lunar`, 나머지는 `Sidereal`)으로 바뀝니다(DES-TRK-01). 같은 대상을 다시 고르면 그대로입니다.

대상 화면은 정보 7줄 + 항목 4개(11줄)를 6줄 창으로 봅니다. 들어가면 커서가 `Track`에 있고 창은 이렇게 보입니다.
```
*Mars                    ← 고른 직후 한 프레임: *Mars    computing...
T.R.A. 187.48143 DEG?    ← 경통이 가리키는 RA ('?' = 극축 정렬 확인 전)
T.DEC.  52.43350 DEG?
dRA   -145.66 min W      ← 대상까지 RA 방향으로 돌릴 양(시간의 분)과 방향
dDEC   -38.38 deg S      ← 적위 방향으로 돌릴 양과 방향
ALT -28.09 AZ  40.75     ← 대상의 고도·방위(북 0°, 동 +)
>Track OFF 15.0411"/s    ← 추적 켜기/끄기와 지금 추적 속도
```
첫 항목에서 위로 돌리면 대상 좌표 두 줄이 보입니다.
```
*Mars
R.A.   151.06717 DEG     ← 대상의 날짜 기준 겉보기 좌표
DEC.    14.04876 DEG
T.R.A. 187.48143 DEG?
...
```
아래로 돌리면 나머지 항목이 보입니다.
```
 Track ON  15.0411"/s
 Rate Sidereal           ← 클릭하면 Sidereal ↔ Lunar
 Offset    +20.0 PWM     ← 클릭하면 편집 (다이얼 한 칸 = PWM 1)
>Back   (RA uncal)       ← RAduty가 0(보정 전)이면 표시
```
- **위치와 시각**이 없으면(DES-AST-01) 제목이 `*Mars    no loc/time`이고 좌표·안내 줄이 `---`입니다. 위치는 GPS → 직전 GPS → 수동 값 순서, 시각은 GPS → 수동 시각 순서로 씁니다(DES-LOC-01, 04).
- **대상 좌표**(R.A./DEC.): 광행시간, 세차, 관측지 시차를 보정한 날짜 기준 좌표입니다. 1초마다 다시 계산합니다.
- **경통 좌표**(T.R.A./T.DEC., DES-PNT-02): 경통에 붙인 IMU의 롤/피치(자세 추정기 값)로 매 프레임 계산합니다. 극축이 정렬된 적도의를 가정하므로, 2Polar Align에서 정렬을 확인하기 전에는 끝에 `?`가 붙습니다.
  - 중력만으로는 자오선 기준 거울상 두 자세를 구별할 수 없어서, 고른 대상에 가까운 쪽을 보여줍니다. 피어 뒤집힘(meridian flip) 자세도 같은 하늘 방향으로 접어서 계산합니다.
  - IMU 축 방향은 `board_config.h`의 `IMU_TUBE_OPTICAL_AXIS`(광축), `IMU_TUBE_DEC_AXIS`(적위축)으로 정합니다. 경통을 천구 북극에 맞추고 T.DEC.가 +90 근처인지 확인합니다.
  - 정확도는 가속도계 수준(0.2~0.5°)이고, 극축 정렬 오차가 그대로 더해집니다. 지자기를 쓰는 방법 A(DES-PNT-01)는 아직 없습니다.
- **안내**(DES-PNT-04): `dRA` = 대상 RA − 경통 RA(±180°로 접음)를 시간의 분(1° = 4분)으로, `E`(+)/`W`(−)와 함께 보여 줍니다. `dDEC`는 도 단위, `N`/`S`입니다. `|dRA·cos Dec|`와 `|dDEC|`가 모두 허용 범위(5Settings의 Guide tol, 기본 1°) 안이면 제목에 `ON TARGET`이 붙습니다.
- **추적**(DES-TRK-01, 02): `Track`을 클릭하면 켜고 끕니다. 추적은 대상이 아니라 지금 경통이 가리키는 곳을 고른 속도로 따라갑니다. `Sidereal`은 위치·시각 없이도 돌고, `Lunar`는 달 계산(위치·시각 필요)이 있어야 돕니다(없으면 `Track ON    ---`, 모터 지령은 그대로).
- **모터 지령**(DES-MOT-01, 02): 추적이 켜져 있으면 RRcmd를 `방향 × (RAduty × 속도 / 15.041 + Offset)`으로 덮어씁니다. 방향은 남반구(위도 < 0)에서 −1, 5Settings의 `Motor dir Reverse`면 한 번 더 뒤집습니다. RAduty가 0이면 Offset만으로 모터가 돕니다.
- 추적은 화면에서 나가도 계속됩니다. 다른 대상을 고르거나 6RA Speed에서 RRcmd를 편집하면 꺼집니다. Offset은 대상을 바꿔도 유지됩니다.

### 2Polar Align — 극축 정렬 보조 (DES-PA)
들어가면 모터를 멈춥니다. 정보 6줄 + 항목 2개를 6줄 창으로 봅니다.
```
*2Polar Align
Dec 90: tube // axis     ← 첫 줄(위로 돌리면 보임): Dec를 90°로, 경통을 극축과 나란히
ALT +36.00 LAT +37.50    ← 경통 광축의 고도(중력)와 관측지 위도
dALT  -1.50 UP           ← 고도 - 위도. 극축을 올릴지(UP) 내릴지(DOWN), 0.5° 안이면 OK
dAZ    --- no mag        ← 방위 오차: 지자기(DES-MAG) 전까지 없음
Polaris at  1:19         ← 북극성이 천구 북극 둘레 어느 시계 방향에 있는지
State: adjust            ← adjust / ALT OK. AZ: Polaris / ALIGNED
>(Confirm: wait OK)      ← OK가 2초 유지되면 Confirm aligned, 확인 후에는 Clear aligned
 Back
```
- 고도 오차 `|dALT|`가 0.5°(`PA_ALT_TOL_DEG`) 안에서 2초(`PA_OK_HOLD_MS`) 넘게 유지되면 `Confirm aligned`를 누를 수 있습니다. 누르면 "정렬됨"이 켜지고 대상 화면의 `?`가 없어집니다. 전원을 끄면 꺼집니다.
- 방위는 아직 지자기가 없어서 고도만 확인합니다. 방위는 북극성 위치를 보고 맞춥니다(`ALT OK. AZ: Polaris`).
- **북극성 시계**(DES-PA-03, DES-AST-02): J2000 좌표(RA 02h31m49s, Dec +89°15′51″)를 날짜로 세차 보정한 뒤 시간각 h로 시계 위치 = 12 − h/30°(12시 = 극 바로 위)를 구합니다. **맨눈으로 북쪽 하늘을 본 방향**입니다. 상이 뒤집히는 극축 망원경에서는 6시간 돌려서 읽습니다. 위도가 북반구일 때만 보여 줍니다.
- 위치가 없으면 `LAT    ---`, `dALT   --- no loc`입니다.

### 3Status — 상태 화면
```
*3Status
>GPS
 Acc/Gyro
 Mag/Temp
 IMU Calib
 Att.Estimator
 Health
 Back                   ← 스크롤해서 보임
```
각 화면은 정보만 보여 주고, 클릭하면 3Status로 돌아갑니다.

```
*GPS                     *Acc/Gyro                *Mag/Temp
Date: 2026/11/21         Ax    XXXX.XXXXX mpss    Mx   XXXX.XXXXX uT
Time:   12:00:05.400     Ay    XXXX.XXXXX mpss    My   XXXX.XXXXX uT
  GPS Sat  8             Az    XXXX.XXXXX mpss    Mz   XXXX.XXXXX uT
Pos GPS  Time GPS        Wx    XXXX.XXXXX dps     Temp XXXX.XXXXX 'c
 N  37.56650000 DEG      Wy    XXXX.XXXXX dps
 E 126.97800000 DEG      Wz    XXXX.XXXXX dps
```
- GPS 화면 4번째 줄은 Fix 상태(NoFix/GPS/DGPS)와 위성 수, 5번째 줄은 지금 쓰는 위치 출처(`GPS`/`LAST`/`MAN`/`---`)와 시각 출처(`GPS`/`MAN`/`---`)입니다(DES-LOC-01).
- Acc/Gyro, Mag/Temp는 보정 전 원시값입니다.

```
*IMU Calib               *Att.Estimator           *Health
Ax b +0.1250 s1.0100     ROLL   XXXX.XXXXX DEG    Loop XXXXXX/XXX.XHz
Ay b +0.0000 s1.0000     PITCH  XXXX.XXXXX DEG    MAIN XXXXXX
Az b +0.0000 s1.0000     aR XXXX.XX aP XXXX.XX    DISP XXXXXX
bWx  XXXX.XXXXX dps      Wx     XXXX.XXXXX DPS    GPS  XXXXXX
bWy  XXXX.XXXXX dps      Wy     XXXX.XXXXX DPS    IMU  XXXXXX
bWz  XXXX.XXXXX dps      Wz     XXXX.XXXXX DPS    EST  XXXXXX AST XXXXX
```
- IMU Calib: 가속도 바이어스(b, m/s²)와 배율(s), 자이로 바이어스(°/s). 4Calibration에서 적용한 값입니다.
- Att.Estimator: 상보필터(τ = 1 s) Roll/Pitch, 가속도만으로 구한 aR/aP, 보정된 Wx/Wy/Wz. 입력은 보정된 가속도·자이로입니다(DES-ATT-01).
- Health: 구간별 실행 시간[µs]. AST는 마지막 천체 계산 시간입니다.

### 4Calibration — 보정 (DES-CAL)
```
*4Calibration
>Gyro Bias
 Accel 6-pos
 Back
```
두 보정 모두 화면에 있는 동안 모터를 멈추고, 결과를 보여 준 뒤 `Apply`를 골라야 적용·저장합니다. 나가거나 `Discard`를 고르면 이전 값이 남습니다.

**Gyro Bias** (DES-CAL-03): 1초 안정 대기 뒤 30초(300개) 동안 축별 평균과 표준편차를 잽니다.
```
*Gyro Calib              *Gyro Calib              *Gyro Calib
Keep still  0/30 s       Measuring 15/30 s        Done. Apply?
Wx    +0.50000 dps       bWx   +0.50000 dps       bWx   +0.50000 dps
Wy    -0.25000 dps       bWy   -0.25000 dps       bWy   -0.25000 dps
Wz    +0.12500 dps       bWz   +0.12500 dps       bWz   +0.12500 dps
                         sd 0.010 0.000 0.000     sd 0.010 0.000 0.000
>Cancel                  >Cancel                  >Apply   Discard
```
측정 중 충격(DES-CAL-02: |ω|가 평균에서 2 °/s, |a|가 0.5 m/s² 넘게 벗어남, `CAL_SHOCK_*`)이 나면 `Shock detected.`와 `Retry`/`Quit`를 보여 줍니다.

**Accel 6-pos** (DES-CAL-04~06): X+, X−, Y+, Y−, Z+, Z− 순서로 그 축을 위로 향하게 둡니다. 자세가 맞으면(그 축 0.8 g 넘게, 나머지 0.35 g 안) 1초 기다린 뒤 5초 잽니다.
```
*Accel Calib             *Accel Calib
Step 1/6 X+ up           Done. Apply?
Turn X+ up, hold         X b +0.1200 s1.0100      ← 바이어스(m/s²)와 배율
a  +9.8  +0.0  +0.1      Y b -0.0800 s0.9900
Detach IMU, lay it       Z b +0.2500 s1.0200
flat, same axes back
>Cancel                  >Apply   Discard
```
- 3번째 줄은 `Turn X+ up, hold` → `Hold still...` → `Measuring 3/5 s`로 바뀝니다. 충격이 나거나 자세가 흐트러지면 **그 단계만** 다시 하고 `Moved! Redo X+ up`을 보여 줍니다.
- 배율이 0.9~1.1, |바이어스|가 1 m/s² 안이 아니면 `Bad result. Retry`와 `Retry`/`Quit`입니다.
- 경통에 붙인 채로는 6방향을 만들기 어려우므로 IMU를 떼어 평평한 곳에서 하고 같은 방향으로 다시 붙입니다(화면 5, 6번째 줄).

### 5Settings — 설정 (DES-CFG-05)
정보 2줄 + 항목 8개를 6줄 창으로 봅니다.
```
*5Settings
Flash: loaded           ← loaded / saved / SAVE FAILED / defaults
RAduty   +187.5 PWM     ← 항성시 속도에 해당하는 RA 듀티 (0 = 보정 전)
>RAduty = RRcmd         ← 지금 RRcmd 값을 RAduty로 정하고 저장
 Location >
 Time >
 Motor dir Normal       ← 클릭하면 Normal ↔ Reverse (배선 방향)
 Guide tol  1.00 deg    ← 클릭하면 편집 (0.1° 단위, 0.1~20°)
 Save                   ← 지금 값들을 저장
 Reset defaults         ← 클릭하면 'Reset? click again', 한 번 더 누르면 기본값으로 되돌리고 저장 (돌리면 취소)
 Back
```
- 저장하는 값과 형식은 `src/config/settings.h`(DES-CFG-02, 03). 플래시에는 ESP8266 `EEPROM` 라이브러리로 256 B를 씁니다.
- **언제 쓰는가**: `Save`, `RAduty = RRcmd`, `Reset defaults`, 보정 `Apply`, Location의 `Save`, Time의 `Set clock`(UTC 오프셋 때문), 그리고 직전 GPS 위치 자동 저장(아래). 저장된 내용과 같으면 쓰지 않습니다.
- UI는 요청 플래그만 세우고, 메인 루프가 UI 다음에 `config::serviceRequests`로 씁니다. 그래서 `Flash: saved`는 한 프레임 뒤에 나옵니다.
- Motor dir, Guide tol은 바로 쓰이고, 전원을 꺼도 남기려면 `Save`를 누릅니다.
- 자편각(DES-MAG-02)은 지자기 기능과 함께 넣습니다.

**Location** (DES-LOC-01~03)
```
*Location               *Lat step 0.1           ← 편집 중: 지금 고치는 자리
In use: GPS             In use: GPS
>Mode Auto               Mode Manual
 Lat   +0.0000 deg      *Lat  +37.5000 deg
 Lon   +0.0000 deg       Lon +126.9780 deg
 Alt      0 m            Alt     40 m
 Copy GPS fix            Copy GPS fix
 Save                   ← 스크롤
 Back
```
- `Mode`: `Auto`는 GPS fix → 직전 GPS → 수동 값 순서, `Manual`은 수동 값만 씁니다. `In use`는 지금 쓰는 출처입니다.
- 값 줄을 클릭하면 큰 자리부터 고칩니다(위도 10°, 1°, 0.1° … 0.0001°, 경도 100°부터, 고도 1000 m부터). 클릭할 때마다 다음 자리로, 마지막 자리 다음 클릭에서 끝납니다. 값을 하나라도 고치면 수동 값이 "있음"이 됩니다.
- `Copy GPS fix`: 지금 GPS fix를 수동 값으로 복사합니다(fix가 없으면 `(none)`).
- **직전 GPS 위치**(DES-LOC-02): fix(위성 4개 이상)가 30초 유지되면, 저장된 값이 없거나 1 km 넘게 옮겨졌을 때 위치와 날짜를 저장합니다. 전원을 켤 때마다 최대 한 번입니다.

**Time** (DES-LOC-04)
```
*Time (local)
In use GPS 21:00:08     ← 지금 쓰는 시각을 지역 시각으로
>Year   2026
 Month  11
 Day    21
 Hour   21
 Minute  0
 Second  0              ← 스크롤
 UTC +09:00             ← 15분 단위, -12:00 ~ +14:00
 Set clock  (set)       ← 클릭하면 위 값으로 수동 시각을 맞춤
 Back
```
- 값을 클릭해서 고치고(월·일·시·분·초는 한 바퀴 돌아감), `Set clock`을 누르면 지역 시각 − UTC 오프셋을 수동 시각으로 씁니다. 날짜가 그 달에 없으면(2월 31일) 그 달 마지막 날로 맞춥니다.
- 수동 시각은 `millis()`로 흘러가고 저장하지 않습니다. GPS 시각이 있으면 GPS를 씁니다. UTC 오프셋은 저장합니다.
- 들어올 때 시각이 있으면 지금 시각으로 칸을 채웁니다.

### 6RA Speed — 적경 축 모터 수동 구동
```
*6RA Speed
>RRcmd    0.00000DEG    ← 클릭하면 편집 모드 (*Set RA Speed, 다이얼 한 칸 = 1)
 RRate    0.00000DPS    ← 보정된 자이로로 잰 실제 회전 속도
 Quit
 ROL    0.00000  DEG
 LAT N 37.566500DEG     ← 지금 쓰는 위치의 위도
 PIT    0.00000  DEG
```
RRcmd를 편집하면 추적이 꺼집니다. 별을 따라가는 RRcmd를 찾아 5Settings의 `RAduty = RRcmd`로 저장합니다(DES-MOT-01).

### Sleep
상태바만 남기고 내용 영역을 비웁니다. 버튼을 누르면 메인 메뉴로 돌아갑니다.

## 7. 설계와 현재 코드의 차이

| 항목 | 설계 | 현재 코드 |
|---|---|---|
| 방위(dAZ), 지향 방법 A | DES-PA-02, DES-PNT-01, 03: 지자기 방위 | 지자기(DES-MAG) 전이라 `--- no mag`. 지향은 늘 방법 B(중력), 정렬 확인 전에는 `?` |
| 정렬 OK | DES-PA-04: 고도와 방위 | 고도만 |
| Calibration | Gyro, Accel, Mag | Mag 없음(DES-MAG-01) |
| Mag/Temp | DES-MAG-03: 방위 HEAD | 온도 표시 |
| Att.Estimator | DES-ATT-03: Yaw | Roll/Pitch만 |
| Settings | DES-CFG-05: 자편각 | 자편각 없음(DES-MAG-02) |
| Sleep | 화면 끄기 | 빈 화면. 디스플레이 전원은 끄지 않음 |
| 숫자 서식 | `%9.4f` | 대부분 `%10.5f` |

## 8. 이후 구조 구상 (`Arduino/archive/ws/menu_display.h`, 미완성)

`switch` 문 하나에 몰려 있던 메뉴 로직을 `MenuDisplay` 클래스로 옮기려던 초안입니다. 지금의 `Screen` 클래스 구조(10절)가 이 구상을 이어받았습니다.

## 9. UI의 변천

1. `archive/ws/gps_imu_oled_button.cpp`: 버튼 하나로 GPS → IMU → EST → PERFORM 페이지를 차례로 넘김
2. `archive/ws/menu.ino`, `app/app.ino`: 로터리 + 버튼 기반 메뉴, 상태바/제목줄/내용 영역 레이아웃 정립
3. `src/ui/`: `Screen` 클래스와 화면 스택. PC 테스트와 시뮬레이터가 같은 코드를 씀
4. v0.2.0: 쓰는 순서대로 메뉴 재구성(DES-UI-01), 보정·위치·극축 정렬·안내 화면 추가

## 10. 코드 구조

| 파일 | 역할 |
|---|---|
| `src/ui/frame.h` | 한 화면 분량(8줄×22바이트) 버퍼. `set(row, fmt, ...)`은 줄 너비를 넘으면 잘라내고 `overflow`를 켬(테스트가 확인) |
| `src/ui/ui_context.h` | `Telemetry`(UI가 보여줄 값, 메인 루프가 매 프레임 채움), `UiState`(UI가 바꾸는 값: RA 속도 명령, 보정값, 추적, 위치·시각 입력, 모터 정지 요청) |
| `src/ui/screen.h` | `Screen` 인터페이스(`onEnter`/`onRotate`/`onClick`/`tick`/`render`), `MenuCursor`(메뉴 선택·스크롤), `PageCursor`(정보 줄 + 항목 화면의 스크롤) |
| `src/ui/screen_manager.*` | 화면 스택(push = 들어가기, pop = 나가기), 커서 깜박임, 매 프레임 `motor_inhibit` 초기화 |
| `src/ui/screens.*` | 각 화면 클래스 |
| `src/ui/telescope_ui.*` | 화면들을 묶은 `TelescopeUi`와 상태바. 메인 루프는 프레임마다 `ui.step(회전량, 클릭, ctx, frame)`만 호출 |
| `src/app/loop_logic.h` | 메인 루프의 순수 로직(`correctImu`, `updateLocationTime`, `updateAstro`, `motorCommand`). `app.ino`와 시뮬레이터가 같이 씀 |
| `src/estimation/calibration.*` | 충격 감지, 자이로 30초 보정, 가속도 6방향 보정 |
| `src/location/location.*` | 위치 출처 고르기, 거리, 직전 GPS 기록 |
| `src/astro/target_telemetry.h` | 천체 계산 결과를 `Telemetry`로, 추적 속도·방향, 경통 지향, 북극성 |
| `src/config/settings.*` | 저장할 설정 구조체, 직렬화(little endian), CRC-32, 범위 검사, `SettingsStore` 인터페이스와 PC용 `RamSettingsStore` |
| `src/config/settings_binding.h` | 설정 ↔ `UiState` 옮기기, UI의 저장·초기화 요청 처리(`serviceRequests`) |
| `src/config/eeprom_store.h` | 보드 전용 `EepromSettingsStore`(ESP8266 `EEPROM`) |

메인 루프 한 바퀴: 센서 읽기 → `correctImu`(보정값 적용) → 자세 추정 → `updateLocationTime`(위치·시각 출처) → `updateAstro`(1초마다 천체, 매 프레임 경통 지향) → `ui.step` → 설정 저장 요청 처리 → `motorCommand`(추적, 모터 정지).

UI 코드는 하드웨어에 의존하지 않아서 PC에서도 빌드됩니다. 단위 테스트(`Arduino/test/test_ui.cpp`)와 UI 시뮬레이터가 같은 코드를 씁니다.

화면을 추가하려면 `Screen`을 상속한 클래스를 만들고, `TelescopeUi`에 멤버로 넣은 뒤 메뉴 항목에 연결하면 됩니다.
