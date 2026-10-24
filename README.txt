ESP32 Audio EQ / Wi-Fi Audio 测量工具
=====================================

本仓库包含 ESP32 I2S 音频 EQ 固件，以及用于测量、编辑和试听 EQ 配置的
桌面 ALSA/PHP 工具。

主要目录
--------
esp32/       ESP32 固件（I2S ADC -> 24-bit 立体声 EQ -> I2S DAC）
config/      EQ 预设和由 REW 导出的配置文件
measure/     用于调音和对比的测量数据
tests/       主机端 DSP 边界测试
testi2s/     旧的 Arduino I2S 测试 sketch

ESP32 固件
----------
引脚、构建/烧录、HTTP 接口、NVS 键、EQ 文件格式、I2S 数据格式和性能
说明详见 esp32/README.md。

快速构建：
  cd esp32
  . $IDF_PATH/export.sh
  idf.py build
  idf.py -p /dev/ttyUSB0 flash monitor

通过 GPIO33 启动 Wi-Fi/HTTP 后，编辑入口为：
  http://192.168.4.1/eq_l
  http://192.168.4.1/eq_r

保存编辑页会立即热加载新 EQ，同时把可选的 Swap L/R 设置持久化到 NVS。

桌面工具
--------
  sh make.sh

该命令在 bin/ 下构建 record、play、line、eq、wave 和 cc。基于 ALSA 的
record/play/line/eq 需要 ALSA 开发头文件（Debian/Ubuntu 的
libasound2-dev）；wave 和 cc 不需要 ALSA。

PHP 页面（index.php、action.php、line.php）与 ESP32 的 HTTP 页面相互独立。
它需要 PHP GD、arial.ttf，并在 config.php 中配置正确的 $HOME 路径。

主机 DSP 测试：
  make -C tests test

重要说明
--------
- ESP32 的 softAP 和 HTTP 服务没有认证，只应在可信网络中使用。
- ESP32 的 EQ 直接存储在 NVS 键 eq_l 和 eq_r；旧的 l_config/r_config
  间接引用不再使用。
- 可选的左右交换设置存储在 NVS 键 swap_lr。
- maxvol.c 是独立的旧工具，不属于固件，也不在默认 make.sh 构建列表中。
