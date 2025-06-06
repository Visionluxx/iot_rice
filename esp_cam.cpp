#include "esp_camera.h"
#include "base64.h"

// UART dùng giao tiếp A7680C (TX0/RX0)
#define MODEM_BAUD 115200

// Gắn vào các chân camera của ESP32-CAM
#define PWDN_GPIO_NUM    -1
#define RESET_GPIO_NUM   -1
#define XCLK_GPIO_NUM     0
#define SIOD_GPIO_NUM    26
#define SIOC_GPIO_NUM    27

#define Y9_GPIO_NUM      35
#define Y8_GPIO_NUM      34
#define Y7_GPIO_NUM      39
#define Y6_GPIO_NUM      36
#define Y5_GPIO_NUM      21
#define Y4_GPIO_NUM      19
#define Y3_GPIO_NUM      18
#define Y2_GPIO_NUM       5
#define VSYNC_GPIO_NUM   25
#define HREF_GPIO_NUM    23
#define PCLK_GPIO_NUM    22

unsigned long lastSentTime = 0;
const unsigned long interval = 30UL * 60UL * 1000UL; // 30 phút

String server = "http://<ip-server>:5000/api/image"; // Thay bằng IP hoặc domain server bạn

// ---------- Hàm khởi tạo camera ----------
void initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;  // 320x240
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.println("Camera init failed!");
    while (1);
  }
}

// ---------- Hàm gửi lệnh AT và chờ phản hồi ----------
bool sendAT(String command, String expected, unsigned long timeout = 2000) {
  Serial1.println(command);
  unsigned long t = millis();
  String response = "";

  while (millis() - t < timeout) {
    while (Serial1.available()) {
      char c = Serial1.read();
      response += c;
    }
    if (response.indexOf(expected) != -1) {
      return true;
    }
  }
  Serial.println("AT ERROR: " + response);
  return false;
}

// ---------- Hàm gửi ảnh lên server ----------
void sendImage() {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Chụp ảnh thất bại");
    return;
  }

  // Mã hóa ảnh thành base64
  String base64Img = base64::encode(fb->buf, fb->len);
  esp_camera_fb_return(fb);

  // Tạo JSON gửi đi
  String payload = "{\"image\":\"" + base64Img + "\"}";

  // Bắt đầu gửi qua HTTP bằng AT
  sendAT("AT+SAPBR=3,1,\"Contype\",\"GPRS\"", "OK");
  sendAT("AT+SAPBR=3,1,\"APN\",\"internet\"", "OK");  // Thay "internet" nếu nhà mạng yêu cầu
  sendAT("AT+SAPBR=1,1", "OK", 10000); // Mở kết nối GPRS
  sendAT("AT+HTTPINIT", "OK");
  sendAT("AT+HTTPPARA=\"CID\",1", "OK");

  // Gửi tới API ảnh
  sendAT("AT+HTTPPARA=\"URL\",\"" + server + "\"", "OK");
  sendAT("AT+HTTPPARA=\"CONTENT\",\"application/json\"", "OK");

  sendAT("AT+HTTPDATA=" + String(payload.length()) + ",10000", "DOWNLOAD");
  delay(100);
  Serial1.print(payload);  // Gửi dữ liệu JSON
  delay(1000);

  sendAT("AT+HTTPACTION=1", "200", 10000);  // Gửi POST
  sendAT("AT+HTTPTERM", "OK");
  sendAT("AT+SAPBR=0,1", "OK");  // Ngắt kết nối GPRS

  Serial.println("Đã gửi ảnh xong.");
}

// ---------- Thiết lập ----------
void setup() {
  Serial.begin(115200);      // Debug
  Serial1.begin(MODEM_BAUD); // UART1 cho A7680C

  initCamera();
  delay(3000);
}

// ---------- Loop chính ----------
void loop() {
  unsigned long now = millis();
  if (now - lastSentTime > interval || lastSentTime == 0) {
    Serial.println("Bắt đầu gửi ảnh...");
    sendImage();
    lastSentTime = now;
  }

  delay(1000); // Chờ 1 giây tránh treo máy
}

