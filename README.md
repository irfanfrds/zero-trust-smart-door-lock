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

### 📸 System Screenshots

#### 1. Web Dashboard Interface
**Live Access Logs & Intrusion Detection**
<img width="675" height="397" alt="front" src="https://github.com/user-attachments/assets/795f74d9-c5ce-4bcf-9d4d-0891cd11a75c" />
<img width="820" height="555" alt="code dash" src="https://github.com/user-attachments/assets/f53b4c36-f25d-49c5-af26-d5abff7dc44d" />
<img width="822" height="522" alt="code dash 2" src="https://github.com/user-attachments/assets/2cf22cb2-06ad-4278-9b1e-fb669e039fa5" />

**Secure Sign-In Page**
<img width="504" height="418" alt="pass" src="https://github.com/user-attachments/assets/f2bb1b33-a9b5-4240-a7ab-73ca8d896f4a" />

**User Registration (Pairing RFID & Digital Token)**
<img width="575" height="406" alt="register" src="https://github.com/user-attachments/assets/a24db62a-8aaf-493c-9d73-49fd0beb436d" />
<img width="824" height="549" alt="code reg" src="https://github.com/user-attachments/assets/dfe34dd8-c3a8-4b36-b45c-c12c6ff78fc2" />
<img width="826" height="548" alt="code reg 2" src="https://github.com/user-attachments/assets/3a56fff8-730c-4d15-8a9d-2dd9a82e89e6" />

**Credential Management (Active / Lost Mode)**
<img width="678" height="196" alt="lost" src="https://github.com/user-attachments/assets/50784951-53f4-4420-acfb-5d5479cd5ad4" />

**2. Real-Time Telegram 2FA Alerts**
*Access Granted & Denied Logs:*
<img width="277" height="475" alt="tele" src="https://github.com/user-attachments/assets/4c08f6e7-1688-408f-995a-53b5b9ff0f6b" />

*Security Lockdown (Brute Force Attack Detected):*
<img width="315" height="370" alt="brute" src="https://github.com/user-attachments/assets/51d68fb4-fbc3-4340-a6c3-982dc7f4f0ce" />

**3. Backend Server Status**
*Node.js mTLS Server & MySQL Connection:*
<img width="675" height="142" alt="server" src="https://github.com/user-attachments/assets/a19917dd-b352-447b-830f-7fea7b0ffee3" />
<img width="808" height="552" alt="code node" src="https://github.com/user-attachments/assets/465e8004-2df8-4551-aa9a-615e019df0fb" />
<img width="808" height="532" alt="code node 2" src="https://github.com/user-attachments/assets/14282de7-6ce1-4268-b8c8-49bf1834255b" />
<img width="883" height="454" alt="sql" src="https://github.com/user-attachments/assets/0d489915-2c47-466c-9972-8986ae30b63d" />
<img width="883" height="424" alt="sql 2" src="https://github.com/user-attachments/assets/485b6c42-8f49-432b-88e7-4ffe5b491a68" />





