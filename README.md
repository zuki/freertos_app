# Raspberry PicoでFreeRTOS

1. `FreeRTOS/Source/portable/ThirdParty/GCC/RP2040`を使用
   (環境変数`FREERTOS_KERNEL_PATH`に設定)
2. `FreeRTOS/Demo/ThirdParty/Community-Supported-Demos/CORTEX_M0+_RP2040/Standard/main_blinky.c`を修正して使用
3. I2C接続のBME280からデータを取得 (task 1) し、SPI接続のOLEDディスプレイ
   (SSD1331)に表示する (task 2) するtaskを作成
4. データの受け渡しはQueueを使う

https://github.com/zuki/freertos_app/assets/233833/fc55f445-2f18-43df-9beb-ecd701c26d71
