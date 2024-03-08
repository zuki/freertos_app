/*
 *     環境センサーBME280制御タスク
*/
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "time.h"
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "bme280.h"

/* The LED toggled by the Rx task. */
#define TASK_LED    ( PICO_DEFAULT_LED_PIN )

extern QueueHandle_t  LCDQueue;

// キュー送信用バッファ定義
union {
    uint8_t buf[16];
    struct {
        int32_t pres;
        int32_t temp;
        int32_t humi;
    } mes;
} Measure;

// 気圧/気温/湿度の較正に使用する高精度な温度を計算する中間関数
static int32_t bme280_convert(int32_t temp, struct bme280_calib_param* params) {
    // データシートに記載されている32ビット固定小数点較正実装を使用する
    int32_t var1, var2;
    var1 = ((((temp >> 3) - ((int32_t)params->dig_t1 << 1))) *
            ((int32_t)params->dig_t2)) >> 11;
    var2 = (((((temp >> 4) - ((int32_t)params->dig_t1)) *
              ((temp >> 4) - ((int32_t)params->dig_t1))) >> 12) *
            ((int32_t)params->dig_t3)) >> 14;

    return var1 + var2;
}

// 原データ（気温）を較正する
static int32_t bme280_convert_temp(int32_t temp, struct bme280_calib_param* params) {
    int32_t t_fine = bme280_convert(temp, params);
    return (t_fine * 5 + 128) >> 8;
}

// 原データ（気圧）を較正する
static int32_t bme280_convert_pressure(int32_t pressure, int32_t temp, struct bme280_calib_param* params) {
    int32_t t_fine = bme280_convert(temp, params);

    int32_t var1, var2;
    uint32_t converted;
    var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)params->dig_p6);
    var2 += ((var1 * ((int32_t)params->dig_p5)) << 1);
    var2 = (var2 >> 2) + (((int32_t)params->dig_p4) << 16);
    var1 = (((params->dig_p3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) +
            ((((int32_t)params->dig_p2) * var1) >> 1)) >> 18;
    var1 = ((((32768 + var1)) * ((int32_t)params->dig_p1)) >> 15);
    if (var1 == 0) {
        return 0;  // 0除算例外を防ぐ
    }
    converted = (((uint32_t)(((int32_t)1048576) - pressure) - (var2 >> 12))) * 3125;
    if (converted < 0x80000000) {
        converted = (converted << 1) / ((uint32_t)var1);
    } else {
        converted = (converted / (uint32_t)var1) * 2;
    }
    var1 = (((int32_t)params->dig_p9) *
            ((int32_t)(((converted >> 3) * (converted >> 3)) >> 13))) >>
           12;
    var2 = (((int32_t)(converted >> 2)) * ((int32_t)params->dig_p8)) >> 13;
    converted =
        (uint32_t)((int32_t)converted + ((var1 + var2 + params->dig_p7) >> 4));
    return converted;
}

// 原データ（湿度）を較正する
static uint32_t bme280_convert_humidity(int32_t humidity, int32_t temp, struct bme280_calib_param* params) {
    int32_t t_fine = bme280_convert(temp, params);

    int32_t var1;
    var1 = (t_fine - ((int32_t)76800));
    var1 = (((((humidity << 14) - (((int32_t)params->dig_h4) << 20) -
               (((int32_t)params->dig_h5) * var1)) + ((int32_t)16384)) >> 15) *
            (((((((var1 * ((int32_t)params->dig_h6)) >> 10) *
                 (((var1 * ((int32_t)params->dig_h3)) >> 11) +
                  ((int32_t)32768))) >> 10) +
               ((int32_t)2097152)) * ((int32_t)params->dig_h2) + 8192) >> 14));
    var1 = (var1 - (((((var1 >> 15) * (var1 >> 15)) >> 7) * ((int32_t)params->dig_h1)) >> 4));
    var1 = (var1 < 0 ? 0 : var1);
    var1 = (var1 > 419430400 ? 419430400 : var1);

    return (uint32_t)(var1 >> 12);
}

// 原データ取得
static void bme280_read_raw(int32_t* temperature, int32_t* pressure, int32_t* humidity) {
    uint8_t buf[8];
    uint8_t reg = REG_PRESSURE_MSB;

    i2c_write_blocking(i2c0, BME280_ADDR, &reg, 1, true);
    i2c_read_blocking(i2c0, BME280_ADDR, buf, 8, false);

    // 読み取った20/16ビットの値を変換用に32ビットの符号付き整数に格納する
    *pressure = (int32_t)(buf[0] << 12) | (buf[1] << 4) | (buf[2] >> 4);
    *temperature = (int32_t)(buf[3] << 12) | (buf[4] << 4) | (buf[5] >> 4);
    *humidity = (int32_t)(buf[6] << 8) | buf[7];
}

// 較正パラメタの取得
static void bme280_get_calib_params(struct bme280_calib_param* params) {
    uint8_t buf[NUM_CALIB_PARAMS1] = {0};
    uint8_t reg1 = REG_DIG_T1_LSB;
    uint8_t reg2 = REG_DIG_H2_LSB;

    // read 0x88 - 0xa1
    i2c_write_blocking(i2c0, BME280_ADDR, &reg1, 1, true);
    // read in one go as register addresses auto-increment
    i2c_read_blocking(i2c0, BME280_ADDR, buf, NUM_CALIB_PARAMS1, false);

    // store these in a struct for later use
    params->dig_t1 = (uint16_t)(buf[1] << 8) | (uint16_t)buf[0];
    params->dig_t2 = (int16_t)(buf[3] << 8)  | (int16_t)buf[2];
    params->dig_t3 = (int16_t)(buf[5] << 8)  | (int16_t)buf[4];

    params->dig_p1 = (uint16_t)(buf[7] << 8) | (uint16_t)buf[6];
    params->dig_p2 = (int16_t)(buf[9] << 8)  | (int16_t)buf[8];
    params->dig_p3 = (int16_t)(buf[11] << 8) | (int16_t)buf[10];
    params->dig_p4 = (int16_t)(buf[13] << 8) | (int16_t)buf[12];
    params->dig_p5 = (int16_t)(buf[15] << 8) | (int16_t)buf[14];
    params->dig_p6 = (int16_t)(buf[17] << 8) | (int16_t)buf[16];
    params->dig_p7 = (int16_t)(buf[19] << 8) | (int16_t)buf[18];
    params->dig_p8 = (int16_t)(buf[21] << 8) | (int16_t)buf[20];
    params->dig_p9 = (int16_t)(buf[23] << 8) | (int16_t)buf[22];

    params->dig_h1 = (uint8_t)buf[25];

    // read 0xe1 - 0xe6
    i2c_write_blocking(i2c0, BME280_ADDR, &reg2, 1, true);
    i2c_read_blocking(i2c0, BME280_ADDR, buf, NUM_CALIB_PARAMS2, false);

    params->dig_h2 = (int16_t)(buf[1] << 8) | (int16_t)buf[0];
    params->dig_h3 = (uint8_t)buf[2];
    params->dig_h4 = (int16_t)(buf[3] << 4) | (int16_t)(buf[4] & 0x0f);
    params->dig_h5 = (int16_t)(buf[5] << 4) | (int16_t)(buf[4] >> 4 & 0x0f);
    params->dig_h6 = (int8_t)buf[6];
}

static void bme280_init(void) {
    // osrs_h x1 = 0x01
    uint8_t ctr_hum[2]  = { REG_CTRL_HUM, 0x01};
    // osrs_t x1, osrs_p x1, normal mode operation = 0x27
    uint8_t ctr_meas[2] = { REG_CTRL_MEAS, 0x27};
    // t_sb = 500, filter = 16
    uint8_t config[2]   = { REG_CONFIG, 0x90};
    i2c_write_blocking(i2c0, BME280_ADDR, ctr_hum, 2, false);
    i2c_write_blocking(i2c0, BME280_ADDR, ctr_meas, 2, false);
    i2c_write_blocking(i2c0, BME280_ADDR, config, 2, false);
}

/* タスクの実行関数*/
void prvBME280Task(void *pvParameters)
{
    // 生データ、前データ
    int32_t raw_temp, raw_pres, raw_humi;
    struct bme280_calib_param params;   // 較正パラメタ
    const TickType_t xDelay = 1000;

    (void)pvParameters;

    // 環境センサーBME280の初期化
    bme280_init();
    // 較正パラメタの取得
    bme280_get_calib_params(&params);
    vTaskDelay((TickType_t)250);    // データポーリングとレジスタ更新がぶつからないようにスリープさせる

    while (1) {
        // センサーデータの読み取り
        bme280_read_raw(&raw_temp, &raw_pres, &raw_humi);
        Measure.mes.temp = bme280_convert_temp(raw_temp, &params);
        Measure.mes.pres = bme280_convert_pressure(raw_pres, raw_temp, &params);
        Measure.mes.humi = bme280_convert_humidity(raw_humi, raw_temp, &params);
        gpio_xor_mask(1u << TASK_LED);
        xQueueSend(LCDQueue, Measure.buf, 0);
        vTaskDelay( xDelay);        // 1秒おきに測定
    }
}
