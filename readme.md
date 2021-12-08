## 俄罗斯方块Arduino版本 (Tetris game for Arduino)

#### 运行环境
* Wio Terminal [Arduino Seeed平台开发套件](https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/)
* LCD分辨率 320*240
* LCD驱动 ILI9341
* Arduino依赖：
    - Seeed Wio Terminal 依赖包，参考wiki安装
    - RF24 [RF24依赖资料](https://nrf24.github.io/RF24/)（**非必要，使用JoyStick Shield的RF24远程控制**）

#### 快捷键
* Wio Terminal五向摇杆控制方向，向上为旋转方块
* Wio Terminal扩展按钮B 暂停
* Wio Terminal扩展按钮A 旋转

#### 备注
1. 如果是其他Arduino版本，需配置LCD屏，按要求连接针脚，可参考[LCD WIKI](http://www.lcdwiki.com/zh/%E9%A6%96%E9%A1%B5#SPI_Display_Module)
2. 如果使用其他分辨率的LCD屏，可按需修改代码配置，最小要求320*240，过小则会导致显示不全
3. 按钮连接后，需要修改代码tetris.h中对应的按钮针脚
4. 增加支持JoyStick Shield，支持NRF2401远程控制，可参考wiki [keyestudio JoyStick Shield](https://wiki.keyestudio.com/Ks0153_keyestudio_JoyStick_Shield)
5. JoyStick Shield控制端代码[https://github.com/oliverCJ/arduino_joystick_shield_rf24](https://github.com/oliverCJ/arduino_joystick_shield_rf24)
6. 受控端[NRF2401引脚](https://nrf24.github.io/RF24/)连线到Wio Terminal背部：

|  NRF24L01   | BCM | BOARD |
|  ---- | ---- | ---- |
| CE  | 23 | 16 |
| CSN  | 24 | 18 |
| MOSI  | 10 | 19 |
| MISO  | 9 | 21 |
| SCK  | 11 | 23 |
| VCC 3.3  | - | 17 |
| GND  | - | 25 |
| IRQ  | - | - |

#### 效果
![效果图](https://raw.githubusercontent.com/oliverCJ/arduino_tetris/master/show.jpeg)