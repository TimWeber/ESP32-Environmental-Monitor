# ESP32 Environmental Monitoring System

An embedded environmental monitoring system built for the ESP32-C3. With the help of FreeRTOS, it reads temperature, humidity, and air quality data from sensors and sends it to a remote server over WiFi.

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
- **User-configurable**: Sensor reading intervals, network transmission intervals, and validation thresholds are all configurable via settings.json

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

```mermaid
graph TB
    subgraph "FreeRTOS Task Architecture"
        subgraph "Core Tasks"
            SENSOR[Sensor Task<br/>• Read AHT21 & ENS160<br/>• Data validation<br/>• Environmental compensation]
            
            NETWORK[Network Task<br/>• HTTP transmission<br/>• Retry logic<br/>• JSON formatting]
            
            MONITOR[Monitor Task<br/>• System health<br/>• Statistics<br/>• Diagnostics]
        end
        
        subgraph "System Protection"
            WATCHDOG[Watchdog Task<br/>• Task monitoring<br/>• System recovery]
        end
        
        subgraph "Inter-Task Communication"
            QUEUE[Sensor Queue<br/>• Data transfer]
            EVENTS[Event Group<br/>• Synchronisation]
        end
    end
    
    %% Task relationships
    SENSOR --> QUEUE
    QUEUE --> NETWORK
    SENSOR --> EVENTS
    NETWORK --> EVENTS
    MONITOR --> EVENTS
    WATCHDOG --> SENSOR
    WATCHDOG --> NETWORK
    WATCHDOG --> MONITOR
    
    %% Styling
    classDef coreTask fill:#e3f2fd,stroke:#1976d2,stroke-width:2px
    classDef protection fill:#fff3e0,stroke:#f57c00,stroke-width:2px
    classDef communication fill:#e8f5e8,stroke:#388e3c,stroke-width:2px
    
    class SENSOR,NETWORK,MONITOR coreTask
    class WATCHDOG protection
    class QUEUE,EVENTS communication
```

### Task Scheduling and Timing

```mermaid
gantt
    title ESP32 Environmental Monitor - Task Scheduling
    dateFormat  YYYY-MM-DD
    axisFormat %H:%M
    
    section Sensor Task
    AHT21 Reading    :aht21, 2024-01-01, 10s
    ENS160 Reading   :ens160, after aht21, 10s
    Data Validation  :validation, after ens160, 10s
    Queue Send       :queue, after validation, 10s
    
    section Network Task
    Queue Receive    :receive, after queue, 10s
    HTTP Request     :http, after receive, 40s
    Response Process :response, after http, 10s
    
    section Monitor Task
    Health Check     :health, 2024-01-01, 10s
    Statistics       :stats, after health, 10s
    
    section Watchdog
    Feed Watchdog    :watchdog, 2024-01-01, 50s
```

**Note**: All timing intervals are user-configurable via settings.json. Default values shown above can be customised at build time.

### Sensor Data Validation Flow

```mermaid
flowchart TD
    SENSOR_TASK[Sensor Task] --> START[Read Sensor Data]
    
    %% AHT21 Flow (Left Side)
    START --> A1[Read AHT21 Data]
    A1 --> B1{AHT21 Valid?}
    B1 -->|Yes| C1[Check Temperature Range]
    B1 -->|No| D1[Log AHT21 Warning]
    
    C1 --> E1{Temperature in Range?}
    E1 -->|Yes| F1[Check Humidity Range]
    E1 -->|No| G1[Mark AHT21 Invalid]
    
    F1 --> H1{Humidity in Range?}
    H1 -->|Yes| I1[AHT21 Data Accepted]
    H1 -->|No| G1
    
    I1 --> J1[Set Environmental Compensation]
    J1 --> K1[Send AHT21 Data to Network]
    G1 --> L1[Skip AHT21 Database Post]
    D1 --> L1
    
    %% ENS160 Flow (Right Side)
    START --> A2[Read ENS160 Data]
    A2 --> B2{ENS160 Valid?}
    B2 -->|Yes| C2[Check AQI Range]
    B2 -->|No| D2[Log ENS160 Warning]
    
    C2 --> E2{AQI in Range?}
    E2 -->|Yes| F2[Check TVOC Range]
    E2 -->|No| G2[Mark ENS160 Invalid]
    
    F2 --> H2{TVOC in Range?}
    H2 -->|Yes| I2[Check eCO2 Range]
    H2 -->|No| G2
    
    I2 --> J2{eCO2 in Range?}
    J2 -->|Yes| K2[ENS160 Data Accepted]
    J2 -->|No| G2
    
    K2 --> L2[Send ENS160 Data to Network]
    G2 --> M2[Skip ENS160 Database Post]
    D2 --> M2
    
    %% Combined Results
    K1 --> COMBINE[Combine Data]
    L2 --> COMBINE
    L1 --> COMBINE
    M2 --> COMBINE
    
    COMBINE --> VALIDATE{Any Invalid Data?}
    VALIDATE -->|No| SEND[Send to Network Task]
    VALIDATE -->|Yes| SKIP[Skip Database Post]
    
    SEND --> HTTP[HTTP POST to Server]
    HTTP --> DB[Database Storage]
    SKIP --> MONITOR[Continue Monitoring]
```

**Note**: All validation thresholds (temperature, humidity, AQI, TVOC, eCO2 ranges) are user-configurable via settings.json.

## Key System Components

### Tasks
- **Sensor Task**: Reads AHT21 and ENS160 sensors every 1 minute (configurable)
- **Network Task**: Handles HTTP transmission to server with configurable retry logic
- **Monitor Task**: System health monitoring and statistics collection
- **Watchdog Task**: Prevents system hangs and provides recovery mechanisms
- **Heartbeat Task**: Sensor health monitoring

### Data Flow
1. **Sensor Reading**: AHT21 → ENS160 (environmental compensation)
2. **Data Validation**: Threshold checking for AQI, TVOC, eCO2 (all configurable)
3. **Queue Management**: Inter-task communication via FreeRTOS queues
4. **Network Transmission**: HTTP POST with configurable retry logic
5. **Database Storage**: Server-side data persistence

### Error Handling
- **Sensor Failures**: Automatic reset and recovery mechanisms
- **Network Failures**: Configurable retry logic and timeout settings
- **System Monitoring**: Watchdog and heartbeat protection

### User Configuration
All system parameters are configurable via settings.json:
- **Sensor reading intervals**: How frequently sensors are read
- **Network transmission intervals**: How often data is sent to server
- **Validation thresholds**: Acceptable ranges for all sensor data
- **Retry logic**: Network retry attempts and delays
- **WiFi settings**: SSID, password, and connection timeouts
- **Server settings**: URL, request timeouts, and authentication 
