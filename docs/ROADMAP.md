# SimpleGoTo 할 일과 설계 공백

2026-11-24, v0.1.0 기준입니다. 각 항목의 근거 문서를 괄호 안에 적었습니다. 기능 범위는 [REQUIREMENTS.md](REQUIREMENTS.md)가 기준입니다.
표시: **[실물]** 하드웨어로만 확인 가능, **[설계]** 방식을 먼저 정해야 함, **[구현]** 방식은 정해져 있고 코드만 남음.

## 1. 지금 가장 먼저 (실물 확인, 다른 일의 전제)

| # | 할 일 | 왜 먼저인가 |
|---|---|---|
| 1 | **[실물] IMU 축 방향 확인**: 경통을 천구 북극에 맞추고 Planet Tracking의 `T.DEC.`가 +90 근처인지. 아니면 `IMU_TUBE_OPTICAL_AXIS`/`IMU_TUBE_DEC_AXIS` 수정 (`board_config.h`, UI.md) | T.R.A./T.DEC.(지향 표시)가 맞아야 그 위의 모든 기능이 의미가 있음 |
| 2 | **[실물] RA 모터 보정**: 별 하나를 따라가는 RRcmd를 찾아 11 Settings의 `RAduty = RRcmd`로 저장. 처음 값은 `RA_DUTY_AT_SIDEREAL`(0)이라 Offset만으로 모터가 돎 (MOON_POSITION.md 남은 일) | 자동 추적의 기본값. DC 모터는 PWM과 속도가 선형이 아니므로 여러 점을 재서 표로 바꿀 수도 있음 |
| 3 | **[실물] 배선 가정 확인**: `(assumed)` 부품(배터리, 강압 컨버터, L298N형 드라이버, KY-040), `⚠` 핀 순서(ICM20948, GPS), 부팅 핀 D3/D4/D8 (ARCHITECTURE.md) | 배선도를 실물과 맞춰 두어야 이후 회로 추가(전압 측정 등)가 안전 |
| 4 | **[실물] 원격 UI 측정**: `REMOTE_UI_ENABLE=1`로 Health 루프 주기, 전류, Wi-Fi 켜고/끄고 Mag 값 변화. 폰으로 Offset 편집, Track 켜기/끄기, 상태바 `w`/`W`, 잠금 후 재접속, 304 캐시 (REMOTE_UI_PLAN.md) | ESP32로 옮길지 판단하는 근거 |
| 5 | **[실물] 달 계산 시간 측정**: `moonGeocentric`, `moonState`, 1 Hz 부하 µs 표 채우기 (MOON_POSITION.md) | 루프 여유 확인 |

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

구현 순서는 REQUIREMENTS.md 5절을 따릅니다: 설정 저장 → 보정 → 위치·시각 → 지자기·방위 → 극축 정렬 → 지향 안내·추적 → 메뉴 재구성.

## 3. 구현만 남은 것

| 항목 | 지금 | 근거 |
|---|---|---|
| 설정 화면 나머지 칸 | 저장 형식에는 위치 모드·수동 위치·직전 GPS·UTC 오프셋·자편각·모터 방향·안내 범위가 있지만 화면과 사용처가 없음. 위치(DES-LOC), 지자기(DES-MAG) 구현 때 연결. 직전 GPS 자동 저장(DES-LOC-02)도 그때 | DES-CFG-05, DES-LOC-02 |
| 전원 전압 측정 | 상태바 `V 12.60` 고정값. 분압 저항을 A0에 달고 읽기 | ARCHITECTURE.md, UI.md |
| 자이로·가속도 보정 다시 만들기 | 지금은 자이로 16초 보정만 있고 바로 적용됨. Acc. 항목은 누르면 아무 일 없음 | DES-CAL-03~07 |
| Sleep | 화면만 비움. OLED 전원은 그대로 | UI.md 7절 |
| 원격 전용 위젯 | Offset 슬라이더, Track 버튼. 다이얼로 충분한지 실물에서 본 뒤 | REMOTE_UI_PLAN.md 3단계 |
| PC 데모의 Wi-Fi 표시 | 시뮬레이터 상태바에는 `w`/`W`가 안 나옴(러너가 원격 상태를 모름) | web_bridge.py |

## 4. 미뤄 둔 것 (필요할 때)

- **IRAM 확보**: 30,343 / 32,768 B (원격 UI 빌드 30,507 B). 넘치면 `-DNO_SW_SERIAL`(636 B) → `mmu=4816`(약 18.8 KB) 순서 (IRAM.md).
- **ESP32로 이전**: 원격 UI 실물 측정(1.4)에서 루프가 밀리거나 Mag 간섭이 크거나 리셋이 나면.
- **안드로이드 앱**: 원격 UI 4단계(선택). 브라우저로 부족할 때만 (REMOTE_UI_PLAN.md).

## 5. 문서 정리

- UI.md 7~9절이 `Arduino/ws/...`를 가리키지만 실제 위치는 `Arduino/archive/ws/`입니다.
- UI.md 7절 "설계와 현재 코드의 차이" 표와 메뉴 구조는 REQUIREMENTS.md(DES-UI-01)와 다릅니다. 기능이 들어가면 같이 고칩니다.
- `TelescopeUi` 등 코드 이름은 SimpleGoTo로 바꾸지 않았습니다(필요하면 따로).
