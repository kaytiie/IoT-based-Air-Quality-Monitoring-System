# 🏠 Indoor Air Quality Monitoring System

## 📘 Introduction
This project aims to design and implement an **Indoor Air Quality Monitoring System** using an **ESP32** microcontroller.  
The system continuously measures key environmental parameters such as **temperature, humidity, gas concentration, and dust density** to evaluate air quality.  

All collected data are sent to a **cloud platform (ThingsBoard or ThingSpeak)** for real-time visualization and analysis.  
This helps users monitor indoor air conditions and maintain a **healthy living environment**.

---

## 🔧 Hardware Components
| Component | Function |
|------------|-----------|
| **ESP32** | Main microcontroller that reads sensor data and connects to Wi-Fi |
| **MQ-135 Gas Sensor** | Detects harmful gases like CO₂, NH₃, alcohol, and benzene |
| **DHT11 Sensor** | Measures temperature and humidity |
| **GP2Y1010 Dust Sensor** | Detects particulate matter (PM) and dust density |
| **OLED Display** | Displays real-time sensor data |
| **Buzzer** | Triggers an alert when air quality exceeds safe limits |
| **LED Indicators** | Shows air quality level (good, moderate, poor) |

---

## ⚙️ System Features
- Real-time monitoring of air quality parameters.  
- Cloud connectivity for data logging and visualization.  
- Local OLED display for instant updates.  
- Alerts via buzzer and LEDs when thresholds are exceeded.  
- Expandable design for additional sensors or smart home integration.

---

## 🌐 Cloud Dashboard
The ESP32 sends data to **ThingsBoard** or **ThingSpeak**, where users can:
- View **real-time graphs** of temperature, humidity, gas, and dust levels.  
- Analyze **trends** over time.  
- Set **alert thresholds** for specific parameters.

---

## 📊 Results
- The system successfully measured and transmitted sensor data to the cloud.  
- Alerts were triggered when gas concentration or temperature exceeded set limits.  
- Air quality changes were observed during activities like cooking or cleaning.

| Parameter | Observation |
|------------|--------------|
| Temperature | Stable with minor fluctuations |
| Humidity | Within normal indoor range |
| Air Quality | Spikes detected during specific household activities |

---

## 🧩 Conclusion
This project demonstrates the effective use of **ESP32** and multiple sensors to monitor indoor air quality in real time.  
It provides users with valuable insights and alerts to maintain a healthier environment.  

### 🔮 Future Improvements
- Add more precise CO₂ and VOC sensors.  
- Integrate with home automation systems for automatic ventilation control.  
- Include historical data logging and trend prediction using AI or cloud analytics.  

---


## 🛠️ Tools & Technologies
- **ESP32**  
- **Arduino IDE / PlatformIO**  
- **ThingsBoard / ThingSpeak Cloud**  
- **C / C++ (Arduino Framework)**  

