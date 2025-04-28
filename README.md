# Fingerprint Match Detector
Jack Burton, Katherine Deane, Brady Kamali
### Overview

This project interfaces the Adafruit basic fingerprint sensor with the STM32L432KC using UART.  
It handles **fingerprint enrollment** and **matching** over UART via a function to construct command packets conforming to the sensor's documentation.

An attached Analog Discovery 2 (AD2) **monitors the UART** traffic using **UART Spy** mode. A helper script (`Match_detect.js`) detects when a **successful fingerprint match** occurs by identifying the signature `0x07 0x00 0x07 0x00` in the fingerprint sensor response packets and **sets an output pin (DIO6) high** for 5 seconds.

---

## Specification

**Platform:** PlatformIO  
**Board:** ST Nucleo-L432KC  
**Framework:** CMSIS  
**Peripherals:** [Adafruit Optical Fingerprint Sensor (Product 4690)](https://www.adafruit.com/product/4690)  
**External Tool:** Analog Discovery 2 (AD2) with WaveForms


## Structure

- `main.cpp` — Embedded code to enroll fingerprints and match prints continuously. Controls a servo to open/close upon match.
- `timer.cpp`, `uart.cpp`, `gpio.cpp` — Low-level CMSIS helper libraries (UART, GPIO, and PWM configuration).
- `Match_detect.js` — WaveForms script to monitor UART, detect fingerprint match, and toggle DIO6.

---

## How it Works

- STM32 talks to the fingerprint sensor over **USART1** at **57.6k baud** using packet structures outlined in the documentation.
- Upon a matching fingerprint, the sensor sends an ACK packet containing successful match sequence (`0x07 0x00 0x07 0x00`).
- AD2 spies on the UART line between STM32 and sensor.
- `Match_detect.js` listens for the success sequence. When detected:
  - **DIO6** is set HIGH (or **Wavegen1 Channel 1** is turned ON) for **5 seconds**.

---
## Quickstart

1. **Flash the Board**
   - Open the project in PlatformIO.
   - Build and upload to `nucleo_l432kc` board.

2. **Hardware Connections**
   - Connect the fingerprint sensor UART (TX/RX) to the Nucleo (PA9/PA10).
   - Use Analog Discovery 2 to **spy** on the UART line, by connecting DIO1/PIO0 to PA9/PA10 on the Nucleo.

3. **Run Detection Script**
   - Open WaveForms → **Protocol** → **UART Spy**.
   - Set: **Baud: 57.6k**, **Parity: None**, **8 Bits**, **Polarity: Standard**.
   - Open Wavegen, and configure an appropriate output voltage to perform the desired function on fingerprint match.
   - Load and run `Match_detect.js`.
   - Watch DIO6 (or Wavegen Channel 1) toggle high on fingerprint match.



## Setup Instructions

### 1. PlatformIO (Nucleo Firmware)

- Clone or download this project.
- Open it in PlatformIO.
- Confirm the following PlatformIO settings:

```ini
[env:nucleo_l432kc]
platform = ststm32
board = nucleo_l432kc
framework = cmsis
```

## Acknowledgements
Thank you to my friend and housemate Ege Ozgul for his help with debugging.

