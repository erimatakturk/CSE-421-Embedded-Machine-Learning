#include "mbed.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_sdram.h"
#include <cstdio>

// ================= LCD =================
static void lcd_init() {
    BSP_SDRAM_Init();
    BSP_LCD_Init();
    BSP_LCD_LayerDefaultInit(0, 0xC0000000);
    BSP_LCD_SelectLayer(0);

    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
    BSP_LCD_SetFont(&Font20);
}

static void lcd_print(int line, const char* txt) {
    BSP_LCD_DisplayStringAt(0, LINE(line),
                            (uint8_t*)txt, CENTER_MODE);
}

// ================= Digit labels =================
static const char* DIGITS[10] = {
    "0","1","2","3","4","5","6","7","8","9"
};

// ================= Fake image =================
static uint8_t sample_digit[28*28];

// ================= Very light classifier =================
static int digit_predict() {
    int sum = 0;
    for(int i=0;i<28*28;i++) sum += sample_digit[i];

    if(sum < 200) return 1;
    if(sum < 400) return 3;
    if(sum < 600) return 7;
    return 8;
}

// ================= MAIN =================
int main() {
    printf("=== Section 12.9 Digit Recognition ===\n");

    lcd_init();

    lcd_print(1, "Section 12.9");
    lcd_print(2, "Digit Recognition");

    // Fake digit pattern
    for(int i=0;i<28*28;i++) {
        sample_digit[i] = (i % 4 == 0) ? 1 : 0;
    }

    int pred = digit_predict();

    printf("Predicted digit: %s\n", DIGITS[pred]);

    char buf[32];
    snprintf(buf, sizeof(buf), "Predicted: %s", DIGITS[pred]);
    lcd_print(4, buf);

    while(true) {
        ThisThread::sleep_for(1s);
    }
}
