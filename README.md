## Pin Table
| Signal | GPIO Used | Connection for the test |
|---|---|---|
| UART TX | D18 | Jumper to D19 |
| UART RX | D19 | Jumper to D18 |
| I²C SDA | D21 | OLED SDA |
| I²C SCL | D22 | OLED SCL |
| SPI SCK | D18 | (Not used in loopback) |
| SPI MOSI | D23 | Jumper to D19 |
| SPI MISO | D19 | Jumper to D23 |
| SPI CS | D5 | (Not used in loopback) |

## Expected vs Observed Table

| Test | Expected | Observed |
|---|---|---|
| UART with jumper | Echoes PING | Echoes PINGPING |
| UART without jumper | No echo | Sent PING... no echo |
| I²C with module | Finds address (e.g. 0x3C) | Found device at 0x3C |
| I²C without module | No devices found | No I2C devices found |
| SPI with jumper | Sent 0xA5, received 0xA5 | Sent 0xA5, received 0xA5 |
| SPI without jumper | Sent 0xA5, received 0x0 | Sent 0xA5, received 0xFF / 0x0 |
| Sketch at 9600, monitor at 115200 | Garbage characters | Garbage characters |
| Sketch at 9600, monitor at 9600 | Clean text | Clean text |

## Questions

**1. Why does UART need both sides to use the same baud rate, while SPI uses a clock line?**
> UART is asynchronous, meaning there is no shared clock. Both sides must agree on the exact time each bit takes (baud rate) to read the data correctly. SPI is synchronous, meaning the Master generates a dedicated clock signal (SCK) that tells the receiver exactly when to read each bit.

**2. Why can two devices with the same address not share one I²C bus?**
> Because I²C uses addresses to identify devices. If two devices have the same address, the Master will get confused about which device it is trying to talk to, and both might try to respond at the same time, causing a collision.

**3. Why is CS set HIGH between SPI transfers?**
> CS (Chip Select) is active LOW. Setting it HIGH deselects the slave device, ensuring it ignores any random data on the SPI lines until the Master is ready to start a new transfer.
