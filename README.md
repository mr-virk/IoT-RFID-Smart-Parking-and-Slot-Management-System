# IoT RFID Smart Parking & Slot Management System

---

## Abstract
 <br />
This project implements a secure, automated smart parking solution using ESP8266, RFID cards, servo gates, LCD display, Blynk IoT connectivity, and IR sensor-based slot detection. The system manages up to four slots and uses EEPROM to persistently store authorized cards for entry. Each vehicle scans its RFID card at the gate; only permitted cards allow the servo-controlled barrier to open, with audit logs and access events recorded in the Blynk cloud. IR sensors detect real-time occupancy and update available slots on both the local LCD and the remote app. Administrators can add or delete authorized cards via Blynk, monitor uptime and slot status, and perform manual overrides if needed. The modular architecture supports easy expansion to more slots and additional sensors, making it scalable for deployments in residential complexes, public parking lots, or institutional campuses. This system enhances parking security, increases automation, lowers labor dependency, and enables detailed data analytics for facility managers.

---

## Hardware Components
 <br />
- ESP8266 (WiFi main controller)
- MFRC522 RFID Reader
- PCF8574 I/O Expander (I2C)
- IR Sensors (x4, for slot detection)
- Servo Motor (gate control)
- 20x4 I2C LCD Display
- LEDs & wiring/breadboard
- Power supply

---

## Software Structure
 <br />
- `main.ino`: RFID, slot management, servo, LCD, Blynk logic
- Built-in support for: BlynkSimpleEsp8266, MFRC522, PCF8574, LiquidCrystal_I2C

---

## Key Features
 <br />
- RFID authentication for entry/exit
- Real-time IR-based slot status updating
- Servo gate automation
- LCD and cloud-based feedback
- Card management (add/delete via serial/Blynk)
- EEPROM for persistent card storage

---

## UML Class Diagram
 <br />
```mermaid
classDiagram
class ESP8266 {
+setupWiFi()
+setupBlynk()
+run()
+checkRFID()
+servoControl()
+updateLCD()
+updateSlotStatus()
}
class RFID_Module {
+scanCard()
+authenticate()
+addCard()
+deleteCard()
}
class Servo {
+openGate()
+closeGate()
}
class IR_Sensors {
+readSlotStatus()
}
class PCF8574 {
+readInputs()
}
class LCD_Display {
+showStatus()
+showAdminMenu()
}
class Blynk_App {
+syncVirtualPins()
+displayUptime()
+logEvent()
}
ESP8266 "1" --> "1" RFID_Module : Uses
ESP8266 "1" --> "1" Servo : Controls
ESP8266 "1" --> "1" IR_Sensors : Reads
ESP8266 "1" --> "1" LCD_Display : Updates
ESP8266 "1" --> "1" Blynk_App : Cloud Sync
IR_Sensors --> PCF8574 : via I2C
```

---

## Flowchart
 <br />
```mermaid
flowchart TD
A[System Startup] --> B{WiFi & Device Init}
B --> C[LCD/EEPROM/PCF8574 Init]
C --> D[Wait for Card or Exit Sensor]
D -->|Card Detected| E{Is Card Stored?}
E -->|Yes| F[Access Granted: Open Gate, Log Event, Update Slot Status]
E -->|No| G[Access Denied: Show LCD Status]
D -->|Exit Detected| H[Open Gate, Update Slot Status]
F --> C
G --> C
H --> C
```

---

## State Diagram
 <br />
```mermaid
stateDiagram-v2
[*] --> Init
Init --> WiFi_Connect
WiFi_Connect --> Idle
Idle --> Card_Scan : RFID detected
Card_Scan --> Auth : Authenticate Card
Auth --> Open_Gate : If Authorized
Auth --> Deny : If Not Authorized
Open_Gate --> Update_Slots
Deny --> Idle
Update_Slots --> Idle
Idle --> Exit_Scan : Exit Sensor Triggered
Exit_Scan --> Open_Exit_Gate
Open_Gate --> Update_Slots
```

---

## File Descriptions
 <br />
| File/Folder     | Content/Function                                |
|-----------------|-------------------------------------------------|
| main.ino        | Main firmware code                              |
| Schematic      | Schematic               |

---

## License
 <br />
MIT License

---
