# archive

현재 펌웨어(`Arduino/app`)에서 쓰지 않는 코드를 참고용으로 보관하는 곳입니다. 빌드에는 포함되지 않습니다.

| 폴더 | 내용 |
|---|---|
| `app_unused/` | `app/`에 있었지만 쓰이지 않던 파일 |
| `app_unused/mpu6500.*`, `mpu9250.*`, `invensense_imu.*` | Bolder Flight IMU 드라이버 복사본 (MPU9250/MPU6500 시절, 현재는 ICM20948 사용) |
| `app_unused/mpu9250_custom.h` | 직접 작성하던 MPU9250 레지스터 정의 |
| `app_unused/rotary.*`, `singleton.hpp` | 싱글톤 기반 로터리 클래스 초안 (리팩토링 때 참고) |
| `app_unused/tictok_ms.*` | ms 단위 시간 측정기 (현재는 `tictok_us` 사용) |
| `app_unused/estimation.h` | 빈 헤더 |
| `ws/` | 단계별 실험 스케치 (blink → GPIO → I2C → OLED → GPS → 로터리 → 메뉴 → PWM/스테퍼), EKF 파이썬 시뮬레이션, 계층형 메뉴 초안 `menu_display.h` |
