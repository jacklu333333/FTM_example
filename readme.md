# FTM Example
> This is the repo for the FTM client and station for the module of TTGO T-display S2. The client will automatically search the station prefix and tried to initiate the FTM protocol. The client will return the result via serial port communication specify the distance and SSID with time. The time sync for NTP is not implement, should be implement via init of serial port, and get the global time from the master processor.

### spec
1. Framework: Arduino
2. Processor: Esp32-S2

### Note 
1. Please add the TFT_eSPI to the arduino library directory.
2. Please install esp32 board library version 2.0.17