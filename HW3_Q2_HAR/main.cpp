
#include "mbed.h"
#include "stm32f7xx_hal.h"
#include "stm32746g_discovery_sdram.h"
#include "stm32746g_discovery_lcd.h"

#include <cstdio>
#include <cstdlib>
#include <chrono>

#include "sample_accel.h"
#include "har_model.h"

using namespace std::chrono_literals;

// Serial
FileHandle *mbed::mbed_override_console(int) {
    static UnbufferedSerial pc(USBTX, USBRX, 115200);
    return &pc;
}

// LCD helper
static void lcd_print(int line, const char *txt) {
    BSP_LCD_DisplayStringAt(0, LINE(line), (uint8_t*)txt, CENTER_MODE);
}

// Safe LCD init
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

int main() {

    printf("System booting...\n");
    ThisThread::sleep_for(500ms);

    init_display_safe();

    lcd_print(0, "Q4: HAR (10.7)");

    printf("Q4 - Human Activity Recognition (Section 10.7)\n");

    // Basit pencere: ilk 3 örnek
    float ax[3], ay[3], az[3];
    for (int i = 0; i < 3; i++) {
        ax[i] = ACCEL_DB[i].ax;
        ay[i] = ACCEL_DB[i].ay;
        az[i] = ACCEL_DB[i].az;
    }

    float feat[4];
    extract_features(ax, ay, az, 3, feat);

    int pred = har_predict(feat);
    int true_label = ACCEL_DB[0].label;

    printf("True label: %s\n", true_label == 0 ? "Walking" : "Jogging");
    printf("Predicted : %s\n", pred == 0 ? "Walking" : "Jogging");

    char l1[40], l2[40];
    snprintf(l1, sizeof(l1), "True: %s", true_label == 0 ? "Walking" : "Jogging");
    snprintf(l2, sizeof(l2), "Pred: %s", pred == 0 ? "Walking" : "Jogging");

    lcd_print(2, l1);
    lcd_print(3, l2);

    while (true) {
        ThisThread::sleep_for(1s);
    }
}
