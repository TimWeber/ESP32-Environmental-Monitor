# ESP32 Environmental Monitoring System

An embedded environmental monitoring system built for the ESP32-C3. With the help of FreeRTOS, it reads temperature, humidity, and air quality data from sensors and sends it to a remote server over WiFi.

### Sensors
- **AHT21**: Temperature and humidity sensor with I2C communication
- **ENS160**: Air quality sensor that measures TVOC, eCO2, and AQI
- **I2C Manager**: Handles all the I2C communication in one place

### **Networking**
- **WiFi**: Connects to your network using credentials stored securely in NVS
- **HTTP Client**: Sends JSON data to your server with retry logic
- **SNTP**: Syncs time for accurate timestamps
- **Health Server**: Custom HTTP server providing system health monitoring

### **Configuration**
- **JSON config**: All settings in a JSON file that gets embedded in the firmware
- **Secure storage**: WiFi passwords and server URLs stored encrypted
- **Calibration**: Configurable sensor calibration and validation
- **User-configurable**: Sensor reading intervals, network transmission intervals, and validation thresholds are all configurable via settings.json

## Architecture

### ** Sensor Management**

```mermaid
graph TD
    A[ISensor Interface] --> B[SensorManager]
    C[AHT21Sensor] --> A
    D[ENS160Sensor] --> A
    E[Future Sensors] --> A
    
    B --> F[RTOSManager]
    B --> G[Health Monitor]
    B --> H[Network Task]
    
    I[Config Manager] --> B
    J[I2C Manager] --> C
    J --> D
    
    style A fill:#e1f5fe
    style B fill:#f3e5f5
    style C fill:#e8f5e8
    style D fill:#e8f5e8
    style E fill:#fff3e0
```

### **Data Flow**
```mermaid
graph TD
    A[AHT21 Sensor] -->|I2C| B[I2C Manager]
    C[ENS160 Sensor] -->|I2C| B
    B --> D[SensorManager]
    D --> E[Sensor Task]
    E -->|Calibration| F[Calibration Manager]
    F --> G[Data Queue]
    G --> H[Network Task]
    H -->|HTTP POST| I[Remote Server]
    
    J[Configuration] -->|JSON| K[Config Manager]
    K --> L[RTOS Manager]
    K --> M[Sensor Registry]
    L --> E
    L --> H
    
    N[WiFi Manager] -->|Credentials| H
    O[Secure Storage] -->|NVS| N
    P[Monitor Task] -->|Health Check| L
    P -->|Statistics| Q[System Stats]
    R[SNTP Manager] -->|Time Sync| H
    
    H -->|Health Stats| S[Health Monitor]
    E -->|Health Stats| S
    S --> T[Health Server]
    T -->|HTTP GET| U[Health Endpoint]
    
    V[Heartbeat Task] -->|Sensor Health| L
    V -->|Status Check| D
    
    style A fill:#e1f5fe
    style C fill:#e1f5fe
    style B fill:#f3e5f5
    style D fill:#fff3e0
    style E fill:#fff3e0
    style H fill:#fff3e0
    style P fill:#fff3e0
    style V fill:#fff3e0
    style I fill:#e8f5e8
    style K fill:#fce4ec
    style M fill:#fce4ec
    style O fill:#fce4ec
    style S fill:#fff8e1
    style T fill:#fff8e1
    style U fill:#e8f5e8
```

### **Task Structure**

```mermaid
graph LR
    subgraph "Core Tasks"
        SENSOR[Sensor Task<br/>• Read sensors via SensorManager<br/>• Validate data<br/>• Update health stats]
        NETWORK[Network Task<br/>• HTTP transmission<br/>• Retry logic<br/>• Update health stats]
        MONITOR[Monitor Task<br/>• System health<br/>• Statistics<br/>• Diagnostics]
        HEARTBEAT[Heartbeat Task<br/>• Sensor health monitoring<br/>• Status checks via SensorManager<br/>• Failure detection]
    end
    
    subgraph "Health System"
        HEALTH_MONITOR[Health Monitor<br/>• Statistics collection<br/>• Thread-safe counters]
        HEALTH_SERVER[Health Server<br/>• HTTP endpoint<br/>• GET /api/sensor#]
    end
    
    subgraph "System Protection"
        WATCHDOG[Watchdog Task<br/>• Task monitoring<br/>• System recovery]
    end
    
    subgraph "Communication"
        QUEUE[Sensor Queue<br/>• Data transfer]
        EVENTS[Event Group<br/>• Synchronisation]
    end
    
    %% Core task relationships
    SENSOR --> QUEUE
    QUEUE --> NETWORK
    SENSOR --> EVENTS
    NETWORK --> EVENTS
    MONITOR --> EVENTS
    HEARTBEAT --> EVENTS
    
    %% Health monitoring relationships
    SENSOR --> HEALTH_MONITOR
    NETWORK --> HEALTH_MONITOR
    HEARTBEAT --> HEALTH_MONITOR
    HEALTH_MONITOR --> HEALTH_SERVER
    
    %% Protection relationships
    WATCHDOG --> SENSOR
    WATCHDOG --> NETWORK
    WATCHDOG --> MONITOR
    WATCHDOG --> HEARTBEAT
    
    %% Styling
    classDef coreTask fill:#e3f2fd,stroke:#1976d2,stroke-width:2px
    classDef healthTask fill:#fff8e1,stroke:#f57c00,stroke-width:2px
    classDef protection fill:#fff3e0,stroke:#f57c00,stroke-width:2px
    classDef communication fill:#e8f5e8,stroke:#388e3c,stroke-width:2px
    
    class SENSOR,NETWORK,MONITOR,HEARTBEAT coreTask
    class HEALTH_MONITOR,HEALTH_SERVER healthTask
    class WATCHDOG protection
    class QUEUE,EVENTS communication
```

### **Task Scheduling and Timing**

```mermaid
gantt
    title ESP32 Environmental Monitor - Task Scheduling
    dateFormat  YYYY-MM-DD
    axisFormat %H:%M
    
    section Sensor Task
    SensorManager Read    :sensor_read, 2024-01-01, 10s
    Data Validation      :validation, after sensor_read, 10s
    Queue Send           :queue, after validation, 10s
    Health Update        :health, after queue, 5s
    
    section Network Task
    Queue Receive        :receive, after queue, 10s
    HTTP Request         :http, after receive, 40s
    Response Process     :response, after http, 10s
    Health Update        :health_net, after response, 5s
    
    section Monitor Task
    Health Check         :health, 2024-01-01, 10s
    Statistics           :stats, after health, 10s
    
    section Heartbeat Task
    Sensor Health        :heartbeat, 2024-01-01, 30s
    Status Check         :status, after heartbeat, 10s
    Failure Detect       :failure, after status, 5s
    
    section Health Monitoring
    Stats Collection     :stats_collect, 2024-01-01, 30s
    Endpoint Ready       :endpoint, after stats_collect, 10s
    
    section Watchdog
    Feed Watchdog        :watchdog, 2024-01-01, 50s
```

**Note**: All timing intervals are user-configurable via settings.json. Default values shown above can be customised at build time.

### **Health Monitoring System**

The system includes a comprehensive health monitoring system that tracks:

#### **Real-time Statistics**
- **Sensor readings**: Total successful and failed readings
- **Network transmissions**: Total successful and failed HTTP requests
- **System uptime**: Continuous uptime tracking
- **Memory usage**: Free memory monitoring
- **WiFi signal strength**: Connection quality monitoring

#### **Sensor Health**
- **Sensor connectivity**: AHT21 and ENS160 connection status
- **Success rates**: Percentage of successful sensor readings
- **Sensor states**: Current operational state (ready, warm_up, etc.)
- **Zero readings count**: Tracks invalid sensor data

#### **Network Health**
- **HTTP success rate**: Percentage of successful transmissions
- **Average response time**: Network performance metrics
- **Retry attempts**: Total retry attempts for failed requests

### **Health Endpoint**

The system provides a health monitoring endpoint at `http://<esp32-ip>/api/health`:

#### **Request**
```http
GET /api/health
```

#### **Response**
```json
{
  "device": "ESP32_Sensor_01",
  "timestamp": "2024-01-15T14:30:00Z",
  "uptime_seconds": 259200,
  "system_health": {
    "sensor_readings_total": 4320,
    "sensor_readings_failed": 12,
    "network_transmissions_total": 1440,
    "network_transmissions_failed": 8,
    "memory_free_bytes": 23552,
    "wifi_signal_dbm": -45,
    "watchdog_resets": 0
  },
  "sensor_health": {
    "aht21_connected": true,
    "ens160_connected": true,
    "aht21_success_rate": 99.7,
    "ens160_success_rate": 99.2,
    "zero_readings_count": 15,
    "aht21_state": "ready",
    "ens160_state": "warm_up"
  },
  "network_health": {
    "http_success_rate": 99.4,
    "average_response_time_ms": 1200,
    "retry_attempts_total": 23
  }
}
```
