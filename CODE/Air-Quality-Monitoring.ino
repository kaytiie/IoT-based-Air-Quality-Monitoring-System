#include "DHT.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ThingsBoard.h>
#include <Arduino_MQTT_Client.h>
#include <WiFi.h>
#include <ArduinoJson.h>

#define DHTPIN 15
#define DHTTYPE DHT11
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define MQ135_PIN 36

#define LED_PIN 13
#define BUZZER_PIN 19
#define TEMP_THRESHOLD 35.0
#define HUMIDITY_THRESHOLD 45.0
#define AIR_QUALITY_THRESHOLD 400
#define DUST_THRESHOLD 12
#define BUTTON_PIN 23  // Nút nhấn kết nối với GPIO 23

#define WIFI_AP "Lobby"
#define WIFI_PASS "lobby15082002$"
#define TB_SERVER "thingsboard.cloud"
#define TOKEN "PcdDL9FFH4TPbUmLbsYG"

int measurePin = 25;  // Chân đo cho ESP32
int ledPower = 26;    // Chân nguồn cho LED

int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;

float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;

DHT dht(DHTPIN, DHTTYPE);
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
WiFiClient espClient;
Arduino_MQTT_Client mqttClient(espClient);
ThingsBoard tb(mqttClient, 256);

float temperature = 0.0;
float humidity = 0.0;
float air_quality_ppm = 0.0;
float dust_density = 0.0;

unsigned long warningStopTime = 0;
bool warningActive = false;
bool thresholdCheckActive = true;  // Biến điều khiển việc kiểm tra ngưỡng
bool buttonState = HIGH;
bool ledState = 0;
bool buzzerState = 0;
bool buttonTbState = 1;
bool autoStopWarningState = 1;
bool subscribed = false; // Indicates if RPC subscription is done

RPC_Response processSetButtonStatus(const RPC_Data &data);
// Define the array of RPC callbacks
const std::array<RPC_Callback, 1U> callbacks = {
  RPC_Callback{ "buttonTbState", processSetButtonStatus } //tên của RPC và hàm callback xử lý dữ liệu cho RPC đó.
};

void connectToWiFi()
{
  Serial.println("Connecting to WiFi...");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20)
  {
    WiFi.begin(WIFI_AP, WIFI_PASS);
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nConnected to WiFi");
  }
  else
  {
    Serial.println("\nFailed to connect to WiFi.");
  }
}

void connectToThingsBoard()
{
  if (!tb.connected())
  {
    Serial.println("Connecting to ThingsBoard server");
    if (tb.connect(TB_SERVER, TOKEN))
    {
      Serial.println("Connected to ThingsBoard");
    }
    else
    {
      Serial.println("Failed to connect to ThingsBoard");
    }
  }

  // Subscribe to RPC callbacks
  Serial.println("Subscribing for RPC...");
  if (!tb.RPC_Subscribe(callbacks.cbegin(), callbacks.cend())) {
    Serial.println("Failed to subscribe for RPC");
    return;
  }
}

void sendDataToThingsBoard(float temp, float hum, float air_quality, float dust, bool buttonState, bool ledState, bool buzzerState)
{
  String jsonData = "{\"temperature\":" + String(temp, 2) +
                    ", \"humidity\":" + String(hum, 2) +
                    ", \"air_quality\":" + String(air_quality, 2) +
                    ", \"dustDensity\":" + String(dust, 2) +
                    ", \"buttonState\":" + String(buttonState) +
                    ", \"ledState\":" + String(ledState) +
                    ", \"buzzerState\":" + String(buzzerState) + "}";

  tb.sendTelemetryJson(jsonData.c_str());
  Serial.println("Data sent to ThingsBoard");
}

void readSensors()
{
  // Đọc cảm biến DHT
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();
  if (!isnan(temp) && !isnan(hum))
  {
    temperature = temp;
    humidity = hum;
    Serial.print("DHT Data: ");
    Serial.print("Temp: ");
    Serial.print(temperature);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");
  }
  else
  {
    Serial.println("Failed to read from DHT sensor!");
  }

  // Đọc cảm biến MQ-135
  int sensorValue = analogRead(MQ135_PIN);
  air_quality_ppm = map(sensorValue, 0, 4095, 0, 1000);
  Serial.print("MQ-135 PPM: ");
  Serial.println(air_quality_ppm);

  // Đọc cảm biến Dust
  digitalWrite(ledPower, LOW); // Bật IR LED
  delayMicroseconds(samplingTime); // Delay 0.28ms
  
  voMeasured = analogRead(measurePin); // Đọc giá trị ADC V0
  delayMicroseconds(deltaTime); // Delay 0.04ms
  digitalWrite(ledPower, HIGH); // Tắt LED
  delayMicroseconds(sleepTime); // Delay 9.68ms

  // Tính điện áp từ giá trị ADC
  calcVoltage = voMeasured * (3.3 / 4095.0); // Chuyển đổi giá trị ADC sang điện áp cho ESP32
  
  // Tính mật độ bụi dựa trên điện áp đo được
  if (calcVoltage < 0.1) {
    dustDensity = 0; // Loại bỏ nhiễu khi điện áp thấp
  } else {
    dustDensity = 0.17 * calcVoltage - 0.1; // Áp dụng phương trình tính mật độ bụi
  }

  // In kết quả ra Serial Monitor
  Serial.print("Dust Density: ");
  Serial.print(dustDensity, 2);
  Serial.println(" mg/m³");
}

// Kiểm tra các giá trị ngưỡng
void checkThresholds(float temp, float hum, float air_quality, float dust) {
  if (temp > TEMP_THRESHOLD || hum < HUMIDITY_THRESHOLD || air_quality > AIR_QUALITY_THRESHOLD || dust > DUST_THRESHOLD) {
    if (!warningActive) {
      Serial.println("Warning: Threshold exceeded!");
      warningActive = true;
      autoStopWarningState = 0;
      thresholdCheckActive = false; // Tạm dừng kiểm tra ngưỡng
    }
    
  }
}

void autoStopWarning(float temp, float hum, float air_quality, float dust){
  if (temp < TEMP_THRESHOLD && hum > HUMIDITY_THRESHOLD && air_quality < AIR_QUALITY_THRESHOLD && dust < DUST_THRESHOLD) {
    autoStopWarningState = 1;
  }  
}

void processTime(const JsonVariantConst& data) {
  // Process the RPC response containing the current time
  Serial.print("Received time from ThingsBoard: ");
  Serial.println(data["time"].as<String>());
}

RPC_Response processSetButtonStatus(const RPC_Data &data) {
  // Process the RPC request to change the LED state
  int dataInt = data;
  buttonTbState = dataInt;  // Update the LED state based on the received data
  Serial.println("dataInt: ");
  Serial.print(dataInt);
  return RPC_Response("newStatus", dataInt);  // Respond with the new status
}

// Hàm xử lý nút bấm
void handleButtonPress() {
  buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW || buttonTbState == 0 || autoStopWarningState == 1){   
    Serial.println("Button pressed! Turning off warning.");

    // Tắt cảnh báo
    warningActive = false;
    digitalWrite(LED_PIN, LOW);    // Tắt đèn LED
    digitalWrite(BUZZER_PIN, HIGH); // Tắt còi
    ledState = 0;
    buzzerState = 0;
    // Tạm dừng kiểm tra ngưỡng
    warningStopTime = millis();  // Ghi lại thời gian tắt cảnh báo
    thresholdCheckActive = false;  // Dừng kiểm tra ngưỡng
    buttonTbState = 1;
  }
}

// Hàm nháy đèn và còi khi cảnh báo
void warning() {
  digitalWrite(BUZZER_PIN, LOW); // Bật còi
  digitalWrite(LED_PIN, HIGH);
  ledState = 1;
  buzzerState = 1;
  Serial.println("Warning!!!!!!!!");
  // Xử lý nút nhấn
  handleButtonPress();

}

void oledDisplay()
{
  display.clearDisplay();
  display.setCursor(0, 0);

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.print("Temp: ");
  display.print(temperature, 2);
  display.println(" C");

  display.setCursor(0, 16);
  display.print("Humidity: ");
  display.print(humidity, 2);
  display.println(" %");

  display.setCursor(0, 32);
  display.print("Air: ");
  display.print(air_quality_ppm, 2);
  display.println(" PPM");

  display.setCursor(0, 48);
  display.print("Dust: ");
  display.print(dust_density, 2);
  display.println(" mg/m3");

  display.display();
}

void setup()
{
  Serial.begin(115200);
  dht.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("OLED init failed");
    while (1)
      ;
  }
  display.clearDisplay();

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, HIGH);
  pinMode(ledPower, OUTPUT);

  // Cấu hình chân nút nhấn
  pinMode(BUTTON_PIN, INPUT_PULLUP); // Sử dụng điện trở kéo lên nội bộ

  connectToWiFi();
  connectToThingsBoard();
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
    connectToWiFi();
  if (!tb.connected())
    connectToThingsBoard();
  if (!subscribed) {
    Serial.println("Requesting RPC...");
    RPC_Request_Callback timeRequestCallback("getCurrentTime", processTime);  //Khi RPC trả về kết quả, hàm processTime sẽ được gọi để xử lý dữ liệu này.
    if (!tb.RPC_Request(timeRequestCallback)) {
      Serial.println("Failed to request for RPC");
      return;
    }
    Serial.println("Request done");
    subscribed = true;
  }
  tb.loop();
  
  // Đọc dữ liệu cảm biến
  readSensors();
  oledDisplay();
  sendDataToThingsBoard(temperature, humidity, air_quality_ppm, dust_density, buttonState, ledState, buzzerState);

  // Kiểm tra các ngưỡng
  if (thresholdCheckActive) {
    checkThresholds(temperature, humidity, air_quality_ppm, dust_density);
  } else {
    // Kiểm tra khoảng thời gian kích hoạt lại checkThresholds
    if ((millis() - warningStopTime) >= 60000) {
      thresholdCheckActive = true; 
    }
  }

  // Cảnh báo nếu cần
  if (warningActive) {
    warning();
  }

  autoStopWarning(temperature, humidity, air_quality_ppm, dust_density);

  delay(300); 
}
