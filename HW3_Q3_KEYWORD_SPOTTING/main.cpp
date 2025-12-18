
#include "mbed.h"
#include "stm32f7xx_hal.h"
#include "stm32746g_discovery_sdram.h"
#include "stm32746g_discovery_lcd.h"

#include <cmath>
#include <cstdio>
#include <chrono>

#include "sample_audio.h"
#include "kws_model.h"

using namespace std::chrono_literals;

// ---------------- Serial ----------------
FileHandle *mbed::mbed_override_console(int) {
    static UnbufferedSerial pc(USBTX, USBRX, 115200);
    return &pc;
}

// ---------------- LCD helper ----------------
static void lcd_print(int line, const char *txt) {
    BSP_LCD_DisplayStringAt(0, LINE(line), (uint8_t*)txt, CENTER_MODE);
}

// ---------------- SAFE DISPLAY INIT ----------------
static void init_display_safe() {

    // 1️⃣ SDRAM FIRST (ZORUNLU)
    if (BSP_SDRAM_Init() != SDRAM_OK) {
        printf("SDRAM init failed!\n");
        while (true);
    }

    // 2️⃣ LCD
    BSP_LCD_Init();

    // 3️⃣ Framebuffer SDRAM adresinde
    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
    BSP_LCD_SelectLayer(0);

    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
    BSP_LCD_SetFont(&Font20);
}

// ---------------- MAIN ----------------
int main() {

    printf("System booting...\n");
    ThisThread::sleep_for(500ms);

    // LCD + SDRAM güvenli init
    init_display_safe();

    lcd_print(0, "Q2: Keyword Spotting");

    printf("Q2 - Keyword Spotting (Section 10.8)\n");

    // 1) MFCC
    float mfcc[5];
    compute_mfcc(AUDIO_FRAME, AUDIO_FRAME_SIZE, mfcc);

    printf("MFCC features:\n");
    for (int i = 0; i < 5; i++) {
        printf("c%d = %.3f\n", i, mfcc[i]);
    }

    // 2) Prediction
    int pred = keyword_predict(mfcc);

    if (pred) {
        printf("Keyword DETECTED\n");
        lcd_print(2, "Keyword: YES");
    } else {
        printf("Keyword NOT detected\n");
        lcd_print(2, "Keyword: NO");
    }

    while (true) {
        ThisThread::sleep_for(1s);
    }
}
