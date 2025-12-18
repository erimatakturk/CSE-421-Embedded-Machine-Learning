// main.cpp
// Q3 - Section 10.9: Handwritten Digit Recognition
// Board: DISCO-F746NG

#include "mbed.h"
#include "stm32f7xx_hal.h"
#include "stm32746g_discovery_sdram.h"
#include "stm32746g_discovery_lcd.h"

#include <cstdio>
#include <cstdlib>
#include <chrono>

#include "sample_digits.h"
#include "digit_model.h"

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

    if (BSP_SDRAM_Init() != SDRAM_OK) {
        printf("SDRAM init failed!\n");
        while (true);
    }

    BSP_LCD_Init();
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

    init_display_safe();

    lcd_print(0, "Q3: Digit Recognition");

    printf("Q3 - Handwritten Digit Recognition (Section 10.9)\n");

    // Rastgele bir örnek seç
    int idx = rand() % DIGIT_DB_SIZE;
    const DigitSample &s = DIGIT_DB[idx];

    printf("Selected sample index: %d\n", idx);
    printf("Hu moments:\n");
    for (int i = 0; i < 7; i++) {
        printf("h%d = %.3f\n", i, s.hu[i]);
    }

    int pred = digit_predict(s.hu);

    printf("True label: %s\n", s.label == 0 ? "Zero" : "Not Zero");
    printf("Predicted : %s\n", pred == 0 ? "Zero" : "Not Zero");

    char l1[40];
    char l2[40];
    snprintf(l1, sizeof(l1), "True: %s", s.label == 0 ? "Zero" : "Not Zero");
    snprintf(l2, sizeof(l2), "Pred: %s", pred == 0 ? "Zero" : "Not Zero");

    lcd_print(2, l1);
    lcd_print(3, l2);

    while (true) {
        ThisThread::sleep_for(1s);
    }
}
