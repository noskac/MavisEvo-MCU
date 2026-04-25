# ⚡ MavisEvo-MCU (UAV Firmware) (ONGOING)

*Firmware* universal berbasis C++ untuk sistem aktuator Remotely Operated Vehicle (ROV) MavisEvo. Repositori ini merupakan jembatan perangkat keras (*hardware bridge*) yang menerima perintah kecepatan (*surge, lateral, heave, yaw*) dari ROS 2 melalui komunikasi Serial, dan mengubahnya menjadi sinyal PWM presisi untuk mengendalikan ESC (*Electronic Speed Controller*).

## ✨ Fitur Utama
* **Universal Codebase:** Menggunakan *C++ Preprocessor Directives* (`#ifdef`) sehingga satu kode sumber bisa dikompilasi langsung untuk **ESP32** maupun **Teensy 4.1** tanpa modifikasi ulang.
* **Hardware Timer PWM:** Dioptimalkan dengan *library* `ESP32Servo` (untuk ESP32) guna memanfaatkan *timer hardware* (RTOS) agar sinyal ESC 50Hz sangat stabil dan tidak bergetar (*jitter*).
* **Multi-Mode Control:** Mendukung transisi mulus antara Mode AI (Auto dari ROS 2), Mode Manual (Keyboard/Remote), dan Failsafe (Disabled).
* **Custom Serial Protocol:** Mampu melakukan *parsing* data *string* koma berkecepatan tinggi (115200 baud) secara *real-time*.

## 🛠️ Tech Stack & Environment
* **Platform:** PlatformIO (VS Code)
* **Framework:** Arduino
* **Supported Boards:** ESP32 Dev Module, Teensy 4.1

---
*Developed by Andaru Wicaksono.*