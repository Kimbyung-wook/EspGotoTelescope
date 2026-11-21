# OLED UI 시뮬레이터

PC에서 망원경 펌웨어의 OLED 메뉴를 돌려 보는 도구입니다. 하드웨어 없이 로터리 회전·클릭과 가짜 센서 값으로 화면 전환과 모터 출력을 확인할 수 있습니다.

```
gui/simulator.py  (tkinter)  ──stdin: input/sensors/tick──▶  build/ui_runner.exe  (C++)
  128×64 화면, 센서 패널,    ◀──stdout: frame(8줄+모터)───    Session ─ UiBackend ─ MockUi
  모터 표시                                                              (나중에 실제 펌웨어 UI)
```

- **UI 러너** (`runner/`): C++14 콘솔 프로그램. 한 줄 JSON으로 입력·센서 값을 받고 10 Hz 프레임마다 화면 8줄과 모터 출력을 돌려줍니다. 프로토콜은 [PROTOCOL.md](PROTOCOL.md).
- **UI 백엔드 경계**: UI 로직은 `runner/ui_backend.h`의 `sim::UiBackend` 뒤에만 있습니다. 지금은 [docs/UI.md](../../../docs/UI.md)를 따라 만든 목업 `MockUi`이고, 펌웨어 UI가 `Screen`/`ScreenManager`로 정리되면 그 코드를 감싼 백엔드로 바꿔 끼웁니다.
- **GUI** (`gui/`): Python 3.12 표준 라이브러리(tkinter)만 씁니다. 추가 패키지가 필요 없습니다.

## 빌드와 실행 (Git Bash)

```bash
cd Arduino/tools/ui_simulator
./build.sh                     # build/ui_runner.exe (MinGW g++ 6.3, -std=c++14)
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
./run_tests.sh                 # 러너 메뉴 로직 테스트 (입력 순서 -> 기대 화면 줄)
python gui/protocol_check.py   # 실제 러너 프로세스와 JSON 프로토콜로 모든 메뉴 왕복 (GUI 없음)
python gui/simulator.py --shots build/shots   # GUI 자동 순회 + 스크린샷 저장 후 종료
```

`--shots`는 정해진 순서로 모든 메뉴를 돌며 화면 PNG(`build/shots/NN_*.png`), 창 전체 캡처(`window_*.png`), 각 화면 텍스트(`tour_log.txt`)를 남깁니다.

## 폰트

화면 글꼴은 Adafruit GFX 기본 5×7 폰트(`glcdfont.c`)를 변환한 `gui/glcdfont_data.py`입니다. 원본의 BSD 라이선스 문구를 파일 머리에 그대로 두었습니다. 라이브러리를 업데이트한 뒤 다시 만들려면:

```bash
python gui/make_font.py   # 기본 경로: ~/Documents/Arduino/libraries/Adafruit_GFX_Library/glcdfont.c
```

## 목업과 실제 펌웨어의 차이

목업은 `docs/UI.md`의 **설계**를 따릅니다. 현재 `app.ino`와 다른 점:

- 숫자 서식 `%9.4f`(코드는 `%10.5f`), 메뉴 문자열 `2Status-GPS`처럼 공백 없음.
- `4 Status-Mag/Alt`에 HEAD(지자기 방위)와 온도를 함께 표시.
- `5 Status-IMUCalib`은 `6 Do IMU Calib.`의 Gyro Bias 보정 결과를 보여줌. Acc Bias/Scale, Gyro Scale은 "Not implemented" 화면.
- `7 Att.Estimator`는 13항목 스크롤 목록 + Exit. R/Q/K는 고정 예시 값.
- `8 Health`의 시간, `9 Axis for Moon`의 달 R.A./DEC.는 고정 예시 값.
- 하위 화면 전환은 클릭한 그 프레임에 바로 그려짐(펌웨어는 다음 프레임).
- RRcmd는 로터리 1틱 = 1, 값이 곧 PWM 듀티(부호 = 방향), ±255로 제한.

## 실제 펌웨어 UI 연결하기 (예정)

1. `runner/firmware_ui.cpp`에 `sim::UiBackend`를 구현합니다. `begin()`에서 화면 관리자를 만들고, `frame()`에서 `InputSnapshot`·`SensorState`를 펌웨어가 읽는 형태(Rotary 카운트/클릭, GPS·IMU 값)로 넘긴 뒤 `msg[8][22]`를 `Frame`으로, 모터 핀 상태를 `MotorOutput`으로 복사합니다.
2. 펌웨어 헤더에 필요한 호스트 스텁(`Arduino.h` 등)은 이 폴더의 `stubs/`에 따로 둡니다(`Arduino/test/stubs`는 단위 테스트 전용).
3. `runner/backends.cpp`의 `createBackend()`에 등록하고 `build.sh`에 펌웨어 소스 경로를 추가합니다.
4. GUI는 그대로 두고 `python gui/simulator.py --backend firmware`로 실행합니다(러너에 `--backend firmware`가 전달됨).
