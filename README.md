# DTC/DMAC Demo
## Compare of DTC and DMAC of RA4M1 MCU

It contains the application source files to support an article post of Renesas RA4M1 MCU on Arduino Uno R4 Wifi board.

This demo code presents a practical comparison to show how DMAC and DTC both transfer the same data from one RAM buffer to another, but use different triggering methods. The demo highlights that DMAC can start immediately by software, while DTC waits for a hardware timer event.

## General usage
To use this project file, you need to install the Arduino IDE and add the Renesas R4 core. This will allow you to use the RA FSP (Flexible Software Package) within the Arduino environment
```
#include "FspTimer.h"
#include "r_dtc.h"
#include "r_dmac.h"
```
