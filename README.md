# PlantsCare
![Status](https://img.shields.io/badge/Status-UnderDev-red.svg) [![Donate](https://img.shields.io/badge/Donate-Buy%20Me%20a%20Coffee-yellow.svg)](https://www.buymeacoffee.com/TiagoPaulaSilva)

PlantsCare is an IoT device for ultra low power standalone capacitive soil moisture meter.

## Features
##### Controller Device / [16-bit, RISC, MSP430 FRAM-based Microcontroller](https://github.com/TiagoPaulaSilva/PlantsCare/blob/master/Hardware/3.%20Datasheets/MCU/MSP430%20-%20Datasheet.pdf)

##### Wireless Technology / [Bluetooth Low Energy - CSR1010](https://github.com/TiagoPaulaSilva/WorkshySwitch/blob/master/Hardware/3.%20Datasheets/BLE/CSR1010%20(BLE%20IC)%20-%20Datasheet.pdf)

##### Low Power Requirements / [CR2450 Battery Life > 2 years](https://github.com/TiagoPaulaSilva/PlantsCare/blob/master/Hardware/3.%20Datasheets/Battery/CR2450.pdf)

##### Hardware Project / [Autodesk EAGLE](https://www.autodesk.com/products/eagle/free-download)

##### Firmware Project / [Code Composer Studio (CCS)](http://www.ti.com/tool/CCSTUDIO)

## PCB Gerber Preview
<img src="https://github.com/TiagoPaulaSilva/PlantsCare/blob/master/Others/PlantsCare%20Gerber%20Preview.png" width="10%" height="10%">

## Schematic
<img src="https://github.com/TiagoPaulaSilva/PlantsCare/blob/master/Others/PlantsCare%20Schematic%20Preview.png" width="100%" height="100%">

## Bill of Materials
| Qty | Parts | Description | Value | Package |
|--|--|--|--|--|
1|R2|SMD Resistor|47k ±5%|R0603
1|R4|SMD Resistor|1M ±5%|R0603
2|R6/R7|SMD Resistor|0R ±5%|R0603
3|R1/R3/R5|SMD Resistor|10k ±5%|R0603
1|C4|SMD Capacitor|1 nF / 50V|C0603
1|C7|SMD Capacitor|1 uF / 6.3V|C0603
1|C1|SMD Capacitor|10 uF / 50V|C0603
2|C2/C3|SMD Capacitor|100 nF / 50V|C0603
2|C5/C6|SMD Capacitor|22 pF / 50V|C0603
1|D1|Small Signal Fast Switching Diode|1N4148|SOD523
1|LED1|SMD LED|Blue|L0805
1|U1|Microcontroller|MSP430FR4133IG48R|TSSOP-48
1|Q1|SMD Crystal|32.768 kHz|3.2 x 1.5
1|PCB1|Qualcomm BLE CSRmesh Module|BLE-1010 MPCBA|-
1|BT1|Through Hole Coin Cell Battery Holder|CR2450|-
1|S1|SMD Switch/Momentary/Normally Open|SPST-NO|3 x 6 mm
1|CN1|Through Hole Connector/4 Pins/2 mm pitch|-|-

### Contributing
0. Give this project a :star:
1. Create an issue and describe your idea
2. [Fork it](https://github.com/TiagoPaulaSilva/PlantsCare/fork)
3. Create your feature branch (`git checkout -b my-new-feature`)
4. Commit your changes (`git commit -a -m "Added feature title"`)
5. Publish the branch (`git push origin my-new-feature`)
6. Create a new pull request
7. Done! :heavy_check_mark:

### License Information
<a rel="license" href="http://creativecommons.org/licenses/by-sa/4.0/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by-sa/4.0/88x31.png" /></a><br /><span xmlns:dct="http://purl.org/dc/terms/" property="dct:title">PlantsCare</span> by <a xmlns:cc="http://creativecommons.org/ns#" href="https://github.com/TiagoPaulaSilva" property="cc:attributionName" rel="cc:attributionURL">Tiago Silva</a> is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by-sa/4.0/">Creative Commons Attribution-ShareAlike 4.0 International License</a>.<br />Permissions beyond the scope of this license may be available at <a xmlns:cc="http://creativecommons.org/ns#" href="https://twitter.com/tiagopsilvaa" rel="cc:morePermissions">https://twitter.com/tiagopsilvaa</a>.
