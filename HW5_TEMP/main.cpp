// main.cpp - Section 12.10: Estimating Future Temperature Values
// Board: DISCO-F746NG
// Mbed OS 6

#include "mbed.h"
#include "stm32746g_discovery_sdram.h"
#include "stm32746g_discovery_lcd.h"
#include <cmath>
#include <cstdio>

using namespace std::chrono_literals;

// ---------------- LCD ----------------
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

static void lcd_print(int line, const char *txt) {
    BSP_LCD_DisplayStringAt(0, LINE(line), (uint8_t*)txt, CENTER_MODE);
}

// ---------------- Model Parameters ----------------
#define INPUT_SIZE   5
#define HIDDEN_SIZE  10

// Dummy trained weights (kitaptaki export mantığına uygun)
static const float W1[HIDDEN_SIZE][INPUT_SIZE] = {
    {0.12f, 0.08f, 0.15f, 0.10f, 0.05f},
    {0.05f, 0.11f, 0.07f, 0.09f, 0.14f},
    {0.09f, 0.13f, 0.06f, 0.12f, 0.08f},
    {0.14f, 0.05f, 0.11f, 0.07f, 0.09f},
    {0.10f, 0.09f, 0.08f, 0.06f, 0.11f},
    {0.07f, 0.14f, 0.10f, 0.05f, 0.08f},
    {0.11f, 0.06f, 0.09f, 0.14f, 0.07f},
    {0.08f, 0.10f, 0.12f, 0.09f, 0.05f},
    {0.06f, 0.07f, 0.14f, 0.11f, 0.09f},
    {0.13f, 0.08f, 0.05f, 0.10f, 0.12f}
};

static const float B1[HIDDEN_SIZE] = {
    0.1f,0.1f,0.1f,0.1f,0.1f,0.1f,0.1f,0.1f,0.1f,0.1f
};

static const float W2[HIDDEN_SIZE] = {
    0.12f,0.10f,0.08f,0.11f,0.09f,
    0.07f,0.13f,0.10f,0.06f,0.14f
};

static const float B2 = 0.2f;

// ---------------- Activation ----------------
static inline float relu(float x) {
    return x > 0 ? x : 0;
}

// ---------------- Prediction ----------------
static float predict_temperature(const float *x) {
    float h[HIDDEN_SIZE];

    // Hidden layer
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        float s = B1[i];
        for (int j = 0; j < INPUT_SIZE; j++) {
            s += W1[i][j] * x[j];
        }
        h[i] = relu(s);
    }

    // Output layer (regression)
    float out = B2;
    for (int i = 0; i < HIDDEN_SIZE; i++) {
        out += W2[i] * h[i];
    }

    return out;
}

// ---------------- MAIN ----------------
int main() {
    printf("=== Section 12.10 Temperature Prediction ===\n");

    lcd_init();
    lcd_print(0, "Temp Prediction");

    // Örnek: Son 5 sicaklik (datasetten gelmis gibi)
    float input_temps[INPUT_SIZE] = {22.1f, 22.3f, 22.6f, 22.9f, 23.2f};

    float pred = predict_temperature(input_temps);

    printf("Input temps: ");
    for (int i = 0; i < INPUT_SIZE; i++) {
        printf("%.1f ", input_temps[i]);
    }
    printf("\nPredicted next temperature: %.2f\n", pred);

    char buf[32];
    snprintf(buf, sizeof(buf), "Next Temp:");
    lcd_print(3, buf);
    snprintf(buf, sizeof(buf), "%.2f C", pred);
    lcd_print(4, buf);

    while (true) {
        ThisThread::sleep_for(1s);
    }
}
