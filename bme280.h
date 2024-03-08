#ifndef APP_BME280_H
#define APP_BME280_H

/**
 * Copyright (c) 2021 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 **/

// デバイスバスアドレス
#define BME280_ADDR     (0x76)

// ピン定義
#define I2C0_SDA_PIN    (4)
#define I2C0_SCL_PIN    (5)

// ハードウェアレジスタ
#define REG_ID          (0xD0)
#define REG_CONFIG      (0xF5)
#define REG_CTRL_MEAS   (0xF4)
#define REG_CTRL_HUM    (0xF2)
#define REG_RESET       (0xE0)

#define REG_TEMP_XLSB   (0xFC)
#define REG_TEMP_LSB    (0xFB)
#define REG_TEMP_MSB    (0xFA)

#define REG_PRESSURE_XLSB   (0xF9)
#define REG_PRESSURE_LSB    (0xF8)
#define REG_PRESSURE_MSB    (0xF7)

// 較正レジスタ
#define REG_DIG_T1_LSB  (0x88)
#define REG_DIG_T1_MSB  (0x89)
#define REG_DIG_T2_LSB  (0x8A)
#define REG_DIG_T2_MSB  (0x8B)
#define REG_DIG_T3_LSB  (0x8C)
#define REG_DIG_T3_MSB  (0x8D)
#define REG_DIG_P1_LSB  (0x8E)
#define REG_DIG_P1_MSB  (0x8F)
#define REG_DIG_P2_LSB  (0x90)
#define REG_DIG_P2_MSB  (0x91)
#define REG_DIG_P3_LSB  (0x92)
#define REG_DIG_P3_MSB  (0x93)
#define REG_DIG_P4_LSB  (0x94)
#define REG_DIG_P4_MSB  (0x95)
#define REG_DIG_P5_LSB  (0x96)
#define REG_DIG_P5_MSB  (0x97)
#define REG_DIG_P6_LSB  (0x98)
#define REG_DIG_P6_MSB  (0x99)
#define REG_DIG_P7_LSB  (0x9A)
#define REG_DIG_P7_MSB  (0x9B)
#define REG_DIG_P8_LSB  (0x9C)
#define REG_DIG_P8_MSB  (0x9D)
#define REG_DIG_P9_LSB  (0x9E)
#define REG_DIG_P9_MSB  (0x9F)
#define REG_DIG_H1      (0xA1)
#define REG_DIG_H2_LSB  (0xE1)
#define REG_DIG_H2_MSB  (0xE2)
#define REG_DIG_H3      (0xE3)
#define REG_DIG_H4      (0xE4)
#define REG_DIG_H5      (0xE5)

// 読むべき較正レジスタの数
#define NUM_CALIB_PARAMS1   26
#define NUM_CALIB_PARAMS2   7

// 較正パラメタ構造体
struct bme280_calib_param {
    // 温度パラメタ
    uint16_t dig_t1;
    int16_t dig_t2;
    int16_t dig_t3;

    // 気圧パラメタ
    uint16_t dig_p1;
    int16_t dig_p2;
    int16_t dig_p3;
    int16_t dig_p4;
    int16_t dig_p5;
    int16_t dig_p6;
    int16_t dig_p7;
    int16_t dig_p8;
    int16_t dig_p9;

    uint8_t dig_h1;
    int16_t dig_h2;
    uint8_t dig_h3;
    int16_t dig_h4;
    int16_t dig_h5;
    int8_t dig_h6;
};

#endif
