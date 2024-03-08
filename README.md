# `FreeRTOS/Source/portable/ThirdParty/GCC/RP2040`を使う

1. I2C接続のBME280からデータを取得し、SPI接続のOLEDディスプレイ
   (SSD1331)に表示する
2. データの受け渡しはQueueで行う
3. `Demo/ThirdParty/Community-Supported-Demos/CORTEX_M0+_RP2040/Standard/main_blinky.c`を引用し、不要な部分を削除して現在形に

<video src="running.mov" controls></video>
