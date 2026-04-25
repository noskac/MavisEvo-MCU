#include <Arduino.h>

// ================================================================
// DETEKSI BOARD OTOMATIS (Berdasarkan platformio.ini)
// ================================================================
#ifdef BOARD_ESP32
    #include <ESP32Servo.h>
    #define RX_CHANNELS 5
    const uint8_t rxPins[RX_CHANNELS] = {36, 39, 34, 35, 32}; // Input Only Pins
    const uint8_t PIN_DKIRI   = 13;
    const uint8_t PIN_DKANAN  = 12;
    const uint8_t PIN_BKIRI   = 14;
    const uint8_t PIN_BKANAN  = 27;
    const uint8_t PIN_TBKIRI  = 26;
    const uint8_t PIN_TBKANAN = 25;
    const String BOARD_NAME = "ROV MAVIS - ESP32 Mode";

#elif defined(BOARD_TEENSY)
    #include <Servo.h>
    #define RX_CHANNELS 5
    const uint8_t rxPins[RX_CHANNELS] = {2, 3, 4, 5, 6};
    const uint8_t PIN_DKIRI   = 8;
    const uint8_t PIN_DKANAN  = 9;
    const uint8_t PIN_BKIRI   = 12;
    const uint8_t PIN_BKANAN  = 24;
    const uint8_t PIN_TBKIRI  = 25;
    const uint8_t PIN_TBKANAN = 28;
    const String BOARD_NAME = "ROV MAVIS - TEENSY 4.1 Mode";

#else
    #error "Board tidak dikenali! Cek konfigurasi platformio.ini"
#endif

// ================================================================
// KONFIGURASI GLOBAL (Sama untuk semua board)
// ================================================================
#define PWM_MIN       1100
#define PWM_MAX       1900
#define PWM_NEUTRAL   1500
#define RX_MIN        1089
#define RX_MAX        1924
const unsigned long AUTO_SERIAL_TIMEOUT = 400;

// ================================================================
// CLASS THRUSTER
// ================================================================
class Thruster {
public:
    void attach(uint8_t pin, bool reversed = false) {
        this->reversed = reversed;
        
        #ifdef BOARD_ESP32
            esc.setPeriodHertz(50); // Khusus ESP32 perlu set frekuensi
        #endif
        
        esc.attach(pin, PWM_MIN, PWM_MAX);
        esc.writeMicroseconds(PWM_NEUTRAL);
    }

    void write(int value) {
        if (reversed) value = PWM_NEUTRAL * 2 - value;
        esc.writeMicroseconds(constrain(value, PWM_MIN, PWM_MAX));
    }
private:
    Servo esc;
    bool reversed = false;
};

// ================================================================
// TIPE DATA & VARIABEL GLOBAL
// ================================================================
struct AutoCommand { int surge=0, lateral=0, heave=0, yaw=0, mode=0; };
struct ThrusterOutput { int DKIRI, DKANAN, BKIRI, BKANAN, TBKIRI, TBKANAN; };

Thruster DKIRI, DKANAN, BKIRI, BKANAN, TBKIRI, TBKANAN;
uint16_t channel[RX_CHANNELS] = {0};
unsigned long lastPrint = 0;
const unsigned long PRINT_INTERVAL = 250;
AutoCommand autoCmd;
unsigned long lastAutoSerial = 0;

// ================================================================
// LOGIKA MIXING (Sama untuk semua board)
// ================================================================
ThrusterOutput mixManual(const uint16_t ch[]) {
    ThrusterOutput out;
    int lateral, surge, heave, yaw;

    if (autoCmd.mode == 1) { // Dari Keyboard Laptop
        surge = autoCmd.surge; lateral = autoCmd.lateral;
        heave = autoCmd.heave; yaw = autoCmd.yaw;
    } else { // Dari Remote RC
        lateral = map(ch[0], RX_MIN, RX_MAX, PWM_MIN, PWM_MAX) - PWM_NEUTRAL;
        surge   = map(ch[1], RX_MIN, RX_MAX, PWM_MIN, PWM_MAX) - PWM_NEUTRAL;
        heave   = map(ch[2], RX_MIN, RX_MAX, PWM_MIN, PWM_MAX) - PWM_NEUTRAL;
        yaw     = map(ch[3], RX_MIN, RX_MAX, PWM_MIN, PWM_MAX) - PWM_NEUTRAL;
    }

    out.TBKIRI  = PWM_NEUTRAL + surge + yaw;
    out.TBKANAN = PWM_NEUTRAL + surge - yaw;
    out.DKIRI   = PWM_NEUTRAL + heave - lateral;   
    out.DKANAN  = PWM_NEUTRAL + heave + lateral;
    out.BKIRI   = PWM_NEUTRAL + heave + lateral;
    out.BKANAN  = PWM_NEUTRAL + heave - lateral;
    return out; // Constraint dilewati di bagian class Thruster
}

ThrusterOutput mixAuto(const AutoCommand& cmd) {
    ThrusterOutput out;
    int surge = constrain(cmd.surge, -500, 500);
    int lateral = constrain(cmd.lateral, -500, 500);
    int heave = constrain(cmd.heave, -500, 500);
    int yaw = constrain(cmd.yaw, -500, 500);

    out.DKIRI   = PWM_NEUTRAL + surge + lateral + yaw;
    out.DKANAN  = PWM_NEUTRAL + surge - lateral - yaw;
    out.BKIRI   = PWM_NEUTRAL + surge + lateral - yaw;
    out.BKANAN  = PWM_NEUTRAL + surge - lateral + yaw;
    out.TBKIRI  = PWM_NEUTRAL + heave + yaw;
    out.TBKANAN = PWM_NEUTRAL + heave - yaw;
    return out;
}

// ================================================================
// BACA DATA SERIAL DARI ROS 2
// ================================================================
bool readAutoCommand() {
    if (!Serial.available()) return false;
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return false;

    int values[5] = {0}; 
    int count = 0, start = 0;
    int comma = line.indexOf(',');

    while (comma >= 0 && count < 5) {
        values[count++] = line.substring(start, comma).toInt();
        start = comma + 1;
        comma = line.indexOf(',', start);
    }
    if (count < 5) values[count++] = line.substring(start).toInt();

    if (count == 5) {
        autoCmd.surge = values[0]; autoCmd.lateral = values[1];
        autoCmd.heave = values[2]; autoCmd.yaw = values[3]; autoCmd.mode = values[4];
        lastAutoSerial = millis();
        return true;
    }
    return false;
}

// ================================================================
// SETUP
// ================================================================
void setup() {
    Serial.begin(115200);
    delay(500);
    Serial.println(BOARD_NAME + " Initialized");

    #ifdef BOARD_ESP32
        ESP32PWM::allocateTimer(0);
        ESP32PWM::allocateTimer(1);
        ESP32PWM::allocateTimer(2);
        ESP32PWM::allocateTimer(3);
    #endif

    DKIRI.attach(PIN_DKIRI, true);
    DKANAN.attach(PIN_DKANAN, false);
    BKIRI.attach(PIN_BKIRI, true);
    BKANAN.attach(PIN_BKANAN, true);
    TBKIRI.attach(PIN_TBKIRI, true);
    TBKANAN.attach(PIN_TBKANAN, true);
}

// ================================================================
// LOOP
// ================================================================
void loop() {
    readAutoCommand();

    for (int i = 0; i < RX_CHANNELS; i++) {
        unsigned long pw = pulseIn(rxPins[i], HIGH, 15000UL);
        if (pw >= 800 && pw <= 2300) channel[i] = static_cast<uint16_t>(pw);
        else channel[i] = PWM_NEUTRAL;
    }

    // NAMA ENUM DIGANTI AGAR TIDAK BENTROK DENGAN MACRO ESP32
    enum class ControlMode { ModeManual, ModeDisabled, ModeAuto };
    ControlMode currentMode = ControlMode::ModeDisabled;

    if (millis() - lastAutoSerial <= AUTO_SERIAL_TIMEOUT) {
        if (autoCmd.mode == 1) currentMode = ControlMode::ModeManual;
        else if (autoCmd.mode == 2) currentMode = ControlMode::ModeAuto;
    } else {
        uint16_t modeValue = channel[4]; 
        if (modeValue > 800 && modeValue < 1400) currentMode = ControlMode::ModeManual;
        else if (modeValue > 1600) currentMode = ControlMode::ModeAuto;
    }

    ThrusterOutput out;
    if (currentMode == ControlMode::ModeManual) out = mixManual(channel);
    else if (currentMode == ControlMode::ModeAuto) out = mixAuto(autoCmd);
    else out = {PWM_NEUTRAL, PWM_NEUTRAL, PWM_NEUTRAL, PWM_NEUTRAL, PWM_NEUTRAL, PWM_NEUTRAL};

    DKIRI.write(out.DKIRI); DKANAN.write(out.DKANAN); BKIRI.write(out.BKIRI);
    BKANAN.write(out.BKANAN); TBKIRI.write(out.TBKIRI); TBKANAN.write(out.TBKANAN);

    unsigned long now = millis();
    if (now - lastPrint >= PRINT_INTERVAL) {
        lastPrint = now;
        Serial.print("Mode: ");
        if (currentMode == ControlMode::ModeManual) Serial.print("MANUAL");
        else if (currentMode == ControlMode::ModeAuto) Serial.print("AUTO");
        else Serial.print("DISABLED");
        
        Serial.print(" | DK:"); Serial.print(out.DKIRI);
        Serial.print(" DKn:"); Serial.print(out.DKANAN);
        Serial.print(" BK:"); Serial.print(out.BKIRI);
        Serial.print(" BKn:"); Serial.print(out.BKANAN);
        Serial.print(" TBK:"); Serial.print(out.TBKIRI);
        Serial.print(" TBKn:"); Serial.println(out.TBKANAN);
    }
}