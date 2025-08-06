### NuvoROM

This is an Arduino sketch to initialize LDROM (bootloader) for Nuvoton MCU (e.g.
N76E003). Once initialized, MCU can be programmed using any UART-TTL converter.

ICP programmer schematic:

```
    Arduino     Nuvoton
    5V      --- VDD
    Pin #4  --- DAT (P1.6)
    Pin #5  --- CLK (P0.2)
    Pin #6  --- RST (P2.0)
    GND     --- GND
```

Led on Pin #13 indicates error. If everything goes well then it will turn off.

LDROM code is in `ldrom/ISP_UART0.ihx`. Use [hex2c](https://github.com/matveyt/hex2c) to
convert it to C Include file.

Note: bootloader occupies 2KB of Flash memory.\
Note: any locked chip will be erased.
