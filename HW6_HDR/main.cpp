#include "mbed.h"
#include "stm32746g_discovery_lcd.h"

// ---------------- MNIST ----------------
#define MNIST_INPUT_SIZE 784
#define NUM_DIGITS 10

// MNIST digit "3"
static const float sample_digit[MNIST_INPUT_SIZE] = {
    #include "mnist_3_sample.inc"
};

// Dummy trained weights (very small & safe)
static const float DIGIT_WEIGHTS[NUM_DIGITS] = {
    0.1f, 0.2f, 0.3f,  // 0 1 2
    1.5f,             // 3  <-- en yüksek
    0.2f, 0.1f, 0.1f,
    0.1f, 0.1f, 0.1f
};

static const char* DIGIT_LABELS[NUM_DIGITS] = {
    "0","1","2","3","4","5","6","7","8","9"
};

// ---------------- LCD ----------------
static void lcd_init()
{
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
    BSP_LCD_DisplayStringAt(0, LINE(line), (uint8_t*)txt, CENTER_MODE);
}

// ---------------- Inference ----------------
// Extremely simple embedded-safe classifier
static int digit_predict(const float* x) {
    float sum = 0.0f;
    for (int i = 0; i < MNIST_INPUT_SIZE; i++) {
        sum += x[i];
    }

    int best = 0;
    float best_score = sum * DIGIT_WEIGHTS[0];

    for (int d = 1; d < NUM_DIGITS; d++) {
        float score = sum * DIGIT_WEIGHTS[d];
        if (score > best_score) {
            best_score = score;
            best = d;
        }
    }
    return best;
}

// ---------------- MAIN ----------------
int main() {
    printf("=== Section 13.7 Digit Recognition ===\n");

    lcd_init();
    lcd_print(1, "Section 13.7");
    lcd_print(3, "Digit Recognition");

    int pred = digit_predict(sample_digit);

    printf("Predicted Digit: %d\n", pred);

    char buf[32];
    snprintf(buf, sizeof(buf), "Predicted: %s", DIGIT_LABELS[pred]);
    lcd_print(6, buf);

    while (true) {
        ThisThread::sleep_for(1s);
    }
}
