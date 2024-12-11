# NodeMCU Dual Relay Timer Controller

Added option to select NO and NC mode for the relay.

Added option to set a name for the relay.

A web-based dual relay controller using NodeMCU (ESP8266) with automatic scheduling and persistent storage.

## Features

- Web-based interface with auto-updating clock (IST)
- Dual relay control with independent schedules
- Persistent storage of settings using EEPROM
- Automatic scheduling with enable/disable option
- Manual override controls
- Midnight auto-reset option
- WiFi reconnection handling
- Mobile-responsive design

## Hardware Requirements

- NodeMCU ESP8266
- 2-Channel Relay Module
- Power Supply (5V)
- Connecting wires

## Pin Configuration

- NodeMCU -> Relay Module
- D3 -> Relay 1 Input
- D4 -> Relay 2 Input
- 3.3V -> VCC
- GND -> GND


## Software Requirements

### Required Libraries
- ESP8266WiFi
- ESP8266WebServer
- EEPROM
- TimeLib
- NTPClient
- WiFiUdp

## Installation

1. Install Arduino IDE
2. Add ESP8266 board support
3. Install required libraries
4. Update WiFi credentials in the code
5. Upload to NodeMCU

## Usage

1. Power up the NodeMCU
2. Connect to WiFi network
3. Access web interface via NodeMCU's IP address
4. Set schedules for each relay
5. Enable/disable automatic scheduling as needed

## Web Interface Features

- Real-time clock display (IST)
- Independent ON/OFF time settings for each relay
- Manual override buttons
- Schedule enable/disable control
- Current status display
- Persistent settings storage

## Functions

### Main Functions
- Time synchronization with NTP server
- EEPROM data persistence
- Automatic schedule execution
- WiFi connection management
- Web server handling

### Control Options
- Manual ON/OFF control
- Scheduled operation
- Schedule enable/disable
- Midnight auto-reset

