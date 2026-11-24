# SimpleGoTo 버전

펌웨어 이름과 버전은 `Arduino/app/src/version.h` 한 곳에 있습니다.

```cpp
#define SIMPLEGOTO_NAME    "SimpleGoTo"
#define SIMPLEGOTO_VERSION "0.1.0"
```

- 부팅 화면 첫 줄(`SimpleGoTo v0.1.0`)과 폰 원격 UI 페이지 맨 위에 나옵니다(접속 인사 `hello`의 `name`, `version`).
- PC 데모(`tools/ui_simulator/gui/web_bridge.py`)도 같은 파일을 읽어 보냅니다.

## 번호 규칙

`MAJOR.MINOR.PATCH`

- **MINOR**: 기능이 늘 때(새 화면, 새 원격 UI 단계 등).
- **PATCH**: 고치기만 할 때.
- **MAJOR**: 마운트가 실제로 쓸 만해질 때까지는 0으로 둡니다. 1.0.0은 실물에서 추적과 GoTo가 검증된 뒤에 올립니다.
- 버전을 올리는 커밋은 메시지에 새 버전을 적습니다.

## 기록

### 0.1.0 (2026-11-24)
처음 붙인 번호입니다. 이때까지 들어간 기능:
- OLED 메뉴 UI(로터리 엔코더), 상태 화면, IMU 보정, 자세 추정기, Health.
- 달과 행성 위치 계산, Planet Tracking(추적, Offset), 망원경 지향(T.R.A./T.DEC.).
- Wi-Fi 폰 원격 UI 0~3단계(빌드 옵션 `REMOTE_UI_ENABLE`, 기본 꺼짐): 화면 미러링(도트 화면), 다이얼 입력, 상태바 Wi-Fi 표시, 재접속, 페이지 캐시.
