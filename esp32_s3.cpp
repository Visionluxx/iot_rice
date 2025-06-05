#include <HardwareSerial.h>

// -------------------- Cấu hình chân --------------------
#define TRIG_PIN 10           // Chân trig cảm biến siêu âm
#define ECHO_PIN 9            // Chân echo cảm biến siêu âm
#define TDS_PIN 14            // Chân analog cho cảm biến TDS

// -------------------- UART A7680C --------------------
HardwareSerial sim(2);        // UART2 cho A7680C
#define A7680C_TX 19
#define A7680C_RX 20

// -------------------- Thời gian gửi dữ liệu --------------------
unsigned long lastSend = 0;
const unsigned long interval = 1800000; // 30 phút (30*60*1000 ms)

// -------------------- Hàm đọc mực nước bằng HC-SR04 --------------------
float readWaterLevel() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // timeout 30ms
  float distance_cm = duration * 0.034 / 2.0;      // Tốc độ âm: 0.034 cm/us
  return distance_cm;
}

// -------------------- Hàm đọc giá trị TDS (DFRobot SEN0244) --------------------
float readTDS() {
  int analogValue = analogRead(TDS_PIN);
  float voltage = analogValue * 3.3 / 4095.0;   // ESP32-S3 ADC: 12-bit (0–4095)
  float tdsValue = (133.42 * voltage * voltage * voltage
                    - 255.86 * voltage * voltage
                    + 857.39 * voltage) * 0.5;  // Công thức từ DFRobot
  return tdsValue;
}

// -------------------- Gửi AT command tới A7680C --------------------
void sendAT(String cmd, int waitTime = 1000) {
  sim.println(cmd);
  unsigned long timeout = millis() + waitTime;
  while (millis() < timeout) {
    while (sim.available()) {
      Serial.write(sim.read());
    }
  }
}

// -------------------- Hàm gửi dữ liệu lên server --------------------
void sendDataToServer(float distance, float tds) {
  sendAT("AT");
  sendAT("AT+CGATT=1");
  sendAT("AT+CGDCONT=1,\"IP\",\"internet\"");
  sendAT("AT+NETOPEN", 3000);
  sendAT("AT+IPADDR");
  sendAT("AT+HTTPINIT");
  sendAT("AT+HTTPPARA=\"CID\",1");
  sendAT("AT+HTTPPARA=\"URL\",\"http://yourserver.com/api/data\"");
  sendAT("AT+HTTPPARA=\"CONTENT\",\"application/json\"");
  sendAT("AT+HTTPDATA=128,5000");
  delay(100);

  // Tạo chuỗi JSON
  String json = "{\"water_level_cm\":";
  json += String(distance, 2);
  json += ",\"tds_ppm\":";
  json += String(tds, 2);
  json += "}";

  sim.println(json); // Gửi nội dung
  delay(1000);

  sendAT("AT+HTTPACTION=1", 6000); // POST
  sendAT("AT+HTTPTERM");
  sendAT("AT+NETCLOSE");
}

// -------------------- Setup --------------------
void setup() {
  Serial.begin(115200);

  // Khởi tạo UART2 cho A7680C
  sim.begin(115200, SERIAL_8N1, A7680C_RX, A7680C_TX);

  // Khởi tạo cảm biến siêu âm
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(TDS_PIN, INPUT);

  delay(2000); // Đợi module ổn định
}

// -------------------- Loop chính --------------------
void loop() {
  if (millis() - lastSend > interval) {
    lastSend = millis();

    // Đọc cảm biến
    float level = readWaterLevel();
    float tds = readTDS();

    Serial.println("Đang gửi dữ liệu...");
    sendDataToServer(level, tds);
    Serial.println("Đã gửi xong.");
  }

  delay(1000); // Lặp kiểm tra mỗi giây
}
