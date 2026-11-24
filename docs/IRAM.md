# NodeMCU v3 (ESP8266) IRAM 확보 방안

빌드 로그의 "IRAM 96%"가 무엇으로 차 있는지 재고, 공간을 늘리는 방법을 비교했습니다.
측정: core 3.1.2, `nodemcuv2:xtal=160`, 커밋 c7e82a3 (2026-11-24).

## 1. IRAM이 무엇으로 차 있나

ESP8266의 명령 RAM은 64 KB이고, 기본 설정(`mmu=3232`)에서는 32 KB를 flash 캐시로, 32 KB를 IRAM 코드로 씁니다. 빌드 로그의 "63087 / 65536 (96%)"는 캐시 32768 B를 포함한 숫자입니다.

| 빌드 | IRAM 코드 | 32 KB 중 |
|---|---:|---:|
| 빈 스케치 (`setup(){}` `loop(){}`) | 26 375 B | 80% |
| 현재 펌웨어 | 30 319 B | 93% (남은 공간 2.4 KB) |

**IRAM의 대부분(26 KB)은 SDK와 코어가 차지합니다.** Wi-Fi MAC(lmac, pp, wDev), 타이머, 힙(umm), flash 함수가 여기 들어갑니다. 우리 코드가 더하는 것은 약 4 KB뿐입니다.

빈 스케치와 비교해 늘어난 IRAM (심볼 기준 약 4.0 KB):

| 항목 | 크기 | 출처 | 줄일 수 있나 |
|---|---:|---|---|
| PWM 타이머 ISR (`timer1Interrupt`, waveform) | ~1.2 KB | `analogWrite` (RA 모터) | 아니오 (모터 PWM에 필요) |
| I2C (`Twi::onTwipEvent` 등) | ~0.6 KB | `Wire` (OLED, IMU) | 코어 수정 없이는 어려움 |
| SoftwareSerial ISR | ~0.46 KB | `Adafruit_GPS`가 기본으로 포함 (우리는 안 씀) | **예**: `NO_SW_SERIAL` |
| UART ISR | ~0.4 KB | GPS (UART0) | 아니오 |
| 인터럽트 공통, `detachInterrupt` | ~0.4 KB | 로터리 | 아니오 |
| 로터리 ISR (`Rotary::onEdge` 등) | ~0.26 KB | 로터리 | 아니오 |
| `digitalRead/Write` | ~0.3 KB | ISR에서 씀 | 아니오 |

Wi-Fi 코드는 SDK에 이미 들어 있어 빈 스케치에도 포함됩니다. 원격 UI(Wi-Fi AP + WebSocket, `REMOTE_UI_ENABLE 1`)를 켜도 IRAM은 **164 B**만 늘었습니다(실측, `docs/REMOTE_UI_PLAN.md` 0단계 표).

## 결정 (2026-11-24)
지금은 IRAM을 따로 확보하지 않습니다. 남은 공간(약 2.2 KB, 원격 UI 포함)으로 계획된 기능이 들어가고, IRAM이 넘치면 링크 단계에서 바로 실패하므로 그때 대응합니다. 대응 순서: ① `NO_SW_SERIAL`(대가 없음, 0.6 KB) → ② `mmu=4816`(+16 KB, 실물 속도 확인).
확보가 필요해지는 경우: 타이머 ISR로 스테퍼를 돌리거나 축 엔코더 펄스를 세는 등 ISR을 늘릴 때, 새 라이브러리로 IRAM 초과가 날 때.

## 2. 방안 비교

| 방안 | IRAM 코드 공간 | 남는 공간 | 대가 |
|---|---:|---:|---|
| 현재 (`mmu=3232`) | 32 KB | 2.4 KB | — |
| A. `NO_SW_SERIAL` 정의 | 32 KB | 3.1 KB (+0.64 KB 실측) | 빌드 옵션이 필요 (IDE에서는 설정하기 번거로움) |
| **B. `mmu=4816` (16 KB 캐시 + 48 KB IRAM)** | 48 KB | **18.8 KB** (실측: 46 719 / 65 536, 71%) | flash 캐시가 32 → 16 KB. flash에서 도는 코드(천체 계산, UI)가 느려질 수 있음 |
| C. `mmu=4816H` | 48 KB (남는 부분은 2번째 힙) | 위와 같음 | 위와 같음 + 힙 구분 관리 |
| D. ESP32로 교체 | 충분 | — | 보드/배선 교체 |

### 추천: B (`mmu=4816`)
- 코드 변경 없이 보드 옵션 하나로 IRAM이 16 KB 늘어납니다.
- 대가는 캐시 크기입니다. 우리 루프는 매 프레임 같은 코드(UI, 자세 추정, 1초마다 천체 계산)를 돌아 캐시 적중률이 높은 편이라 영향이 작을 것으로 봅니다. **실물에서 측정해 확인해야 합니다.**
- A는 효과가 작아(0.6 KB) B만으로 부족할 때 더합니다.

## 3. 적용과 확인 절차

1. 실물 측정 (현재 설정과 `mmu=4816` 각각):
   - `tools/astro_bench` 벤치 스케치: 천체 계산 시간
     ```
     SKETCH=tools/astro_bench FQBN=esp8266:esp8266:nodemcuv2:xtal=160,mmu=4816 Arduino/scripts/build.sh --library app/src/astro --upload -p COMx
     ```
   - 본 펌웨어 8 Health 화면: 루프 주기, `EST`, `AST` 값.
2. 느려진 정도가 루프 주기 안에 들어오면 `build.sh`의 기본 FQBN을 `esp8266:esp8266:nodemcuv2:xtal=160,mmu=4816`으로 바꿉니다. Arduino IDE에서는 Tools > MMU > "16KB cache + 48KB IRAM (IRAM)"을 고릅니다.
3. 설정을 빠뜨리지 않도록 `app.ino`에 160 MHz 검사처럼 `MMU_IRAM_SIZE` 검사를 넣습니다.
   ```cpp
   #if MMU_IRAM_SIZE < 0xC000
   #warning "Tools > MMU: select 16KB cache + 48KB IRAM"
   #endif
   ```
4. 단위/시뮬레이터 테스트는 영향이 없습니다(호스트 빌드).

## 4. 앞으로 IRAM을 아끼는 규칙
- `IRAM_ATTR`은 ISR과 ISR에서 부르는 함수에만 붙입니다.
- ISR에서 무거운 계산이나 `double` 연산을 하지 않습니다. soft-float 루틴이 IRAM에 끌려올 수 있습니다.
- 라이브러리를 추가하면 빌드 로그의 IRAM 숫자를 이 문서의 표와 비교합니다.
