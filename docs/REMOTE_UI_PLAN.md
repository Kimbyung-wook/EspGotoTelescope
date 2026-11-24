# 스마트폰 원격 UI 개발 계획

가대에 붙은 로터리 버튼을 돌리면 망원경이 흔들립니다. 그래서 안드로이드 스마트폰에서 SSD1306 화면을 보고 로터리를 조작하려고 합니다. 이 문서는 그 개발 계획입니다.

## 1. 결론: Bluetooth 대신 Wi-Fi

ESP8266(NodeMCU v3)에는 Bluetooth가 없고 Wi-Fi만 있습니다. Wi-Fi로 충분히 할 수 있고, 지금 보드에서는 가장 쉬운 길입니다.

| 방법 | 추가 부품 | 폰 쪽 | 판단 |
|---|---|---|---|
| **ESP8266 Wi-Fi AP + 웹 페이지(WebSocket)** | 없음 | 브라우저(Chrome)만 있으면 됨 | **채택** |
| HC-05 (Bluetooth Classic) | 모듈 + SoftwareSerial | 전용 앱 필요 (브라우저는 BT Classic 불가) | 핀 부족 |
| HM-10 (BLE) + Web Bluetooth | 모듈 + SoftwareSerial | Chrome Web Bluetooth | 핀 부족, BLE 대역폭 작음 |
| ESP32로 교체 | 보드 교체 | Wi-Fi/BLE 모두 가능 | 장기 대안 (IRAM, UART 여유) |

핀 상황(`board_config.h`): GPS가 UART0(RX/TX)를 쓰고, 남은 핀은 D5 하나뿐입니다. 외부 BT 모듈은 TX/RX 두 핀이 필요해서 배선을 바꿔야 합니다. Wi-Fi는 핀이 필요 없습니다.

## 2. 구조

```
[Android Chrome]  ←Wi-Fi (ESP8266 SoftAP "Telescope", WPA2)→  [ESP8266]
  웹 페이지                                                   HTTP: 페이지 1장 (PROGMEM, gzip)
  - 8줄×21자 화면 (빨간 글씨, 야간용)                          WebSocket: 화면 줄 ↔ 입력
  - 가상 다이얼(드래그) + 클릭 버튼 + ±1/±10 버튼
```

- **AP 모드**: 관측지에는 공유기가 없으므로 ESP8266이 직접 AP를 엽니다. 폰이 접속해 `http://192.168.4.1`을 엽니다. 개발 중에는 집 공유기에 붙는 STA 모드와 `telescope.local`(mDNS)도 씁니다.
- **프로토콜**: UI 시뮬레이터의 JSON 프로토콜(`tools/ui_simulator/PROTOCOL.md`)을 그대로 씁니다.
  - ESP → 폰: `{"lines":[8줄]}`. 화면이 바뀔 때만, 최대 5~10 Hz로 보냅니다.
  - 폰 → ESP: `{"rotate":n}`, `{"click":1}`
- **입력 합치기**: `loop()`에서 `delta = 로터리 delta + 원격 delta`, `click = 로터리 click || 원격 click`으로 합칩니다. UI 코드(`TelescopeUi::step`)는 바뀌지 않습니다. 실물 OLED와 로터리도 그대로 동작합니다(폰이 없어도, 끊겨도 됨).
- **화면**: 지금의 `Frame`이 이미 8줄 텍스트라 그대로 보냅니다. 비트맵 전송은 필요 없습니다.

## 3. 위험 요소와 확인 방법

| 위험 | 내용 | 확인/대응 |
|---|---|---|
| IRAM 96% | Wi-Fi/lwIP와 웹소켓 라이브러리가 IRAM을 더 쓸 수 있음 | 0단계에서 먼저 빌드. 넘치면 lwIP "Lower Memory" 변형, ISR 외 `IRAM_ATTR` 정리 |
| 루프 주기 흔들림 | Wi-Fi 처리 때문에 루프가 늦어질 수 있음 | Health 화면 `perform_*` 값으로 측정. 대기 루프는 이미 `yield()`를 부름 |
| 전류 | Wi-Fi 송신 피크 300 mA 이상 | 12 V 전원 + 레귤레이터 여유 확인, `WiFi.setOutputPower`로 낮춤 |
| 지자기/GPS 간섭 | 송신 전류 펄스가 가대의 지자기 센서에 잡힐 수 있음 | Wi-Fi 켜고/끄고 Mag 값 비교. 필요하면 송신 출력 낮춤 |
| 기본 Wi-Fi 상태 | ESP8266은 마지막에 저장된 Wi-Fi 모드로 켜질 수 있음 (지금 코드는 Wi-Fi를 끄지 않음) | 원격 UI를 끈 빌드에서는 `WiFi.mode(WIFI_OFF)` 명시 |
| 보안 | 아무나 접속해 모터를 돌릴 수 있음 | WPA2 비밀번호, 동시 접속 1명 |

## 4. 단계별 계획

각 단계마다 빌드, 테스트, 커밋을 합니다.

### 0단계. 가능성 확인 (반나절)
- `WiFi.softAP` + WebSocket 서버만 넣고 빌드해 IRAM/RAM 사용량을 확인합니다.
- 실물에서 루프 주기(Health), 전류, Mag 값 변화를 잽니다.
- 판단: IRAM이 넘치거나 루프가 크게 흔들리면 ESP32 교체를 검토합니다.

### 1단계. 화면 미러링 (읽기 전용)
- `src/remote/remote_ui.{h,cpp}`: AP 시작, HTTP로 페이지 제공, WebSocket으로 화면 줄 전송.
- `board_config.h`에 `REMOTE_UI_ENABLE`, AP 이름/비밀번호.
- 폰 페이지: 검은 바탕에 빨간 고정폭 글씨, 화면 꺼짐 방지(Wake Lock).
- 시험: 폰에 화면이 OLED와 같게 나오는지 확인합니다.

### 2단계. 입력 (다이얼, 클릭)
- 폰 → ESP 입력을 `loop()`에서 합칩니다.
- 가상 다이얼: 원을 드래그하면 15°마다 `rotate ±1`, 짧게 누르면 클릭. 세밀 조정용 `−10 −1 +1 +10` 버튼.
- 연결이 끊기면 원격 입력을 버립니다(남은 회전량이 계속 들어가지 않게).
- 시험: Offset 편집, Track 켜기/끄기를 폰으로만 해봅니다.

### 3단계. 다듬기
- 접속 상태 표시(폰: 연결/끊김, OLED 상태바에 Wi-Fi 표시).
- 자동 재접속, 페이지 캐시.
- 필요하면 원격 전용 위젯: Offset 슬라이더, Track 버튼을 화면 아래에 따로 둠(메뉴를 거치지 않음).

### 4단계 (선택). 안드로이드 앱
- 브라우저로 부족할 때만 합니다. 볼륨 키를 다이얼로 쓰기, 화면 켜짐 유지, 진동 피드백.
- 같은 WebSocket 프로토콜을 씁니다(Kotlin, OkHttp WebSocket).

## 5. 하드웨어 없이 개발하기

- UI 시뮬레이터(`tools/ui_simulator`)에 WebSocket 브리지(Python)를 붙이면 같은 웹 페이지를 PC에서 띄워 개발할 수 있습니다. 러너가 이미 같은 JSON 줄을 주고받습니다.
- 웹 페이지는 펌웨어와 시뮬레이터가 같은 파일을 씁니다(빌드 때 PROGMEM 헤더로 변환).
- `protocol_check.py`에 원격 입력 경로 테스트를 추가합니다.

## 6. 정할 것

- AP 이름과 비밀번호, 송신 출력.
- 원격 UI를 항상 켤지, 메뉴에서 켜고 끌지(전류, 간섭 때문에).
- 장기적으로 ESP32로 옮길지 (0단계 결과를 보고 결정).
