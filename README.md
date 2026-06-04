# Automatic Fish Feeder (TA-Inkon)

A smart, IoT-based automatic fish feeder system using an ESP32 microcontroller, a servo motor, and a Node.js web dashboard. It allows you to feed your fish remotely via a web interface and features automatic scheduled feedings.

## Features
- **Remote Feeding:** Manually trigger the feeder using a web dashboard.
- **Customizable Angles:** Set different servo angles for different food portions.
- **Scheduled Feeding:** Automatically feeds at 09:00, 13:00, and 20:00 (WIB).
- **Activity Log:** Keeps track of the last 20 feeding events.
- **Online Monitoring:** Expose the local server securely using Ngrok.
- **Hardware Status:** ESP32 online/offline status monitoring on the dashboard.
- **LCD Display:** Real-time status and time display on a 16x2 I2C LCD.

## Hardware Requirements
- ESP32 Development Board
- Servo Motor (connected to Pin 13)
- 16x2 LCD with I2C module (Address `0x27`)
- Jumper wires & power supply

## Software Requirements
- [Node.js](https://nodejs.org/) (for the local server)
- [Arduino IDE](https://www.arduino.cc/en/software) (for programming the ESP32)
- [Ngrok](https://ngrok.com/) (for exposing the server to the internet)

### Arduino Libraries Required
Make sure to install these via the Arduino Library Manager:
- `ESP32Servo` by Kevin Harrington, John K. Bennett
- `LiquidCrystal I2C` by Frank de Brabander

## Setup & Deployment

### 1. Server Setup (Laptop/PC)
1. Clone or download this repository to your local machine.
2. Ensure you have Node.js installed. (No `npm install` needed as it uses vanilla Node.js modules).
3. Open a terminal and navigate to the project directory:
   ```bash
   cd /Users/oryzavdio/Desktop/coding/ta-inkon
   ```
4. Run the local server:
   ```bash
   node server.js
   ```
5. The server will start on `http://localhost:3000`.

### 2. Exposing the Server to the Internet
To allow the ESP32 and external devices (like your phone) to access the dashboard, you need to expose your local server using Ngrok.
1. In a new terminal, run:
   ```bash
   npx ngrok http 3000
   ```
2. Copy the **Forwarding URL** provided by Ngrok (e.g., `https://capsule-abstain-fanfare.ngrok-free.dev`).

### 3. ESP32 Setup
1. Open `fish_feeder_esp32/fish_feeder_esp32.ino` in Arduino IDE.
2. Update the WiFi credentials:
   ```cpp
   const char* ssid     = "YOUR_WIFI_SSID";
   const char* password = "YOUR_WIFI_PASSWORD";
   ```
3. Update the `serverIP` with your active Ngrok Forwarding URL:
   ```cpp
   const char* serverIP = "https://your-ngrok-url.ngrok-free.dev";
   ```
4. Connect your ESP32 to your computer via USB.
5. Select your ESP32 board and COM port in Arduino IDE.
6. Click **Upload** to flash the code to the ESP32.

### 4. Usage
- Open your browser and go to `http://localhost:3000` (or your Ngrok URL if accessing remotely).
- You will see the dashboard with the status of the ESP32.
- Click on the feeding buttons (30°, 45°, 60°, 90°) to manually feed the fish.
- The LCD on the ESP32 will display the current time, connection status, and feeding progress.
