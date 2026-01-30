# Adaptive Lighting System - Main Control 

Main Control code for Adaptive Lighting System running on ESP32 Development Board

## Getting Started

These instructions will get you a copy of the project up and running on your local machine.

### Prerequisites

*   [ESP-IDF](https://github.com/espressif/esp-idf) framework installed and configured.
*   Properly connected ESP board (e.g., ESP32-DevKitC).
*   Create a new branch (name it after your device) and develop within that
*   Go to dependencies.lock and change `USER` to your computer's user. Or, find the location of the your esp folder and change the directory accordingly (make sure you can find protocol_examples_common within the folder)
*   Search for the `TODO` locations and make those changes as you develop

### Building and Flashing

Once configured, build the project, flash it to your board, and monitor the device ouput:

```bash
idf.py build 
idf.py flash 
idf.py monitor