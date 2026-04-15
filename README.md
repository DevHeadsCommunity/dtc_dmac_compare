# DTC/DMAC Demo
## Compare of DTC and DMAC of RA4M1 MCU

It contains the application source files to support an article post of Renesas RA4M1 on Arduino Uno R4 Wifi board.

This demo code presents a practical comparison to show how DMAC and DTC both transfer the same data from one RAM buffer to another, but use different triggering methods. The demo highlights that DMAC can start immediately by software, while DTC waits for a hardware timer event.

## General usage
To use this project file, you need to install the Arduino IDE and add ArduinoCore-renesas. This will allow you to use the RA FSP (Flexible Software Package) within the Arduino environment
```
#include "FspTimer.h"
#include "r_dtc.h"
#include "r_dmac.h"
```

## Building and flashing
To generate a binary file from the Arduino IDE, go to **Sketch** and select **“Export Compiled Binary”** from the list.

The compiled `.bin` file can be flashed onto the target hardware available on EdgeBench. Simply sign up, select the Arduino Uno R4 Wifi board, then navigate to the **Download** option from the left panel. Click the **Upload** button and browse to your compiled firmware image to proceed.

## Shell demo
Given that we use the Embedded Computing Workbench.
The program output can be visible in the **TERMINAL** section

