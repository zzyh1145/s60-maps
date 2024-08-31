[![GitHub release (latest by date)](https://img.shields.io/github/v/release/artem78/s60-maps?style=plastic)](https://github.com/artem78/s60-maps/releases/latest)&nbsp;&nbsp;&nbsp;[![GitHub license](https://img.shields.io/github/license/artem78/s60-maps?style=plastic)](https://github.com/artem78/s60-maps/blob/master/LICENSE.txt)&nbsp;&nbsp;&nbsp;![GitHub All Releases](https://img.shields.io/github/downloads/artem78/s60-maps/total?style=plastic)&nbsp;&nbsp;&nbsp;![GitHub last commit](https://img.shields.io/github/last-commit/artem78/s60-maps?style=plastic)

![icon](images/program_logo.png)
# S60Maps

为 [*Symbian OS 9.x*](https://en.wikipedia.org/wiki/Symbian#Version_comparison)手机提供的地图和导航应用程序，支持 [*Series S60 3rd/5th Edition*](https://en.wikipedia.org/wiki/S60_%28software_platform%29#Versions_and_supported_devices).

----

## 功能

- 支持四种 [OpenStreetMap](https://www.openstreetmap.org/) 图层:
  - [OSM Standard layer](https://wiki.openstreetmap.org/wiki/Standard_tile_layer)
  - [Cycle Map layer](https://wiki.openstreetmap.org/wiki/OpenCycleMap)
  - [Transport Map layer](https://wiki.openstreetmap.org/wiki/Transport_Map)
  - [Humanitarian Map layer](https://wiki.openstreetmap.org/wiki/Humanitarian_map_style)
- 使用 GPS 在地图上显示手机位置
- 可以在没有 GPS 的情况下工作（无需定位）
- 地图离线缓存
- 显示和编辑地标
- 多语言支持：
  - 英语
  - 西班牙语
  - 加利西亚语
  - 葡萄牙语
  - 俄语
  - 波兰语
  - 希伯来语
  - 拉丁美洲西班牙语
  - 乌克兰语
- 支持按键与触屏操作
- 免费和开源软件

## 控制

### 按键操作

- 移动: <kbd>←</kbd>/<kbd>↑</kbd>/<kbd>→</kbd>/<kbd>↓</kbd> (或者 <kbd>2</kbd>/<kbd>4</kbd>/<kbd>6</kbd>/<kbd>8</kbd>)
- 放大: <kbd>▲</kbd> 音量+ (或 <kbd>1</kbd>)
- 缩小: <kbd>▼</kbd> 音量- (或 <kbd>3</kbd>)
- 标记:
  - 创建或重命名: <kbd>5</kbd>
  - 删除 <kbd>C</kbd> (删除键)

### 触屏操作

- 点击屏幕的 *顶部* / *底部* / *左侧* / *右侧 * —  *向上* / *向下* / *左* / *右*
-  *长按后滑动* — 快速移动
- 向 *左* 滑动 / *右* —  *放大* / *缩小*
- 向 *上* / *下* 滑动 —  *显示* / *隐藏* 软按键 (屏幕上的 <kbd>选项</kbd> & <kbd>退出</kbd>)

## 截图

![](images/demo_video.gif "Demo") ![](images/layers.gif "Different map layers") ![](images/position_and_landmarks.png "Main view with landmarks") ![](images/menu.png "Main menu") ![](images/landmarks_list.png "List of landmarks with filter") ![](images/settings.png "Settings window")
## 支持的设备

此软件已在如下设备进行了测试:

- 手机:
  - `OK` [Nokia 808](https://en.wikipedia.org/wiki/Nokia_808_PureView) (感谢 [WunderWungiel](https://github.com/WunderWungiel)) 
  - `OK` [Nokia 5530 XM](https://en.wikipedia.org/wiki/Nokia_5530_XpressMusic) (感谢 [Baranovskiy Konstantin](https://github.com/baranovskiykonstantin))
  - `OK` [Nokia 5800](https://en.wikipedia.org/wiki/Nokia_5800_XpressMusic) (感谢 [fedor4ever](https://github.com/fedor4ever))
  - `OK` [Nokia C5-00 5MP](https://en.wikipedia.org/wiki/Nokia_C5-00) (感谢 [Men770](https://github.com/Men770))
  - `OK` [Nokia E52](https://en.wikipedia.org/wiki/Nokia_E52/E55) (感谢 [Fizolas](https://github.com/fizolas))
  - `OK` [Nokia E63](https://en.wikipedia.org/wiki/Nokia_E63) (感谢 [Fizolas](https://github.com/fizolas))
  - `OK` [Nokia E71](https://en.wikipedia.org/wiki/Nokia_E71) (感谢 [misheu12](https://github.com/misheu12))
  - `OK` [Nokia E72](https://en.wikipedia.org/wiki/Nokia_E72) (感谢 [Fizolas](https://github.com/fizolas))
  - `OK` [Nokia N8](https://en.wikipedia.org/wiki/Nokia_N8) (感谢 Alistair Inglis)
  - `OK` [Nokia N95 8GB](https://en.wikipedia.org/wiki/Nokia_N95#Variations) (我的手机)
  - `OK` [Samsung SGH-i550](https://www.phonearena.com/phones/Samsung-SGH-i550_id2345) (感谢 [Ilya Vysotsky](https://github.com/Computershik73))
- 模拟器:
  - `FAILED` [EKA2L1](https://github.com/EKA2L1/EKA2L1) (here is [issue thread](https://github.com/EKA2L1/EKA2L1/issues/231))

> **注意:** If you can test it works (or not) on other Series S60 devices or emulators, let me know. Also send me some screenshots in order to check a look on different sizes/orientations.

## Download

- Download and install `*.sis` or `*.sisx` (*unsigned!*) package from [release page](../../../releases/latest/).
- If your smartphone is locked (by default):
  - Sign package with developer certificate ([details](https://digipassion.com/signing-sissisx-files-for-symbian-s60/));
  - Install already signed package on your smartphone.
- If your smartphone is unlocked ('rooted') you may install provided unsigned package directly.

## Technical details

All data stored in directory `E:\Data\S60Maps\` (**note:** E drive used regardles of on which drive program installed). Map cache located in `E:\Data\S60Maps\cache\_PAlbTN\<map service>\` directory.

Settings store in `store.dat` file. If you have problems with application to run, try to delete this file and run again.

S60Maps uses phone's landmark database and all of them will be accessed within the application. New landmarks will be added to `S60Maps` category.

## How to build

Read [docs/COMPILING.md](/docs/COMPILING.md)

## Roadmap

- [x] Add support for other map layers and WMS services (like OSM bicycle, OSM humanitarian, OpenTopoMap, etc...) 
- [ ] Add ability to define custom map layers providing tile\`s URLs
- [x] Update old cached tiles **(done: manually for viewed area)**
- [x] Display and edit landmarks
- [ ] Search (using [Nominatim](https://nominatim.openstreetmap.org/))
- [x] Show satellites info (amount, signal strength, etc...)
- [ ] Offline maps (zipped set of PNGs)

## License

This is Open Source software licensed under [GNU GPL v3.0](/LICENSE.txt)

## Donate

- PayPal: megabyte1024@yandex.com
- ETH Ethereum / Tether USDT: 0xB14C877b2eAF7E3b4b49df25039122C0545edA74
- Webmoney WMZ: Z598881055273
- Sberbank card: 5469 4009 8490 5476

## See also

- [About OpenStreetMap](https://wiki.openstreetmap.org/wiki/About_OpenStreetMap)
  - [Symbian software on OpenStreetMap Wiki](https://wiki.openstreetmap.org/wiki/Symbian)
- [GPS Track recorder for Symbian OS](https://github.com/artem78/s60-gps-tracker#readme)
- [Accuracy of GNSS data](https://wiki.openstreetmap.org/wiki/Accuracy_of_GNSS_data)
