# SimpleGoTo 할 일과 설계 공백

2026-11-24, v0.3.1 기준입니다. 각 항목의 근거 문서를 괄호 안에 적었습니다. 기능 범위는 [REQUIREMENTS.md](REQUIREMENTS.md)가 기준입니다.
표시: **[실물]** 하드웨어로만 확인 가능, **[설계]** 방식을 먼저 정해야 함, **[구현]** 방식은 정해져 있고 코드만 남음.

## 1. 지금 가장 먼저 (실물 확인, 다른 일의 전제)

| # | 할 일 | 왜 먼저인가 |
|---|---|---|
| 1 | **[실물] IMU 축 방향 확인**: 경통을 천구 북극에 맞추고 1Target 대상 화면의 `T.DEC.`가 +90 근처인지, 2Polar Align의 `ALT`가 경통 고도와 맞는지. 아니면 `IMU_TUBE_OPTICAL_AXIS`/`IMU_TUBE_DEC_AXIS` 수정 (`board_config.h`, UI.md) | T.R.A./T.DEC.(지향 표시)가 맞아야 그 위의 모든 기능이 의미가 있음 |
| 2 | **[실물] RA 모터 보정**: 별 하나를 따라가는 RRcmd를 6RA Speed에서 찾아 5Settings의 `RAduty = RRcmd`로 저장. 처음 값은 `RA_DUTY_AT_SIDEREAL`(0)이라 Offset만으로 모터가 돎 (MOON_POSITION.md 남은 일) | 자동 추적의 기본값. DC 모터는 PWM과 속도가 선형이 아니므로 여러 점을 재서 표로 바꿀 수도 있음 |
| 3 | **[실물] 배선 가정 확인**: `(assumed)` 부품(배터리, 강압 컨버터, L298N형 드라이버, KY-040), `⚠` 핀 순서(ICM20948, GPS), 부팅 핀 D3/D4/D8 (ARCHITECTURE.md) | 배선도를 실물과 맞춰 두어야 이후 회로 추가(전압 측정 등)가 안전 |
| 4 | **[실물] 원격 UI 측정**: `REMOTE_UI_ENABLE=1`로 Health 루프 주기, 전류, Wi-Fi 켜고/끄고 Mag 값 변화. 폰으로 Offset 편집, Track 켜기/끄기, 상태바 `w`/`W`, 잠금 후 재접속, 304 캐시 (REMOTE_UI_PLAN.md) | ESP32로 옮길지 판단하는 근거 |
| 5 | **[실물] 달 계산 시간 측정**: `moonGeocentric`, `targetState(Moon)`, 1 Hz 부하 µs 표 채우기 (MOON_POSITION.md) | 루프 여유 확인 |
| 6 | **[실물] 보정 문턱값**: 충격 감지(`CAL_SHOCK_GYRO_DPS` 2 °/s, `CAL_SHOCK_ACCEL_MPS2` 0.5 m/s²)가 손 떨림·모터 진동에 너무 민감하거나 둔하지 않은지. 가속도 6방향 자세 판정(0.8 g / 0.35 g) (DES-CAL-02, 04) | 보정이 실물에서 끝까지 되는지 |
| 7 | **[실물] 극축 정렬과 북극성 시계**: 고도 허용 0.5°(`PA_ALT_TOL_DEG`)가 맞출 만한지, `Polaris at h:mm`이 극축 망원경 눈금과 같은 방향인지(맨눈 기준, 상이 뒤집히면 6시간 차) (DES-PA-03, 04) | 정렬 확인 뒤 지향(`?` 없음)을 믿을 수 있는지 |
| 8 | **[실물] 전압 측정**: A0에 직렬 저항(가정 1.2 MΩ)을 달고 `VBAT_SENSE_ENABLE 1`, 멀티미터와 비교해 `VBAT_FULL_SCALE_V` 맞추기. Wi-Fi 켠 채 0.5초 읽기가 연결을 흔들지 않는지 (board_config.h) | 상태바 전압이 맞는지 |

## 2. 정해진 방향 (2026-11-24)

요구사항과 설계는 [REQUIREMENTS.md](REQUIREMENTS.md)(`REQ-*`, `DES-*`)에 있습니다. 앞서 열려 있던 질문은 이렇게 정했습니다.

| 질문 | 결정 | 식별자 |
|---|---|---|
| GoTo를 어떻게 | 자동 slew 없음. 지향을 보여 주고 대상까지 `dRA`/`dDEC`로 안내, 사람이 손으로 맞춤. 모터는 RA 하나 | REQ-SYS-01, REQ-PNT-02, DES-PNT-04 |
| 추적 속도 | 항성시와 달 두 가지 | REQ-MOT-01, REQ-MOT-02, DES-TRK-01 |
| 닫힌 루프 추적 | 나중에, 개루프 결과를 본 뒤 | DES-MOT-03 |
| 극축 정렬 보조 | 만듦. 고도는 중력, 방위는 지자기 + 자편각, 북극성 시간각 표시 | REQ-PA-01, DES-PA-01~04 |
| 방위(Yaw), 지자기 보정 | 만듦. hard/soft iron 보정, 기울기 보정 나침반, 자이로와 합침 | REQ-MAG-01, DES-MAG-01~04, DES-ATT-02 |
| 설정 저장 | 만듦. EEPROM 구조체 + CRC (**구현됨**, 아래 3절) | REQ-CFG-01, DES-CFG-01~06 |
| GPS 없이 | 수동 위치(저장), 직전 GPS 위치(저장), 수동 시각(저장 안 함) | REQ-LOC-02, REQ-LOC-03, DES-LOC-01~04 |
| 자이로·가속도 보정 | 자이로 30초(모터 정지, 결과 확인 후 적용), 가속도 6방향, 충격 시 실패와 재시도 | REQ-CAL-01~03, DES-CAL-01~07 |

구현 순서는 REQUIREMENTS.md 5절을 따릅니다: 설정 저장 → 보정 → 위치·시각 → 지자기·방위 → 극축 정렬 → 지향 안내·추적 → 메뉴 재구성. v0.2.0에서 지자기·방위를 뺀 나머지를 구현했습니다(REQUIREMENTS.md 6절).

## 3. 구현만 남은 것

| 항목 | 지금 | 근거 |
|---|---|---|
| **지자기와 방위** | 원시값만 읽음. 지자기 보정 화면(4Calibration → Mag), 자편각 설정, 기울기 보정 방위, Yaw, 극축 정렬의 `dAZ`와 방위 OK, 지향 방법 A와 A/B 비교가 모두 이것에 걸려 있음 | DES-MAG-01~04, DES-ATT-02~03, DES-PA-02, 04, DES-PNT-01, 03 |
| 원격 전용 위젯 | Offset 슬라이더, Track 버튼. 다이얼로 충분한지 실물에서 본 뒤 | REMOTE_UI_PLAN.md 3단계 |

## 4. 미뤄 둔 것 (필요할 때)

- **IRAM 확보**: 30,343 / 32,768 B (원격 UI 빌드 30,507 B). 넘치면 `-DNO_SW_SERIAL`(636 B) → `mmu=4816`(약 18.8 KB) 순서 (IRAM.md).
- **ESP32로 이전**: 원격 UI 실물 측정(1.4)에서 루프가 밀리거나 Mag 간섭이 크거나 리셋이 나면.
- **안드로이드 앱**: 원격 UI 4단계(선택). 브라우저로 부족할 때만 (REMOTE_UI_PLAN.md).
