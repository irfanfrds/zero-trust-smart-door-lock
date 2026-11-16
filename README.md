# 🚪 Zero Trust Smart Door Lock Access System

An enterprise-grade IoT access control system built to demonstrate **Zero Trust Architecture**. This project eliminates the vulnerabilities of traditional single-factor RFID doors by enforcing linked dual-authentication, Mutual TLS (mTLS) network encryption, and active intrusion detection. 

## 🛡️ Core Security Features

* **Zero Trust Authentication:** Tapping a registered RFID card does not unlock the door. The system pushes a real-time 2FA prompt to the user's linked Telegram smartphone app. Access is only granted when the physical card and digital token perfectly match the database pairing record.
* **Mutual TLS (mTLS) Encryption:** The ESP32 hardware and the Node.js backend mathematically verify each other's cryptographic certificates before establishing a connection. Untrusted devices on the local Wi-Fi are blocked at the transport layer.
* **Active Intrusion Detection (3-Strike Rule):** If the hardware detects 3 consecutive unregistered RFID swipes, it triggers a 60-second mechanical lockdown, sounds a physical siren, and pushes a high-priority "Brute Force Attack" alert to the administrator's Telegram.
* **Administrative "Lost Mode":** A cinematic, dark-mode web dashboard allows administrators to instantly suspend compromised credentials. If a stolen card is swiped, the server drops the request and logs the unauthorized attempt.
* **Dashboard Inactivity Manager:** The administrative interface features a strict session manager that forces a logout after 2 minutes of idle time or if the browser tab is minimized, preventing unauthorized physical access to the logs.

## 🛠️ Technology Stack

* **Hardware:** ESP32 Microcontroller, MFRC522 RFID Reader (13.56 MHz), SG90 Servo Motor, 16x2 I2C LCD, 5V High-Capacity Powerbank, Custom PVC Enclosure.
* **Backend:** Node.js, Express.js.
* **Database:** MySQL (Relational schema for Users, RFID Cards, Digital Tokens, and Access Logs).
* **Frontend:** Custom HTML/CSS/JS (HTTP Basic Authentication, live DOM updates via async fetch).
* **External API:** Telegram Bot API (for digital token prompts and emergency security alerts).

## 🔒 Security Notice for this Repository
*As a cybersecurity best practice, all cryptographic private keys (`.key`), CA certificates (`.crt`), environmental variables (`.env`), and live API tokens have been strictly excluded from this public repository. The uploaded source code contains sanitized placeholder variables for portfolio demonstration purposes.*
