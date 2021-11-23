## 俄罗斯方块Arduino版本 (Tetris game for Arduino)

#### 运行环境
* Wio Terminal [Arduino Seeed平台开发套件](https://wiki.seeedstudio.com/Wio-Terminal-Getting-Started/)
* LCD分辨率 320*240
* LCD驱动 ILI9341

#### 快捷键
* Wio Terminal五向摇杆控制方向，向上为旋转方块
* Wio Terminal扩展按钮B 暂停
* Wio Terminal扩展按钮A 旋转

#### 备注
1. 如果是其他Arduino版本，需配置LCD屏，按要求连接针脚，可参考[LCD WIKI](http://www.lcdwiki.com/zh/%E9%A6%96%E9%A1%B5#SPI_Display_Module)
2. 如果使用其他分辨率的LCD屏，可按需修改代码配置，最小要求320*240，过小则会导致显示不全
2. 按钮连接后，需要修改代码tetris.h中对应的按钮针脚
