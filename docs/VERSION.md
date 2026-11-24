# SimpleGoTo 버전

펌웨어 이름과 버전은 `Arduino/app/src/version.h` 한 곳에 있습니다.

```cpp
#define SIMPLEGOTO_NAME    "SimpleGoTo"
#define SIMPLEGOTO_VERSION "0.3.1"
```

- 부팅 화면 첫 줄(`SimpleGoTo v0.3.1`)과 폰 원격 UI 페이지 맨 위에 나옵니다(접속 인사 `hello`의 `name`, `version`).
- PC 데모(`tools/ui_simulator/gui/web_bridge.py`)도 같은 파일을 읽어 보냅니다.

## 번호 규칙

`MAJOR.MINOR.PATCH`

- **MINOR**: 기능이 늘 때(새 화면, 새 원격 UI 단계 등).
- **PATCH**: 고치기만 할 때.
- **MAJOR**: 마운트가 실제로 쓸 만해질 때까지는 0으로 둡니다. 1.0.0은 실물에서 추적과 GoTo가 검증된 뒤에 올립니다.
- 버전을 올리는 커밋은 메시지에 새 버전을 적습니다.

## 기록

### 0.3.1 (2026-11-24)
- 시각 순서를 GPS → 폰 시각 → 수동 시각으로 바꿈(0.3.0은 수동 시각이 폰 시각보다 앞).

### 0.3.0 (2026-11-24)
- 전원 전압: A0 분압 측정(`VBAT_SENSE_ENABLE`, 기본 0), 거르기, 측정 전에는 `V --.--`.
- Sleep: OLED 전원을 끄고 버튼으로 다시 켬.
- 폰 시각(DES-LOC-05): 원격 UI 페이지가 접속할 때 보내는 시각을 GPS도 수동 시각도 없을 때 씀. 상태바 `P`, 출처 `PHN`.
- PC 데모: `web_bridge.py`가 페이지 접속 여부를 상태바 `w`/`W`로, 페이지 시각을 러너 `phone_time`으로 넘김.

### 0.2.0 (2026-11-24)
REQUIREMENTS.md 5절의 보정, 위치·시각, 극축 정렬, 지향 안내·추적, 메뉴 재구성(지자기 제외):
- 메뉴를 쓰는 순서대로 다시 묶음: 1Target / 2Polar Align / 3Status / 4Calibration / 5Settings / 6RA Speed / Sleep. Axis for Moon은 Target에 합침.
- 보정: 자이로 30초(결과 확인 후 적용), 가속도 6방향(바이어스·배율), 충격 감지와 재시도, 보정 중 모터 정지.
- 위치·시각: 출처 선택(GPS → 직전 GPS → 수동), 직전 GPS 자동 저장, 수동 위치·시각 입력, UTC 오프셋, 상태바 `T`/`M`.
- 극축 정렬: 경통 고도와 위도 차, 북극성 시계 위치, 정렬 확인.
- 대상 화면: dRA/dDEC 안내와 ON TARGET, 추적 방식(Sidereal/Lunar), 남반구·배선 방향, 안내 허용 범위 설정.

### 0.1.0 (2026-11-24)
처음 붙인 번호입니다. 이때까지 들어간 기능:
- OLED 메뉴 UI(로터리 엔코더), 상태 화면, IMU 보정, 자세 추정기, Health.
- 달과 행성 위치 계산, Planet Tracking(추적, Offset), 망원경 지향(T.R.A./T.DEC.).
- Wi-Fi 폰 원격 UI 0~3단계(빌드 옵션 `REMOTE_UI_ENABLE`, 기본 꺼짐): 화면 미러링(도트 화면), 다이얼 입력, 상태바 Wi-Fi 표시, 재접속, 페이지 캐시.
