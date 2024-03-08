/*
 * FreeRTOS V202212.00
 * Copyright (C) 2020 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * https://www.FreeRTOS.org
 * https://github.com/FreeRTOS
 *
 */

/******************************************************************************
 */

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
//#include "semphr.h"
#include "portable.h"

/* Library includes. */
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/i2c.h"
#include "ssd1331.h"
#include "bme280.h"

/* タスク作成時のタスク優先度 */
#define mainQUEUE_OLED_PRIORITY         ( tskIDLE_PRIORITY + 1 )
#define mainQUEUE_BME280_PRIORITY       ( tskIDLE_PRIORITY + 1 )

/*-----------------------------------------------------------*/
static void prvSetupHardware( void );
//void vApplicationMallocFailedHook( void );
//void vApplicationIdleHook( void );
void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName );
//void vApplicationTickHook( void );

void prvOLEDTask(void *pvParameters);
void prvBME280Task(void *pvParameters);

/*-----------------------------------------------------------*/

/* 両タスクで使用するqueue */
QueueHandle_t LCDQueue = NULL;

/*-----------------------------------------------------------*/

int main( void )
{
    // ハードウェアの初期化
    prvSetupHardware();
    /* キューの作成 */
    LCDQueue = xQueueCreate( 2, 16 );    // 16 byte x 2

    if( LCDQueue != NULL )
    {
        // タスクを2つ作成。1つはキューからデータを受け取り表示するタスク
        xTaskCreate( prvOLEDTask,   /* タスクを実装している関数 */
                    "OLED",         /* タスク名: デバッグ用 */
                    1024,           /* タスクのスタックサイズ */
                    NULL,             /* タスクに渡すパラメタ */
                    mainQUEUE_OLED_PRIORITY,    /* タスク優先度 */
                    NULL );   /* タスクハンドル */
        // 環境データを読み取りキューに書き込むタスク
        xTaskCreate( prvBME280Task, "BME", 1024, NULL, mainQUEUE_BME280_PRIORITY, NULL );

        /* タスクの実行を開始する */
        vTaskStartScheduler();
    }

    for( ;; );

    return 0;
}
/*-----------------------------------------------------------*/
static void prvSetupHardware( void )
{
    // 標準stdio型の初期化: uartに必要
    stdio_init_all();
    // オンボードLEDの初期化
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, 1);
    gpio_put(PICO_DEFAULT_LED_PIN, !PICO_DEFAULT_LED_PIN_INVERTED);

    // SSD1331の初期化
    spi_init(spi0, 10*1000*1000);
    gpio_set_function(SPI0_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(SPI0_MOSI_PIN, GPIO_FUNC_SPI);
    gpio_init(SPI0_CSN_PIN);
    gpio_set_dir(SPI0_CSN_PIN, true);
    CS0_DESELECT;
    gpio_init(SPI0_DCN_PIN);
    gpio_set_dir(SPI0_DCN_PIN, true);
    DC0_DATA;
    gpio_init(SPI0_RESN_PIN);
    gpio_set_dir(SPI0_RESN_PIN, true);
    RESET0_OFF;

    // BME280の初期化
    // 500ms sampling time, x16 filter = 0x90
    i2c_init(i2c0, 500 * 1000);
    gpio_set_function(I2C0_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C0_SDA_PIN);
    gpio_pull_up(I2C0_SCL_PIN);
}

/*-----------------------------------------------------------*/

/*
void vApplicationMallocFailedHook( void )
{
     configASSERT( ( volatile void * ) NULL );
}
*/
/*-----------------------------------------------------------*/

void vApplicationStackOverflowHook( TaskHandle_t pxTask, char *pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) pxTask;

    configASSERT( ( volatile void * ) NULL );
}
/*-----------------------------------------------------------*/
/*
void vApplicationIdleHook( void )
{
    volatile size_t xFreeHeapSpace;
    xFreeHeapSpace = xPortGetFreeHeapSize();
    ( void ) xFreeHeapSpace;
}


void vApplicationTickHook( void )
{
}
*/
/*-----------------------------------------------------------*/
