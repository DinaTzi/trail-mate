# Tab5 Board Facts

Sources:

- `boards/tab5/include/boards/tab5/board_profile.h`
- `boards/tab5/src/tab5_board.cpp`

This record describes hardware facts only.

## Identity

- board package: `boards/tab5`
- board id: `tab5`
- platform family: `esp32`
- product name: `Trail Mate Tab5`

## Confirmed Facts

- display present: yes
- display dimensions: not recorded in the current board profile
- touch present: yes
- keyboard present: no
- audio present: yes
- SD card present: yes
- GPS UART present: yes
- RS485 UART present: yes
- LoRa present: yes
- M5-Bus LoRa module routing present: yes
- ESP32-C6 companion present: yes
- ESP32-C6 SDIO2 reference: CMD=13, CLK=12, D0=11, D1=10, D2=9, D3=8
- ESP32-C6 reset reference: GPIO54, requires validation before runtime use
- motion sensors present: yes, BMI270+BMM150
