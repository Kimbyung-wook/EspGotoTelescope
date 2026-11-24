# UI 시뮬레이터 프로토콜 (버전 1)

GUI(`gui/simulator.py`)와 UI 러너(`build/ui_runner.exe`)가 주고받는 메시지입니다.

- 전송: 러너의 **stdin/stdout**. 메시지 하나는 **JSON 객체 한 줄**이고 `\n`으로 끝납니다(`\r\n`도 됩니다). 빈 줄은 무시합니다.
- 문자: ASCII만 씁니다. 화면 글자 가운데 0x20 미만이나 0x7F 이상인 바이트는 `\u00XX`로 보내므로 JSON을 디코드하면 `chr(바이트)`가 됩니다. 이 값이 곧 glcdfont 글리프 번호입니다.
- 시간: 러너는 스스로 시계를 돌리지 않습니다. **`tick` 하나가 10 Hz 프레임 하나(100 ms)** 이고, 시뮬레이션 시각 `t_ms`는 tick마다 100씩 늘어납니다. GUI가 실제 시간으로 100 ms마다 `tick`을 보내서 실시간처럼 돌리고, 테스트는 원하는 만큼 몰아서 보낼 수 있습니다(결과가 늘 같음).
- 입력: `input`은 바로 처리하지 않고 쌓아 두었다가 다음 `tick`에서 한꺼번에 UI에 넘깁니다. 펌웨어에서 인터럽트가 카운트를 쌓고 `loop()`가 프레임마다 `rotary.count()`/`takeClick()`로 읽는 것과 같은 모양입니다.

## GUI → 러너

| `type` | 필드 | 동작 | 응답 |
|---|---|---|---|
| `input` | `rotate`: 정수(+ = CW, − = CCW, 생략 시 0)<br>`click`: 클릭 횟수(생략 시 0) | 로터리 누적 카운트에 더하고 클릭 수를 쌓음 | 없음 |
| `sensors` | `gps`: 객체(선택)<br>`imu`: 객체(선택)<br>`power`: `{"v": 전압}`(선택, 기본 12.60)<br>`remote`: `{"state": 0/1/2}`(선택, 기본 0) | 준 필드만 바꿈. 나머지는 이전 값 유지. `remote`는 상태바 Wi-Fi 표시(0 없음, 1 `w` AP 대기, 2 `W` 폰 접속) | 없음 |
| `phone_time` | `unix_s`: Unix 초<br>`ms`: 밀리초(0~999) | 원격 UI 페이지가 보낸 폰 시각(DES-LOC-05). 다음 `tick`에서 UI에 넘김. GPS fix가 없으면 수동 시각보다 앞서 쓰고 상태바가 `P`로 바뀜 | 없음 |
| `tick` | `n`: 프레임 수(기본 1, 1~1000) | 프레임 `n`개 실행 | `frame` `n`개 |
| `reset` | `imu_ok`, `gps_ok`: bool(선택, 기본은 이전 값, 처음엔 true) | 재부팅: 시각 0, 로터리 카운트 0, 부팅 화면부터 다시 | 없음 |
| `hello` | | | `hello` |
| `quit` | | 러너 종료(종료 코드 0) | 없음 |

`gps` 필드 (Adafruit_GPS와 같은 의미)

| 키 | 뜻 | 기본값 |
|---|---|---|
| `year` | 연도 끝 두 자리 | 26 |
| `month`, `day` | 월, 일 | 11, 21 |
| `hour`, `minute`, `seconds`, `milliseconds` | UTC 시각 | 12, 0, 0, 0 |
| `lat`, `lon` | 위도·경도 [deg], 북·동이 + | 37.5665, 126.9780 |
| `fix` | Fix 품질: 0 NoFix, 1 GPS, 2 DGPS | 1 |
| `sats` | 위성 수 | 8 |

`imu` 필드 (펌웨어 `loop()`에서 부호를 바꾼 **뒤**의 기체 좌표 값)

| 키 | 단위 | 기본값 |
|---|---|---|
| `ax`, `ay`, `az` | m/s² | 0, 0, 9.80665 |
| `wx`, `wy`, `wz` | deg/s | 0, 0, 0 |
| `mx`, `my`, `mz` | µT | 20, 0, −40 |
| `temp` | °C | 22 |

## 러너 → GUI

시작하자마자 `hello`를 한 번 보냅니다.

```json
{"type":"hello","version":1,"backend":"firmware","lines":8,"cols":21,"frame_ms":100}
```

`tick`마다 `frame`을 보냅니다.

```json
{"type":"frame","seq":34,"t_ms":3400,
 "lines":["T 12: 0: 3.4 V 12.60","Main Menu",">1Target"," 2Polar Align"," 3Status"," 4Calibration"," 5Settings"," 6RA Speed"],
 "motor":{"in1":0,"in2":0,"pwm":0}}
```
(실제로는 한 줄입니다.)

| 필드 | 뜻 |
|---|---|
| `seq` | 부팅 뒤 프레임 번호(0부터). `reset`해도 계속 늘어남 |
| `t_ms` | 이 프레임의 시뮬레이션 시각(부팅 뒤 ms) |
| `lines` | 화면 8줄, 줄당 최대 21자. 펌웨어 `monitor->msg[0..7]`에 해당. 빈 줄은 `""` |
| `motor.in1`, `motor.in2` | H-브리지 입력 핀 레벨 0/1 (`MOTOR_IN1`, `MOTOR_IN2`) |
| `motor.pwm` | `MOTOR_PWM` 듀티 0~255 |

잘못된 메시지에는 `error`로 답하고 계속 동작합니다.

```json
{"type":"error","msg":"bad json: expected string"}
{"type":"error","msg":"unknown type: foo"}
```

## 화면 그리기 규칙 (GUI 쪽)

펌웨어 `OLED::print()`는 `clearDisplay()` → `setCursor(0,0)` → 8줄을 `println()` 합니다. GUI(`gui/oled_render.py`)도 Adafruit GFX 기본 폰트로 똑같이 그립니다.

- 글자 칸 6×8 px(5×7 글리프 + 1 px 간격), 배경 투명, 텍스트 크기 1
- 줄마다 `\n`으로 다음 8 px 줄로 이동. 21자를 넘으면 자동 줄바꿈(wrap)되어 아래 줄을 밀어냄
- `_cp437 == false` 기본값이므로 176 이상 코드는 글리프 번호가 1 밀림

## UI 백엔드 경계

러너 안에서 UI는 `runner/ui_backend.h`의 `sim::UiBackend` 하나로만 연결됩니다.

```cpp
class UiBackend {
  virtual const char* name() const = 0;
  virtual void begin(const BootConfig&, const SensorState&) = 0;     // setup()
  virtual void frame(unsigned long now_ms, const InputSnapshot&,     // loop() 한 번
                     const SensorState&, Frame& out, MotorOutput& motor) = 0;
};
```

- `InputSnapshot.count`는 부팅 뒤 누적 회전 수(`Rotary::count()`), `clicks`는 직전 프레임 뒤 클릭 수(`Rotary::takeClick()`는 `clicks > 0`).
- 구현은 `FirmwareUi`(`runner/firmware_ui.*`, 백엔드 이름 `firmware`, 기본값) 하나로, 실제 펌웨어 `SimpleGoToUi::step()`을 10 Hz 프레임마다 호출합니다. 다른 UI를 붙이려면 `UiBackend`를 구현해 `runner/backends.cpp`의 `createBackend()`에 등록하고 `ui_runner --backend 이름`(GUI는 `simulator.py --backend 이름`)으로 고릅니다. 프로토콜과 GUI는 바꿀 필요가 없습니다.
