# Adaptive Lighting MQTT Message Contract

## Table of contents
- [1. Overview](#1-overview)
- [2. Base message format](#2-base-message-format)
- [3. Event specifications](#3-event-specifications)
  - [3.1 SET_RGB](#31-set_rgb)
  - [3.2 TOGGLE_ADAPTIVE_LIGHTING_MODE](#32-toggle_adaptive_lighting_mode)
- [4. Topic architecture and message flow](#4-topic-architecture-and-message-flow)
- [5. Versioning rules](#5-versioning-rules)

---

## 1. Overview

This document defines the MQTT message format exchanged between the following system components:

- Server / App (logical unified entity)
- ESP32 main controller
- ESP32 light devices

The goal is predictable, consistent communication that remains simple for embedded firmware parsing.

---

## 2. Base message format

All MQTT payloads are JSON objects using this schema:

```json
## 4. Topic architecture and message flow

This system uses a centralized routing model where the ESP32 main controller acts as the authority for lighting behavior. The server publishes intent; the main controller routes and executes commands to lights.

### 4.1 Topic hierarchy

Commands (downstream):

- `als/main/cmd`
- `als/room/<room_id>/cmd`
- `als/light/<light_id>/cmd`

Status / events (upstream):

- `als/main/status`
- `als/room/<room_id>/status`
- `als/light/<light_id>/status`

### 4.2 Publisher / subscriber roles


#### Server / App

Publishes:

```text
als/main/cmd
als/room/<room_id>/cmd
```

Subscribes:

```text
als/+/+/status
```

#### ESP32 Main Controller

Subscribes:

```text
als/main/cmd
als/room/+/cmd
```

Publishes:

```text
als/light/<light_id>/cmd
als/main/status
```

#### ESP32 Light Devices

Subscribes:

```text
als/light/<light_id>/cmd
als/room/<room_id>/cmd  (optional)
```

Publishes:

```text
als/light/<light_id>/status
```

### 4.3 Design rules

- Lights MUST NOT subscribe to `als/main/cmd`.
- Topics define who receives a message.
- Payloads define what action is performed.
- Room-level commands allow multiple lights to be controlled simultaneously.
- Device-level commands allow precise control when needed.

---

## 5. Versioning rules

To maintain compatibility across devices and firmware versions:

- Unknown fields MUST be ignored by the receiver.
- New actions MAY be added over time.
- Existing actions MUST NOT change their meaning.
adaptive-lighting/main/cmd
adaptive-lighting/room/+/cmd
```

Publishes:

```text
adaptive-lighting/light/<light_id>/cmd
adaptive-lighting/main/status
```

#### ESP32 Light Devices

Subscribes:

```text
adaptive-lighting/light/<light_id>/cmd
adaptive-lighting/room/<room_id>/cmd  (optional)
```

Publishes:

```text
adaptive-lighting/light/<light_id>/status
```

### 4.3 Design rules

- Lights MUST NOT subscribe to `adaptive-lighting/main/cmd`.
- Topics define who receives a message.
- Payloads define what action is performed.
- Room-level commands allow multiple lights to be controlled simultaneously.
- Device-level commands allow precise control when needed.

---

## 5. Versioning rules

To maintain compatibility across devices and firmware versions:

- Unknown fields MUST be ignored by the receiver.
- New actions MAY be added over time.
- Existing actions MUST NOT change their meaning.
