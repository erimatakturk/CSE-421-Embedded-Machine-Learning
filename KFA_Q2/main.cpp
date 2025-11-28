#include "mbed.h"
#include "stm32f7xx_hal.h"
#include "stm32746g_discovery_sdram.h"
#include "stm32746g_discovery_lcd.h"

#include <cstdio>
#include <chrono>

#include "knn_mfcc.h"

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
    BSP_LCD_SetTextColor(LCD_COLOR_CYAN);
    BSP_LCD_SetFont(&Font24);
}

static void lcd_print_line(int line, const char *text) {
    BSP_LCD_DisplayStringAt(0, LINE(line), (uint8_t*)text, CENTER_MODE);
}

// -------- Keyword label tablosu --------
// Burada 10 sınıf varsayıyorum; istersen kitabın Q2'sine göre değiştirebiliriz.
static const char* KWS_LABELS[10] = {
    "zero",   // 0
    "one",    // 1
    "two",    // 2
    "three",  // 3
    "four",   // 4
    "five",   // 5
    "six",    // 6
    "seven",  // 7
    "eight",  // 8
    "nine"    // 9
};

int main() {
    init_display();

    lcd_print_line(0, "Q2: Keyword Spotting");
    lcd_print_line(1, "MFCC + kNN (Demo)");

    printf("=== Q2: Keyword Spotting Demo (MFCC + kNN iskeleti) ===\n");

    // Burada örnek olarak 0..9 arasi 10 test sample kullaniyoruz.
    // İleride buraya gerçek MFCC feature indexlerini koyabilirsin.
    for (int i = 0; i < 10; ++i) {
        int pred = knn_predict(i);      // su an: idx % 10
        int cls = pred % 10;            // güvenlik: 0..9'a indir

        const char* label = KWS_LABELS[cls];

        // ---- Seri port çıktısı ----
        printf("Sample %d -> predicted class id = %d, keyword = %s\n",
               i, cls, label);

        // ---- LCD çıktısı ----
        char line2[64];
        char line3[64];

        char lineTop[64];
        snprintf(lineTop, sizeof(lineTop), "Sample id: %d", i);
        lcd_print_line(3, lineTop);

        snprintf(line2, sizeof(line2), "Pred id: %d", cls);
        snprintf(line3, sizeof(line3), "Keyword: %s", label);

        lcd_print_line(4, line2);
        lcd_print_line(5, line3);

        ThisThread::sleep_for(1500ms);
    }

    // Son durumda ekranda son örneği sabit bırak
    lcd_print_line(7, "Q2 demo done.");

    // Programı sonlandırmak yerine sonsuza kadar uyut
    while (true) {
        ThisThread::sleep_for(1s);
    }
}
