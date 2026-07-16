#define BLYNK_TEMPLATE_ID "TMPL68hsp-Vx_"
#define BLYNK_TEMPLATE_NAME "laundry"
#define BLYNK_AUTH_TOKEN "Sx1p16BjfYdz_qOJwIuvr0lrhXG8_bF-"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>

char ssid[] = "Anh Duc";
char pass[] = "anhduc2009@";

// ESP32 RX2 GPIO16 nối TX STM32 (PA9)
// ESP32 TX2 GPIO17 nối RX STM32 (PA10)
#define STM32_RX_PIN 16
#define STM32_TX_PIN 17

BlynkTimer timer;

String lastCommand  = "";
String activeService = "";
String stm32Buffer  = "";

// --- Local countdown: ESP32 tự đếm ngược, đồng bộ khi nhận TIME: ---
// Nhờ đó Blynk luôn mượt, không phụ thuộc vào tần suất UART
volatile int  localCountdown  = 0;   // giây còn lại (ESP32 tự quản lý)
unsigned long lastCountdownMs = 0;   // millis() lần cuối đếm ngược
bool          machineBusy     = false;

// -----------------------------------------------------------------------

void updateServicePins(const String &service) {
  if (service == "WASH") {
    Blynk.virtualWrite(V4, 1);
    Blynk.virtualWrite(V5, 0);
  } else if (service == "DRY") {
    Blynk.virtualWrite(V4, 0);
    Blynk.virtualWrite(V5, 1);
  } else if (service == "WASHDRY") {
    Blynk.virtualWrite(V4, 1);
    Blynk.virtualWrite(V5, 1);
  } else {
    Blynk.virtualWrite(V4, 0);
    Blynk.virtualWrite(V5, 0);
  }
}

void resetBlynkOutput() {
  Blynk.virtualWrite(V2, "NONE");
  Blynk.virtualWrite(V3, 0);
  Blynk.virtualWrite(V4, 0);
  Blynk.virtualWrite(V5, 0);
  activeService  = "";
  localCountdown = 0;
  machineBusy    = false;
}

// Đặt lại countdown và trạng thái về idle (dùng khi FREE/FINISHED)
void stopCountdown() {
  machineBusy    = false;
  localCountdown = 0;
  Blynk.virtualWrite(V3, 0);
}

String extractServiceFromResponse(const String &response) {
  int spacePos = response.indexOf(' ');
  if (spacePos > 0) return response.substring(0, spacePos);
  if (activeService != "") return activeService;
  if (lastCommand   != "") return lastCommand;
  return "NONE";
}

void sendToSTM32(String cmd) {
  cmd.trim();
  cmd.toUpperCase();
  if (cmd == "") return;

  Serial.print("Gui sang STM32: ");
  Serial.println(cmd);

  for (int i = 0; i < 3; i++) {
    Serial2.print(cmd);
    Serial2.print("\r\n");
    Serial2.flush();
    delay(50);
  }

  Blynk.virtualWrite(V1, "SENT");
  Blynk.virtualWrite(V2, cmd);
  activeService = cmd;
  updateServicePins(cmd);

  if (cmd == "STOP") {
    Blynk.virtualWrite(V1, "STOPPED");
    resetBlynkOutput();
    lastCommand = "";
  }
}

void handleCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();

  if (cmd != "WASH" && cmd != "DRY" && cmd != "WASHDRY" && cmd != "STOP") {
    Serial.print("Lenh khong hop le: ");
    Serial.println(cmd);
    return;
  }

  lastCommand = cmd;
  sendToSTM32(cmd);
}

BLYNK_WRITE(V0) {
  String cmd = param.asStr();
  Serial.print("Nhan lenh V0: ");
  Serial.println(cmd);
  handleCommand(cmd);
}

BLYNK_WRITE(V7) {
  if (param.asInt() == 1) {
    handleCommand("WASH");
    Blynk.virtualWrite(V7, 0);
  }
}

BLYNK_WRITE(V8) {
  if (param.asInt() == 1) {
    handleCommand("DRY");
    Blynk.virtualWrite(V8, 0);
  }
}

BLYNK_WRITE(V9) {
  if (param.asInt() == 1) {
    handleCommand("WASHDRY");
    Blynk.virtualWrite(V9, 0);
  }
}

BLYNK_WRITE(V10) {
  if (param.asInt() == 1) {
    handleCommand("STOP");
    lastCommand = "";
    Blynk.virtualWrite(V0, "");
    Blynk.virtualWrite(V10, 0);
  }
}

void handleSTM32Line(String response) {
  response.trim();
  if (response.length() == 0) return;

  Serial.print("STM32 tra ve: ");
  Serial.println(response);

  if (response.endsWith("READY")) {
    String service = extractServiceFromResponse(response);
    Blynk.virtualWrite(V1, "READY");
    Blynk.virtualWrite(V2, service);
    activeService = service;
    updateServicePins(service);
    lastCommand = "";
    // Máy chưa BUSY, không khởi động countdown
  }

  else if (response == "BUSY" || response.endsWith("BUSY")) {
    machineBusy = true;
    Blynk.virtualWrite(V1, "BUSY");
    if (activeService != "") {
      Blynk.virtualWrite(V2, activeService);
      updateServicePins(activeService);
    }
  }

  else if (response == "FINISHED" || response.endsWith("FINISHED")) {
    stopCountdown();
    Blynk.virtualWrite(V1, "FINISHED");
    lastCommand = "";
  }

  else if (response == "FREE" || response.endsWith("FREE")) {
    stopCountdown();
    Blynk.virtualWrite(V1, "FREE");
    resetBlynkOutput();
    lastCommand = "";
    Blynk.virtualWrite(V0, "");
  }

  else if (response == "ERROR" || response.endsWith("ERROR")) {
    stopCountdown();
    Blynk.virtualWrite(V1, "ERROR");
    Blynk.virtualWrite(V4, 0);
    Blynk.virtualWrite(V5, 0);
  }

  else if (response.startsWith("TIME:")) {
    int newTime = response.substring(5).toInt();

    // Đồng bộ countdown với giá trị STM32 gửi về (mỗi ~1 giây)
    // Nếu lệch ≤ 1 giây thì giữ nguyên local counter để tránh giật
    int diff = localCountdown - newTime;
    if (diff < -1 || diff > 1) {
      // Lệch nhiều → đồng bộ ngay
      localCountdown    = newTime;
      lastCountdownMs   = millis();
      Blynk.virtualWrite(V3, localCountdown);
    }
    // Nếu lệch trong khoảng [-1, 1] → local đang đúng, không ghi đè
    // tránh giật số trên Blynk

    if (!machineBusy) {
      machineBusy = true;
      lastCountdownMs = millis();
    }

    Blynk.virtualWrite(V1, "BUSY");

    if (activeService != "") {
      Blynk.virtualWrite(V2, activeService);
      updateServicePins(activeService);
    }
  }
}

void checkSTM32Response() {
  while (Serial2.available()) {
    char c = Serial2.read();

    if (c == '\n' || c == '\r') {
      if (stm32Buffer.length() > 0) {
        handleSTM32Line(stm32Buffer);
        stm32Buffer = "";
      }
    } else {
      stm32Buffer += c;
      if (stm32Buffer.length() > 40) {
        stm32Buffer = "";
      }
    }
  }
}

// Chạy mỗi 1000ms: ESP32 tự đếm ngược và cập nhật V3
// → Blynk luôn mượt dù UART có delay hay không
void localCountdownTick() {
  if (!machineBusy) return;

  if (localCountdown > 0) {
    localCountdown--;
    Blynk.virtualWrite(V3, localCountdown);
    Serial.print("Local countdown: ");
    Serial.println(localCountdown);
  }

  // Nếu đếm về 0 mà STM32 chưa gửi FREE → giữ nguyên, đợi STM32 xác nhận
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);

  delay(1000);

  Serial.println("ESP32 Laundry IoT starting...");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  Blynk.virtualWrite(V0, "");
  Blynk.virtualWrite(V1, "FREE");
  resetBlynkOutput();
  lastCommand = "";

  // Local countdown timer: cập nhật V3 mỗi 1 giây từ ESP32 nội bộ
  timer.setInterval(1000L, localCountdownTick);

  Serial.println("ESP32 da ket noi Blynk");
}

void loop() {
  Blynk.run();
  checkSTM32Response();
  timer.run();
}
