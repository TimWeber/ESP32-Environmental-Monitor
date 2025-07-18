# ESP32 Environmental Monitoring System

An embedded environmental monitoring system built for the ESP32-C3. With the help of FreeRTOS, It reads temperature, humidity, and air quality data from sensors and sends it to a remote server over WiFi.

### Sensors
- **AHT21**: Temperature and humidity sensor with I2C communication
- **ENS160**: Air quality sensor that measures TVOC, eCO2, and AQI
- **I2C Manager**: Handles all the I2C communication in one place

### Networking
- **WiFi**: Connects to your network using credentials stored securely in NVS
- **HTTP Client**: Sends JSON data to your server with retry logic
- **SNTP**: Syncs time for accurate timestamps

### Configuration
- **JSON config**: All settings in a JSON file that gets embedded in the firmware
- **Secure storage**: WiFi passwords and server URLs stored encrypted
- **Calibration**: Configurable sensor calibration and validation

## Architecture


### Data Flow
```mermaid
graph TD
    A[AHT21 Sensor] -->|I2C| B[I2C Manager]
    C[ENS160 Sensor] -->|I2C| B
    B --> D[Sensor Task]
    D -->|Calibration| E[Calibration Manager]
    E --> F[Data Queue]
    F --> G[Network Task]
    G -->|HTTP POST| H[Remote Server]
    
    I[Configuration] -->|JSON| J[Config Manager]
    J --> K[RTOS Manager]
    K --> D
    K --> G
    
    L[WiFi Manager] -->|Credentials| G
    M[Secure Storage] -->|NVS| L
    
    N[Monitor Task] -->|Health Check| K
    N -->|Statistics| O[System Stats]
    
    P[SNTP Manager] -->|Time Sync| G
    
    style A fill:#e1f5fe
    style C fill:#e1f5fe
    style B fill:#f3e5f5
    style D fill:#fff3e0
    style G fill:#fff3e0
    style N fill:#fff3e0
    style H fill:#e8f5e8
    style J fill:#fce4ec
    style M fill:#fce4ec
```

### Task Structure
```
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────┐
│   Sensor Task   │    │  Network Task   │    │  Monitor Task   │
│                 │    │                 │    │                 │
│ • Read sensors  │    │ • HTTP client   │    │ • System health │
│ • Queue data    │    │ • Retry logic   │    │ • Watchdog feed │
│ • Calibration   │    │ • JSON format   │    │ • Statistics    │
└─────────────────┘    └─────────────────┘    └─────────────────┘
         │                       │                       │
         └───────────────────────┼───────────────────────┘
                                 │
                        ┌─────────────────┐
                        │   Event Group   │
                        │   & Queues      │
                        └─────────────────┘
```

