// main.cpp - Q3: Handwritten Digit Recognition from Digital Images
// Board: DISCO-F746NG, Mbed OS 6

#include "mbed.h"
#include "stm32f7xx_hal.h"
#include "stm32746g_discovery_sdram.h"
#include "stm32746g_discovery_lcd.h"

#include <cstdio>
#include <chrono>

#include "digit_classifier.h"

using namespace std::chrono_literals;

// -------- Seri port --------
FileHandle *mbed::mbed_override_console(int) {
    static UnbufferedSerial pc(USBTX, USBRX, 115200);
    return &pc;
}

// -------- LCD yardımcıları --------
static void init_display() {
    BSP_SDRAM_Init();
    BSP_LCD_Init();
    BSP_LCD_LayerDefaultInit(0, 0xC0000000);
    BSP_LCD_SelectLayer(0);
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    BSP_LCD_SetTextColor(LCD_COLOR_YELLOW);
    BSP_LCD_SetFont(&Font24);
}

static void lcd_print_line(int line, const char *text) {
    BSP_LCD_DisplayStringAt(0, LINE(line), (uint8_t*)text, CENTER_MODE);
}

int main() {
    init_display();

    lcd_print_line(0, "Q3: Digit Recognition");
    lcd_print_line(1, "Image -> kNN Digits");

    printf("=== Q3: Handwritten Digit Recognition Demo ===\n");
    printf("8x8 image templates + nearest neighbor classifier.\n");

    // 10 ornek test edelim (0..9 icin)
    for (int i = 0; i < 10; ++i) {
        float img[DIGIT_IMG_SIZE];
        get_test_image(i, img);

        int pred = digit_predict(img);

        // ---- Seri port ----
        printf("\nSample %d:\n", i);
        printf("  Using template digit: %d\n", i % 10);
        printf("  Predicted digit: %d\n", pred);

        // İsteğe bağlı: imajı 8x8 print et
        printf("  Image (8x8):\n");
        for (int r = 0; r < 8; ++r) {
            printf("    ");
            for (int c = 0; c < 8; ++c) {
                float v = img[r*8 + c];
                printf("%c", (v > 0.5f) ? '#' : '.');
            }
            printf("\n");
        }

        // ---- LCD ----
        char line2[64];
        char line3[64];

        snprintf(line2, sizeof(line2), "Sample id: %d", i);
        snprintf(line3, sizeof(line3), "Predicted: %d", pred);

        lcd_print_line(3, line2);
        lcd_print_line(4, line3);

        ThisThread::sleep_for(1500ms);
    }

    lcd_print_line(6, "Q3 demo done.");

    while (true) {
        ThisThread::sleep_for(1s);
    }
}
