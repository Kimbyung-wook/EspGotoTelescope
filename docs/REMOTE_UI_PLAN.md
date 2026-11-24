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

#### 0단계 결과 (2026-11-24, 빌드만, 실물 측정 전)

넣은 것:
- `board_config.h`: `REMOTE_UI_ENABLE`(기본 0, `#ifndef`라 빌드 옵션으로 덮어쓸 수 있음), `REMOTE_UI_AP_SSID`/`_PASS`/`_CHANNEL`, `REMOTE_UI_TX_DBM`(기본 10 dBm), HTTP 포트 80, WebSocket 포트 81.
- `src/remote/remote_ui.{h,cpp}`: `remote::begin/poll/publish/takeInput/clients`.
  - 켬(1): `WiFi.softAP`(WPA2, 동시 접속 1대), `ESP8266WebServer`로 PROGMEM 페이지 1장(화면 8줄 + `-10 -1 OK +1 +10` 버튼), `WebSocketsServer`(포트 81).
  - ESP → 폰: 화면 줄이 바뀐 프레임에만 `{"lines":[8줄]}`. 글자 이스케이프는 UI 시뮬레이터와 같음(0x20 미만, 0x7F 이상 → `\u00XX`).
  - 폰 → ESP: `{"rotate":n}`(−100~100), `{"click":1}`를 세기만 하고, 받은 누계를 `{"rx":{"rotate":합,"click":합}}`로 되돌려 보냅니다(페이지 아래에 표시 → 왕복 확인용). **UI 입력에는 아직 합치지 않습니다(2단계).**
  - 끔(0): `setup()` 맨 앞에서 `WiFi.persistent(false); WiFi.mode(WIFI_OFF)`. 네트워크 코드는 링크되지 않습니다.
- `app.ino`: `monitor->print()` 다음에 `remote::publish(frame)`, 대기 루프에서 `yield()` 전에 `remote::poll()`. `TelescopeUi::step`은 그대로입니다.
- 참고: core 3.x는 이미 부팅 때 Wi-Fi를 끈 채(모뎀 슬립) 시작합니다(`__disableWiFiAtBootTime`). 그래서 위험표의 "기본 Wi-Fi 상태"는 core 3.1.2에서는 이미 해결되어 있고, 명시 호출은 보험입니다.

라이브러리: **WebSockets 2.7.2** (Markus Sattler, `links2004/arduinoWebSockets`). ESP8266 core에는 WebSocket 서버가 없어서 설치했습니다. Arduino 라이브러리 관리자에 있고 ESP8266에서 가장 널리 쓰이며, core의 `WiFiServer` 위에서 돌아 추가 의존성이 없습니다. `REMOTE_UI_ENABLE 1`로 빌드할 때만 필요합니다.
```
arduino-cli lib install WebSockets      # Arduino IDE: 라이브러리 관리자 > "WebSockets" by Markus Sattler
Arduino/scripts/build.sh --build-property "compiler.cpp.extra_flags=-DREMOTE_UI_ENABLE=1"
```

빌드 결과 (core 3.1.2, `nodemcuv2:xtal=160`, 기본 lwIP "v2 Lower Memory", MMU 32KB cache + 32KB IRAM):

| 빌드 | IRAM (전체/65536) | IRAM 코드 (/32768) | RAM 전역 (/80192) | flash (/1048576) |
|---|---|---|---|---|
| 기준 364ced6 | 63087 (96%) | 30319 | 35456 | 311960 |
| `REMOTE_UI_ENABLE 0` | 63087 (96%) | 30319 (+0) | 35484 (+28) | 313268 (+1308) |
| `REMOTE_UI_ENABLE 1` | 63251 (96%) | 30483 (+164) | 39200 (+3744) | 356656 (+44696) |
| 1 + lwIP `lm2n` (no features) | 63251 | 30483 | 39124 | 346432 |
| 1 + lwIP `hb2f` (Higher Bandwidth) | 63251 | 30483 | 39200 | 356656 |
| 1 + MMU `4816` (16KB cache + 48KB IRAM) | 46883 (/65536) | 30499 (/49152) | 39200 | 356656 |
| 1 + `-DNO_SW_SERIAL` (Adafruit_GPS의 SoftwareSerial 제거) | 62615 | 29847 (−636) | 39200 | 355456 |

- "IRAM 96%"의 65536에는 flash 명령 캐시 32768이 포함됩니다. 실제 IRAM 코드 여유는 32768 − 30483 = **약 2.2 KB**입니다.
- **Wi-Fi SDK(lwIP, 802.11)의 IRAM 코드는 Wi-Fi를 쓰지 않아도 이미 링크되어 있습니다.** 그래서 원격 UI를 켜도 IRAM은 164 B만 늘었습니다. lwIP 변형은 IRAM에 영향이 없고, 기본값이 이미 "Lower Memory"입니다.
- 남은 heap은 약 80192 − 39200 ≈ 41 KB(정적 기준). AP와 TCP 연결 몇 개에 필요한 동적 메모리(대략 15~20 KB)를 넣어도 여유가 있습니다. 실물에서 `ESP.getFreeHeap()`로 확인하면 좋습니다(3단계 상태 표시 후보).
- IRAM이 더 필요해지면 MMU `4816`이 16 KB를 더 줍니다(대신 flash 캐시가 절반이라 flash 코드가 조금 느려질 수 있음 → Health로 확인).
- 빈 스케치도 IRAM 코드 약 26.4 KB(32 KB의 80%)를 씁니다(SDK, Wi-Fi MAC 포함). 다른 IRAM 절감책: `-DNO_SW_SERIAL`(`compiler.cpp/c.extra_flags`, GPS가 하드웨어 UART0을 쓰므로 SoftwareSerial 불필요)로 약 0.6 KB. 자세한 분석과 결정은 `docs/IRAM.md`를 봅니다.

호스트 테스트 (모두 통과): `Arduino/test/run_tests.sh` 70 tests 584 checks 0 failures, `tools/ui_simulator/run_tests.sh` 267 checks 0 failures, `tools/ui_simulator/build.sh` OK, `python gui/protocol_check.py` OK. (원격 UI 코드는 호스트 빌드에 들어가지 않습니다.)

#### 0단계 실물 측정 체크리스트 (사용자)

준비: `REMOTE_UI_ENABLE 0` 펌웨어와 `1` 펌웨어를 각각 올려 같은 순서로 잽니다. 측정 중 모터는 정지(RRcmd 0) 상태와 Track 켠 상태를 한 번씩.

1. **루프 주기 (8 Health 화면)** — 각 경우 30초 정도 보며 최솟값/최댓값을 적습니다.
   - `Loop`: 목표 100000 µs / 10.0 Hz. 100000을 넘는 값이 자주 나오면 주기가 밀린 것입니다(대기 루프에 `poll()`이 들어가 있어서 Wi-Fi 처리는 여기에 숨음).
   - `MAIN`: 원격 UI 1에서는 `publish()`(JSON 만들기 + 전송) 시간이 더해집니다. 폰 접속 전/후를 비교합니다.
   - `DISP`, `GPS`, `IMU`, `EST`, `AST`: 0과 1에서 차이가 거의 없어야 합니다.
   - 기록: Wi-Fi 끔 / AP만 켬(폰 미접속) / 폰 접속·화면 표시 중 / 폰에서 버튼 연타 중.
2. **전류** (12 V 입력 쪽, 가능하면 3.3 V 레귤레이터 쪽도)
   - 평균: Wi-Fi 끔 / AP 켬 / 폰 접속 중. 피크는 오실로스코프나 피크 홀드가 있으면 측정(송신 피크 300 mA 이상 예상).
   - 레귤레이터 발열, 접속 순간 OLED 깜빡임·리셋이 없는지 확인.
   - 필요하면 `REMOTE_UI_TX_DBM`을 바꿔(예: 0, 10, 17) 비교.
3. **지자기 간섭 (4 Status-Mag/Alt 화면)**
   - 가대를 고정한 채 `Mx/My/Mz`를 Wi-Fi 끔 → AP 켬 → 폰 접속·버튼 연타 순으로 각각 30초 관찰, 평균과 흔들림(최대−최소)을 적습니다.
   - 9 Axis for Moon 화면의 Yaw가 함께 흔들리는지도 봅니다.
   - 차이가 크면 `REMOTE_UI_TX_DBM`을 낮춰 다시 잽니다.
4. **폰 접속 절차**
   1. 부팅 화면에 `6. WiFi AP Telescope`가 나오는지 확인.
   2. 안드로이드 Wi-Fi 설정에서 `Telescope` 선택, 비밀번호 `telescope`. "인터넷 없음" 경고가 나오면 "연결 유지".
   3. Chrome에서 `http://192.168.4.1/` 열기 → `connected`와 OLED와 같은 8줄이 보이는지(메뉴를 로터리로 움직이면 폰도 따라 바뀌는지, 지연 체감).
   4. `+1`, `OK` 버튼 → 페이지 아래 `ESP received: rotate 1, click 1`이 나오는지(이번 단계에서는 화면 커서는 움직이지 않는 것이 정상).
   5. 폰 Wi-Fi 끄기 → `disconnected` 표시 → 다시 켜면 1초 안에 자동 재접속되는지.
   6. 두 번째 폰은 AP 접속이 거부되는지(동시 접속 1대).
   7. 접속한 채 GPS Fix, 시각 표시가 정상인지(GPS는 UART0이라 Wi-Fi와 무관해야 함).
   8. 10분 이상 켜 두고 리셋(부팅 화면 재등장)이 없는지.

#### 0단계 결론 (빌드 기준)

- **ESP8266으로 계속 갑니다.** IRAM은 164 B만 늘었고(Wi-Fi SDK가 원래 들어 있음), RAM +3.7 KB, flash +45 KB로 모두 여유가 있습니다. lwIP 변형을 바꿀 필요도 없었습니다.
- 다만 IRAM 코드 여유가 약 2.2 KB뿐이라 앞으로 `IRAM_ATTR` 함수를 늘리지 않아야 합니다. 부족해지면 MMU `4816`(+16 KB)을 먼저 시도합니다.
- ESP32 검토는 실물 측정에서 ① `Loop`가 자주 100000 µs를 넘거나, ② 송신 출력을 낮춰도 Mag 흔들림이 커서 Yaw가 쓸 수 없거나, ③ 전류 피크로 리셋이 나는 경우에만 합니다.

### 1단계. 화면 미러링 (읽기 전용)
- `src/remote/remote_ui.{h,cpp}`: AP 시작, HTTP로 페이지 제공, WebSocket으로 화면 줄 전송.
- `board_config.h`에 `REMOTE_UI_ENABLE`, AP 이름/비밀번호.
- 폰 페이지: 검은 바탕에 빨간 고정폭 글씨, 화면 꺼짐 방지(Wake Lock).
- 시험: 폰에 화면이 OLED와 같게 나오는지 확인합니다.

#### 1단계 구현 (2026-11-24)
- 폰 페이지 원본: `Arduino/app/src/remote/web/index.html`. 펌웨어와 PC 브리지가 같은 파일을 씁니다.
  - 펌웨어에는 `scripts/gen_remote_page.py`가 만든 `remote_page.h`(PROGMEM)로 들어갑니다. Arduino IDE는 빌드 스크립트를 못 돌리므로 생성 파일도 커밋합니다. 페이지를 고치면 스크립트를 다시 돌립니다(`tools/ui_simulator/run_tests.sh`가 `--check`로 확인).
  - WebSocket 포트는 HTTP 포트 + 1로 계산합니다(ESP 80 → 81, PC 8080 → 8081).
  - 화면: 기본은 **도트 화면**입니다. OLED와 같은 Adafruit 5×7 폰트(glcdfont, BSD 라이선스)를 페이지에 넣고 128×64 점을 캔버스에 둥근 점으로 그립니다. 꺼진 점도 희미하게 보여 도트 매트릭스처럼 보입니다. 픽셀 규칙(6×8 칸, 128 px 줄바꿈, 176 이상 글리프 한 칸 밀림)은 GUI 시뮬레이터의 `oled_render.py`와 같고, 같은 화면을 넣어 8192 픽셀이 모두 같은 것을 확인했습니다.
  - `TEXT` / `DOT` 버튼으로 글자 화면(검은 바탕 빨간 고정폭 글씨, 상태바 줄은 어둡게)과 바꿀 수 있고, 고른 방식은 브라우저에 기억합니다.
  - 상태 줄: `connected` / `connected, no data N s`(3초 넘게 프레임 없음) / `disconnected, retrying...`(1초마다 재접속).
  - 화면 꺼짐 방지: Wake Lock API를 씁니다. 다만 Wake Lock은 HTTPS 또는 localhost에서만 동작해서 `http://192.168.4.1`에서는 안 됩니다. 이때 상태 줄에 `keep-awake needs your phone setting`을 표시하니, 폰의 화면 꺼짐 시간을 길게 설정합니다.
- 메시지: 접속하면 ESP가 `{"hello":{"input":false}}`를 보냅니다. 페이지는 `input`이 true일 때만 입력 버튼을 보여줍니다(2단계부터 true). 화면은 바뀔 때, 그리고 바뀌지 않아도 1초마다 보냅니다.
- PC 데모: `python Arduino/tools/ui_simulator/gui/web_bridge.py` → `http://localhost:8080/`. 펌웨어 UI 러너를 10 Hz로 돌리고 같은 페이지를 제공합니다. 데모에서는 입력 버튼(−10 −1 OK +1 +10)도 동작합니다. `--mirror-only`는 1단계 펌웨어처럼 입력을 무시합니다. 같은 Wi-Fi의 폰에서 `http://<PC IP>:8080/`으로 열어 볼 수도 있습니다(방화벽 허용 필요).
- 실물 확인 (남음): `REMOTE_UI_ENABLE=1`로 올리고 폰을 `Telescope` AP(비밀번호 `telescope`)에 붙여 `http://192.168.4.1`을 엽니다. 폰 화면이 OLED와 같은지, Health 루프 주기가 흔들리지 않는지 확인합니다.

### 2단계. 입력 (다이얼, 클릭)
- 폰 → ESP 입력을 `loop()`에서 합칩니다.
- 가상 다이얼: 원을 드래그하면 15°마다 `rotate ±1`, 짧게 누르면 클릭. 세밀 조정용 `−10 −1 +1 +10` 버튼.
- 연결이 끊기면 원격 입력을 버립니다(남은 회전량이 계속 들어가지 않게).
- 시험: Offset 편집, Track 켜기/끄기를 폰으로만 해봅니다.

#### 2단계 구현 (2026-11-24)
- 펌웨어: `remote::takeInput(rotate, click)`이 쌓인 폰 입력을 꺼내고, `app.ino`의 `loop()`가 로터리 입력에 더합니다(`delta += 원격 회전`, `click = 로터리 클릭 || 원격 클릭`). 접속 시 `{"hello":{"input":true}}`를 보내 페이지에 입력부를 엽니다.
  - 클릭은 한 루프에 하나씩 꺼내고, 쌓아 두는 클릭은 최대 3개입니다(연타가 화면 여러 장을 한꺼번에 넘기지 않게).
  - 연결이 끊기면 쌓인 회전량과 클릭을 버립니다.
  - `REMOTE_UI_ENABLE=0`이면 `takeInput`은 항상 0을 돌려주는 inline 함수라 코드가 늘지 않습니다.
- 페이지: 화면 아래에 가상 다이얼(원 + 손잡이 표시)과 가운데 `OK` 버튼, 그 아래 `−10 −1 +1 +10` 버튼.
  - 다이얼은 원을 따라 끌면 15°마다 한 칸(시계 방향 = +, 로터리와 같음). 한 바퀴 = 24칸. 칸 수는 50 ms마다 모아서 보냅니다.
  - 한 칸마다, 버튼을 누를 때마다 짧게 진동합니다(지원하는 폰).
- PC 데모로 확인: OK로 화면을 나가고, 다이얼 시계 방향 90°(+6칸), 반시계 45°(−3칸)로 메뉴 커서가 그만큼 움직이는 것을 확인했습니다.
- 빌드: 기본(`REMOTE_UI_ENABLE=0`) IRAM 30,319 B로 변화 없음. `REMOTE_UI_ENABLE=1`은 IRAM 30,483 B, flash 366,720 B.
- 실물 확인 (남음): 폰만으로 Offset 편집, Track 켜기/끄기. 다이얼과 로터리를 동시에 돌려도 이상 없는지.

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
