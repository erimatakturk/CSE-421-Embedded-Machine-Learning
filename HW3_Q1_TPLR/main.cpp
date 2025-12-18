
#include "mbed.h"
#include "stm32f7xx_hal.h"
#include "stm32746g_discovery_sdram.h"
#include "stm32746g_discovery_lcd.h"

#include <cstdio>
#include <cstdlib>
#include <chrono>

#include "sample_temperature.h"
#include "temp_model.h"

using namespace std::chrono_literals;

FileHandle *mbed::mbed_override_console(int) {
    static UnbufferedSerial pc(USBTX, USBRX, 115200);
    return &pc;
}

static void lcd_print_line(int line, const char *text) {
    BSP_LCD_DisplayStringAt(0, LINE(line), (uint8_t*)text, CENTER_MODE);
}

static void init_display_safe() {

    // 1) SDRAM MUST be initialized first
    if (BSP_SDRAM_Init() != SDRAM_OK) {
        printf("ERROR: SDRAM init failed!\n");
        while (true);
    }

    // 2) LCD init
    BSP_LCD_Init();

    // 3) Framebuffer on SDRAM
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

    // LCD + SDRAM safe init
    init_display_safe();

    lcd_print_line(0, "Q1: Temp Prediction");
    lcd_print_line(1, "Linear Regression");

    printf("Q1 - Estimating Future Temperature Values\n");
    printf("----------------------------------------\n");

    // Rastgele bir örnek seç
    int idx = rand() % TEMP_DB_SIZE;

    // Tahmin
    float predicted = predict_temperature(TEMP_DB[idx]);
    float actual    = TEMP_NEXT[idx];

    // Serial çıktı
    printf("Selected sample index: %d\n", idx);
    for (int i = 0; i < 5; i++) {
        printf("T(t-%d) = %.2f\n", i+1, TEMP_DB[idx][i]);
    }
    printf("Predicted Temp = %.2f C\n", predicted);
    printf("Actual Temp    = %.2f C\n", actual);

    // LCD çıktı
    char line1[40];
    char line2[40];

    snprintf(line1, sizeof(line1), "Pred: %.2f C", predicted);
    snprintf(line2, sizeof(line2), "Real: %.2f C", actual);

    lcd_print_line(3, line1);
    lcd_print_line(4, line2);

    // Sonsuz döngü
    while (true) {
        ThisThread::sleep_for(1s);
    }
}