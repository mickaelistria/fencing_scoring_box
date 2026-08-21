# Arduino Fencing Scoring Apparatus

## Description

Fencing Scoring Machine/Box/Apparatus code for the arduino platform.
This project is an arduino based fencing scoring machine. It supports all 3
weaspons, foil, epee and sabre.

Detailed assembly instructions are on instructables: 
https://www.instructables.com/id/Arduino-Fencing-Scoring-Apparatus/

## Directory Structure


**Firmware**: the code that implements fencing rules on the arduino and notify of touches.

**Hardware**: the circuit designs around the arduino showing how to connect up the lights and body wires.

**Software**: Communicates over the serial interface with the arduino to allow displaying of lights on a laptop or PC screen, and control various settings.

![Scroing application screenshot](software/node/screenshot.png "Scoring Application Screenshot")

**Notes and research**  Pretty self explanitory

## Hardware Requirements

Full configuration
 - 1 Arduino Uno or Nano (5v/16MHz)
 - 6x 1KΩ for pullups/pulldowns
 - 1 pullup switch to select weapon
 - For feedback/output (choose strategies in the code):
   - 1 ws2812b LED strip (recommended) 
   - 1 laptop with the software client to get lights and control weapons, or
   - individual leds with 220Ω resistors
   - 1 buzzer (optional, useles if using software client with buzzer built-in client)
 
Minimal configuration for sabre
 - 1 Arduino Uno or Nano
 - 4x 10kΩ resistors (we can ignore the "guard" wire)
 - 1 feedback option listed above
 
![MinimalSabreFront](hardware/minimalSabreFront.jpg "Minimal Sabre (Front)")
![MinimalSabreFront](hardware/minimalSabreBack.jpg "Minimal Sabre (Back)")
