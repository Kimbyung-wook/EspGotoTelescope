# OLED UI 시뮬레이터

PC에서 망원경 펌웨어의 OLED 메뉴를 돌려 보는 도구입니다. 하드웨어 없이 로터리 회전·클릭과 가짜 센서 값으로 화면 전환과 모터 출력을 확인할 수 있습니다.

```
gui/simulator.py  (tkinter)  ──stdin: input/sensors/tick──▶  build/ui_runner.exe  (C++)
  128×64 화면, 센서 패널,    ◀──stdout: frame(8줄+모터)───    Session ─ UiBackend ─ FirmwareUi
  모터 표시                                                       └ SimpleGoToUi (Arduino/app/src/ui)
```

- **UI 러너** (`runner/`): C++14 콘솔 프로그램. 한 줄 JSON으로 입력·센서 값을 받고 10 Hz 프레임마다 화면 8줄과 모터 출력을 돌려줍니다. 프로토콜은 [PROTOCOL.md](PROTOCOL.md).
- **UI 백엔드 경계**: UI는 `runner/ui_backend.h`의 `sim::UiBackend` 뒤에만 있습니다. 구현은 `FirmwareUi`(`runner/firmware_ui.*`) 하나로, **실제 펌웨어 UI 코드**(`Arduino/app/src/ui`의 `SimpleGoToUi`, `Screen`/`ScreenManager`)를 그대로 컴파일해 돌립니다. 펌웨어 소스는 읽기만 하고 고치지 않습니다.
- **GUI** (`gui/`): Python 3.12 표준 라이브러리(tkinter)만 씁니다. 추가 패키지가 필요 없습니다.

## 빌드와 실행 (Git Bash)

```bash
cd Arduino/tools/ui_simulator
./build.sh                     # build/ui_runner.exe (MinGW g++ 6.3, -std=c++14, 펌웨어 UI 소스 포함)
python gui/simulator.py        # GUI 실행 (--scale 4 로 확대 배율 변경)
```

PowerShell에서 GUI만 띄울 때:

```powershell
python Arduino\tools\ui_simulator\gui\simulator.py
```

## 조작

| 동작 | 입력 |
|---|---|
| 로터리 CW (커서 아래 / 값 +) | `→` `↓`, 화면 위 휠 아래로, `CW ▶` 버튼 |
| 로터리 CCW (커서 위 / 값 −) | `←` `↑`, 화면 위 휠 위로, `◀ CCW` 버튼 |
| 버튼 클릭 | `Enter` `Space`, 화면 클릭, `● 클릭` 버튼 |

센서 입력칸에 커서가 있으면 화살표·Enter는 입력칸 편집에 쓰입니다. 화면을 한 번 클릭하거나 빈 곳을 누른 뒤 조작하세요.

- **센서 패널**: GPS(날짜·시각·위경도·Fix·위성 수)와 IMU(가속도·자이로·지자기·온도)를 직접 고칩니다. 값은 매 프레임 러너로 보냅니다.
- **프리셋**: `정지(수평)`, `기울임(피치=위도)` — 극축 정렬된 마운트처럼 피치를 위도에 맞춤, `회전(wz 15 dps)` — 자이로 값과 함께 지자기 벡터를 돌려 방위가 바뀜.
- **옵션**: 센서 잡음, GPS 시각 자동 진행(프레임마다 +100 ms), wz로 지자기 회전.
- **모터 출력**: IN1/IN2 램프, 방향, PWM 듀티 막대(0~255).
- **실행**: 일시정지·1프레임 진행, 재부팅(IMU/GPS 실패 부팅 재현), 화면 PNG 저장, 패널 색(흰색/파랑/노랑·파랑 2색), 픽셀 격자.

## 테스트

```bash
./run_tests.sh                 # 실제 펌웨어 UI로 메뉴 테스트 (입력 순서 -> 기대 화면 줄)
python gui/protocol_check.py   # 실제 러너 프로세스와 JSON 프로토콜로 모든 메뉴 왕복 (GUI 없음)
python gui/simulator.py --shots build/shots   # GUI 자동 순회 + 스크린샷 저장 후 종료
```

`--shots`는 정해진 순서로 모든 메뉴를 돌며 화면 PNG(`build/shots/NN_*.png`), 창 전체 캡처(`window_*.png`), 각 화면 텍스트(`tour_log.txt`)를 남깁니다.

## 폰트

화면 글꼴은 Adafruit GFX 기본 5×7 폰트(`glcdfont.c`)를 변환한 `gui/glcdfont_data.py`입니다. 원본의 BSD 라이선스 문구를 파일 머리에 그대로 두었습니다. 라이브러리를 업데이트한 뒤 다시 만들려면:

```bash
python gui/make_font.py   # 기본 경로: ~/Documents/Arduino/libraries/Adafruit_GFX_Library/glcdfont.c
```

## 펌웨어 UI를 돌리는 방식

`FirmwareUi`는 `app.ino`가 보드에서 하는 일을 PC에서 똑같이 합니다.

| `app.ino` | 시뮬레이터 (`runner/firmware_ui.cpp`) |
|---|---|
| `setup()`의 부팅 로그 | 같은 문구를 3프레임마다 한 줄씩 보여 주고 10프레임 유지 (`setup()`은 하드웨어 라이브러리 때문에 PC에서 빌드되지 않아 문구만 옮김). 부팅 중 회전·클릭은 버림 |
| `rotary.count()` 차이, `takeClick()` | 프로토콜 `input`을 모은 값 |
| `updateTelemetry()` | 가짜 센서 값 → `Telemetry` (롤/피치는 `rollFromAccelDeg`/`pitchFromAccelDeg`, `imu_corr`는 `withoutGyroBias(imu, gyro_bias)`). Health 시간은 보드에서 잰 듯한 고정 값 (EST 180 us 등) |
| `att_est.update(imu_corr, dt)` | 매 프레임 `AttitudeEstimator::update(imu_corr, 0.1f)` → `roll_est_deg`/`pitch_est_deg`. 시정수 `ATT_EST_TAU_S`는 `app.ino`에 있어 `firmware_ui.h`의 `kAttEstTauS`(1.0)로 복사해 둠 |
| `ui.step(delta, click, ctx, frame)` | 그대로 호출, `Frame.lines`를 응답 8줄로 |
| `ra_motor.setSpeed(ra_speed_cmd)` | `RaMotor::toOutput(ra_speed_cmd)` → IN1/IN2/PWM |

빌드에 쓰는 펌웨어 소스와 include 경로는 `sources.sh`에 있습니다. `Arduino.h`는 이 폴더의 `stubs/`(핀 함수는 빈 함수)를 씁니다. `Arduino/test/stubs`는 단위 테스트용이라 따로 둡니다. 펌웨어 UI에 화면이나 소스 파일이 늘면 `sources.sh`의 `FIRMWARE_SRCS`에 추가하고 `tests/test_menu.cpp`의 기대 화면을 맞춥니다.

재부팅(`reset`)하면 `SimpleGoToUi`, `UiContext`, `AttitudeEstimator`를 새로 만들어 전원을 다시 켠 것처럼 처음부터 시작합니다.
