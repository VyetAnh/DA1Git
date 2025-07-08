#include <WiFi.h>
#include <FirebaseESP32.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define WIFI_SSID "Thanh"
#define WIFI_PASSWORD "244466666"
#define FIREBASE_HOST "tuan-b1512-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "8k8dZMkpjWnTEEma8NosvVFRuVopDgoUNILZst1O"

#define UART_RX 16
#define UART_TX 17
HardwareSerial SerialPort(2);

FirebaseData fbdo;
FirebaseConfig config;
FirebaseAuth auth;

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7 * 3600, 60000); // GMT+7

String formatTimestamp(time_t t) {
  struct tm *tmStruct = localtime(&t);
  char buf[25];
  sprintf(buf, "%04d-%02d-%02d %02d:%02d", 
    tmStruct->tm_year + 1900, tmStruct->tm_mon + 1,
    tmStruct->tm_mday, tmStruct->tm_hour, tmStruct->tm_min);
  return String(buf);
}

void setup() {
  Serial.begin(115200);
  SerialPort.begin(115200, SERIAL_8N1, UART_RX, UART_TX);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWiFi connected");

  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  timeClient.begin();
  while (!timeClient.update()) timeClient.forceUpdate();

  Serial.println("Setup complete");
}

void loop() {
  static unsigned long lastSendRealtime = 0;
  static String line = "";

  while (SerialPort.available()) {
    char c = SerialPort.read();
    if (c == '\n') {
      float rain; int dist;
      if (sscanf(line.c_str(), "rain:%f,dist:%d", &rain, &dist) == 2) {
        timeClient.update();
        time_t now = timeClient.getEpochTime();

        // Gửi dữ liệu realtime mỗi 2 giây
        if (millis() - lastSendRealtime > 2000) {
          FirebaseJson json;
          json.set("rain", rain);
          json.set("distance", dist);
          json.set("timestamp", now);
          
          if (Firebase.setJSON(fbdo, "/realtime", json)) {
            Serial.println("[Realtime] Đã gửi thành công:");
            Serial.printf("  Rain: %.2f mm, Distance: %d cm\n", rain, dist);
          } else {
            Serial.print("[Realtime] Lỗi gửi dữ liệu: ");
            Serial.println(fbdo.errorReason());
          }

          lastSendRealtime = millis();
        }
      } else {
        Serial.print("[ERROR] Không phân tích được dòng UART: ");
        Serial.println(line);
      }
      line = "";
    } else {
      line += c;
    }
  }
}

