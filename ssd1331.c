/*
 *    OLED制御タスク
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "ssd1331.h"
#include "font.h"

#define WAIT_MS     ( 1000 / portTICK_PERIOD_MS )

// 1行表示の文字列と表示ピクセル: 1行12文字, 1文字8x8ピクセル
char vbuf[12];
static uint16_t lpxs[SSD1331_LBUF_LEN];

// キューハンドル定義
extern QueueHandle_t  LCDQueue;
/** キュー受信用バッファ定義 ***/
union {
    uint8_t buf[16];
    struct {
        int32_t data1;
        int32_t data2;
        int32_t data3;
    } mes;
} Disp;

static void reset(void) {
    RESET0_OFF;
    CS0_DESELECT;
    sleep_ms(5);
    RESET0_ON;
    sleep_ms(80);
    RESET0_OFF;
    sleep_ms(20);
}

// TODO: unit番号を得る
static void send_cmd(uint8_t cmd) {
    spi_set_format(spi0, 8, SPI_CPOL_0,  SPI_CPHA_0, SPI_MSB_FIRST);
    CS0_SELECT;   // チップセレクト
    DC0_COMMAND;   // コマンドモード
    spi_write_blocking(spi0, &cmd, 1);
    DC0_DATA;
    CS0_DESELECT;   // 連続出力の場合は必須
}

static void send_cmd_list(uint8_t *buf, size_t size) {
    for (size_t i = 0; i < size; i++) {
        send_cmd(buf[i]);
    }
}

static void send_data(uint16_t *buf, size_t size) {
    int ret;

    spi_set_format(spi0, 16, SPI_CPOL_0,  SPI_CPHA_0, SPI_MSB_FIRST);
    CS0_SELECT;   // チップセレクト
    DC0_DATA;   // データモード
    ret = spi_write16_blocking(spi0, buf, size);
    DC0_COMMAND;
    CS0_DESELECT;   // 連続出力の場合は必須
    printf("sd: %d\n", ret);
}

static void ssd1331_init(void) {
    // ssd1331をリセット
    reset();

    // デフォルト値で初期化: Adafruit-SSD1331-OLED-Driver-Library-for-Arduinoから引用
    uint8_t cmds[] = {
        SSD1331_SET_DISP_OFF,
        SSD1331_SET_ROW_ADDR, 0x00, 0x3F,
        SSD1331_SET_COL_ADDR, 0x00, 0x5F,
        SSD1331_REMAP_COLOR_DEPTH, 0x72,
        SSD1331_SET_DISP_START_LINE, 0x00,
        SSD1331_SET_DISP_OFFSET, 0x00,
        SSD1331_SET_NORM_DISP,
        SSD1331_SET_MUX_RATIO, 0x3F,
        SSD1331_SET_MASTER_CONFIG, 0xBE,
        SSD1331_POWER_SAVE, 0x0B,
        SSD1331_ADJUST, 0x74,
        SSD1331_DISP_CLOck, 0xF0,
        SSD1331_SET_PRECHARGE_A, 0x64,
        SSD1331_SET_PRECHARGE_B, 0x78,
        SSD1331_SET_PRECHARGE_C, 0x64,
        SSD1331_SET_PRECHARGE_LEVEL, 0x3A,
        SSD1331_SET_VCOMH, 0x3E,
        SSD1331_MASTER_CURRENT_CNTL, 0x06,
        SSD1331_SET_CONTRAST_A, 0x91,
        SSD1331_SET_CONTRAST_B, 0x50,
        SSD1331_SET_CONTRAST_C, 0x7D,
        SSD1331_SET_DISP_ON_NORM
    };
    send_cmd_list(cmds, count_of(cmds));
}

static void set_char(int x, int y, uint8_t ch, uint16_t color) {
    if (x > SSD1331_WIDTH - 8 || y > SSD1331_HEIGHT - 8)
        return;

    uint8_t b, c, p;
    int pos;

    ch = to_upper(ch);
    int idx = get_font_index(ch);    // 文字のfont[]内の行数
    for (int i = 0; i < 8; i++) {
        c = font[idx * 8 + i];
        p = 0x80;
        for (int k = 0; k < 8; k++) {
            b = (c & p);
            p >>= 1;
            pos = (y + i) * SSD1331_WIDTH + x + k;
            lpxs[pos] = (b ? color : COL_BACK);
        }
    }
}

// y行のx桁から文字列strを書き込む
static void write_line(int y, char *str, uint16_t color) {
    // Cull out any string off the screen
    if (y > SSD1331_HEIGHT - 8)
        return;

    memset(lpxs, 0, 2 * SSD1331_LBUF_LEN);

    for (int i = 0; i < 12; i++) {
        set_char(i * 8, 0, *(str+i), color);
    }
    printf("wl: %s\n", str);
    uint8_t cmds[] = {
        SSD1331_SET_COL_ADDR, 0,  95,
        SSD1331_SET_ROW_ADDR, y, y + 7
    };
    printf("wl: send cmd\n");
    send_cmd_list(cmds, 6);
    printf("wl: send data\n");
    send_data(lpxs, SSD1331_LBUF_LEN);
}

static void clear_screen(uint16_t color) {
    uint8_t cmds[] = {
        SSD1331_SET_COL_ADDR, 0, SSD1331_WIDTH - 1,
        SSD1331_SET_ROW_ADDR, 0, SSD1331_HEIGHT - 1
    };

    send_cmd_list(cmds, count_of(cmds));
    sprintf(vbuf, "%s", "           ");
    for (int i = 0; i < SSD1331_WIDTH / 8; i++) {
        write_line(i*8, vbuf, color);
        //send_data(lpxs, SSD1331_LBUF_LEN);
    }
}

/* タスクの実行関数*/
void prvOLEDTask( void *pvParameters )
{
    (void)pvParameters;

    // SSD1331の初期化
    ssd1331_init();
    // 画面の消去
    clear_screen(COL_BACK);

    // タイトルを表示
    sprintf(vbuf, " %s", "ENV_DATA");
    write_line(0, vbuf, COL_FRONT);

    while (1) {
        /** キューから取り出し **/
        xQueueReceive(LCDQueue, Disp.buf, portMAX_DELAY);   // 永久待ち
        // 気圧を表示
        sprintf(vbuf, "P: %7.2f", Disp.mes.data1 / 100.0);
        write_line(10, vbuf, COL_FRONT);
        // 温度を表示
        sprintf(vbuf, "T: %7.2f", Disp.mes.data2 / 100.0);
        write_line(20, vbuf, COL_FRONT);
        // 湿度を表示
        sprintf(vbuf, "H: %7.2f", Disp.mes.data3 / 1024.0 );
        write_line(30, vbuf, COL_FRONT);
    }
}
