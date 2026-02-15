# Adaptive Lighting MQTT Message Contract

## Table of contents
- [1. Overview](#1-overview)
- [2. Base message format](#2-base-message-format)
- [3. Event specifications](#3-event-specifications)
    - [3.1 SET_RGB](#31-set_rgb)
    - [3.2 TOGGLE_ADAPTIVE_LIGHTING_MODE](#32-toggle_adaptive_lighting_mode)
    - [3.3 OCCUPANCY_UPDATE](#33-occupancy_update)
- [4. Topic architecture and message flow](#4-topic-architecture-and-message-flow)
- [5. Versioning rules](#5-versioning-rules)

---

## 1. Overview

This document defines the MQTT message format exchanged between the following system components:

- Server / App (logical unified entity)
- ESP32 main controller
- ESP32 light devices

The goal is predictable, consistent communication that remains simple for embedded firmware parsing.

The main controller is responsible for all autonomous lighting decisions, including adaptive lighting and occupancy-based control. The application acts as a configuration and visualization interface and is not part of the real-time control loop.

---

## 2. Base message format

All MQTT payloads are JSON objects using this schema:

```json
{
  "origin": "MAIN" | "APP",
  "device": "APP" | "LIGHT",
  "action": "<string>",
  "payload": { /* action-specific object */ }
}
```

--- 
## 3. Event specifications 
### 3.1: SET_RGB 
- Direction: Server → Light 
- Device: LIGHT 
- Action: SET_RGB 
Payload (object): 

| Field | Type | Required | Notes | 
|---|---|---|---|
| r | uint8 | yes | Range: 0–255 | 
| g | uint8 | yes | Range: 0–255 | 
| b | uint8 | yes | Range: 0–255 | 

Example:
```json
{
  "origin": "APP",
  "device": "LIGHT",
  "action": "SET_RGB",
  "payload": {
    "r": 255,
    "g": 120,
    "b": 40
  }
}
```


--- 
### 3.2: TOGGLE_ADAPTIVE_LIGHTING_MODE 
- Direction: Server → Light 
- Device: LIGHT 
- Action: TOGGLE_ADAPTIVE_LIGHTING_MODE 
Payload is a union depending on enabled. 

| Field | Type | Required | Notes | 
|---|---|---|---|
| enabled | boolean | yes | true = enable mode | 
| wake_time | string | when enabled | Format: HH:MM | 
| sleep_time | string | when enabled | Format: HH:MM | 

Behavior (ESP device): 
- If enabled is false → exit adaptive lighting mode. 
- If enabled is true → enter adaptive lighting mode and schedule wake_time / sleep_time. 

Example (enable):
```json
{
  "origin": "APP",
  "device": "LIGHT",
  "action": "TOGGLE_ADAPTIVE_LIGHTING_MODE",
  "payload": {
    "enabled": true,
    "wake_time": "07:30",
    "sleep_time": "23:30"
  }
}
```

--- 
### 3.3: OCC_UPDATE

Represents an occupancy state change detected by an occupancy sensor and reported to the main controller.  
This event enables autonomous lighting control without application involvement.

- **Direction:** Occupancy Sensor → Main Controller
- **Target Device:** Main Controller
- **Action:** `OCC_UPDATE`

#### Payload fields

| Field      | Type    | Required | Notes |
|-----------|---------|----------|------|
| occupied  | boolean | yes      | `true` = room occupied, `false` = room unoccupied |
| room_id   | string  | yes      | Logical room identifier the sensor belongs to |
| sensor_id | string  | no       | Optional unique ID of the reporting sensor |
| timestamp | number  | no       | Unix timestamp (seconds); may be added by publisher or broker |

#### Behavior (ESP32 Main Controller)

- Upon receiving `occupied = true`:
  - Cancel any pending vacancy timers for the associated room
  - Mark the room as occupied in internal state
  - MAY restore lighting state depending on configuration

- Upon receiving `occupied = false`:
  - Start (or restart) a vacancy timeout timer for the associated room
  - If no subsequent `occupied = true` event is received before timeout expiry:
    - Issue light control commands to turn off or dim lights in the room
    - Update internal room occupancy state

- The main controller is the **sole authority** for occupancy-based lighting behavior
- The application MUST NOT be required for occupancy automation to function

#### Messaging rules

- `OCC_UPDATE` messages are **event-driven**, not periodic
- Messages SHOULD NOT be retained
- Duplicate `occupied` values MAY be ignored if no state transition occurs
- QoS 1 is recommended to ensure delivery

#### Example

```json
{
  "origin": "OCC",
  "device": "MAIN",
  "action": "OCC_UPDATE",
  "payload": {
    "occupied": true,
    "room_id": "living_room",
    "sensor_id": "occ_01",
    "timestamp": 1700000000
  }
}

---


## 4. Topic architecture and message flow

This system uses a **centralized routing model** in which the ESP32 main controller acts as the authority for lighting behavior.

The Server / App publishes **intent** (what should happen).  
The ESP32 main controller interprets that intent and **routes executable commands** to individual light devices.  
ESP32 light devices execute commands and publish **status updates**.

This separation ensures consistent behavior, enables room-based control, and prevents devices from bypassing system logic.

---

### 4.1 Topic hierarchy

Topics are structured to clearly encode:

- Message direction (command vs status)
- Device type
- Room grouping
- Optional device identity


```text
als/<direction>/<device>/<room>/<device_id?>
```

Where:

- `<direction>` = `cmd` or `status`
- `<device>` = `light`, `occ_sensor`, `daylight_sensor`, etc.
- `<room>` = logical room identifier
- `<device_id>` = optional unique device identifier

---

#### Commands (downstream)

```text
als/cmd/light/<room>
als/cmd/light/<room>/<light_id>
```

- Room-level topics target multiple devices simultaneously.
- Device-level topics target a specific device.


#### Status / events (upstream)
```text
als/status/light/<room>/<light_id>
```

- Status messages are always device-specific.
- Status topics are never used for commands.

### 4.2 Publisher / subscriber roles

#### Server / App

Publishes:

```text
als/cmd/light/<room>
```

Subscribes:

```text
als/cmd/+/+/+
```

#### ESP32 Main Controller

Subscribes:

```
als/cmd/+/+/+
```

Publishes:

```text
als/cmd/light/<room>/<light_id>
als/status/main/system
```

#### ESP32 Light Devices

Subscribes:

```text
als/cmd/light/<room>/<light_id>
als/cmd/light/<room>        (optional)
```

Publishes:

```text
als/status/light/<room>/<light_id>
```

Light devices do not interpret system-wide intent and do not route messages.

### 4.3 Design rules

- Light devices MUST NOT subscribe to topics outside their device type.
- Light devices MUST NOT publish to any cmd topic.
- Topics define who receives a message.
- Payloads define what action is performed.
- Room-level commands enable group control.
- Device-level commands enable precise targeting.
- Status topics are strictly informational and never used for control.

---

## 5. Versioning rules

To maintain compatibility across devices and firmware versions:

- Unknown fields MUST be ignored by the receiver.
- New actions MAY be added over time.
- Existing actions MUST NOT change their meaning.
