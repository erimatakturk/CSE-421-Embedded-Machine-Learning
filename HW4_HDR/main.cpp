#include "mbed.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_sdram.h"
#include <cmath>
#include <cstdio>

using namespace std::chrono_literals;

/* ---------------- LCD ---------------- */
static void lcd_init() {
    BSP_SDRAM_Init();
    BSP_LCD_Init();
    BSP_LCD_LayerDefaultInit(0, 0xC0000000);
    BSP_LCD_SelectLayer(0);
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
    BSP_LCD_SetFont(&Font24);
}

static void lcd_print(int line, const char *txt) {
    BSP_LCD_DisplayStringAt(0, LINE(line), (uint8_t*)txt, CENTER_MODE);
}

/* ---------------- MODEL PARAMETERS ---------------- */
#define INPUT_SIZE   7
#define HIDDEN1      100
#define HIDDEN2      100
#define OUTPUT_SIZE  10

static const float W1[HIDDEN1][INPUT_SIZE] = {{0.01f}};
static const float B1[HIDDEN1] = {0};

static const float W2[HIDDEN2][HIDDEN1] = {{0.01f}};
static const float B2[HIDDEN2] = {0};

static const float W3[OUTPUT_SIZE][HIDDEN2] = {{0.01f}};
static const float B3[OUTPUT_SIZE] = {0};

/* ---------------- ACTIVATIONS ---------------- */
static float relu(float x) {
    return x > 0 ? x : 0;
}

static void softmax(float *x, int n) {
    float maxv = x[0];
    for (int i = 1; i < n; i++) if (x[i] > maxv) maxv = x[i];

    float sum = 0;
    for (int i = 0; i < n; i++) {
        x[i] = expf(x[i] - maxv);
        sum += x[i];
    }
    for (int i = 0; i < n; i++) x[i] /= sum;
}

/* ---------------- MLP FORWARD ---------------- */
static int predict_digit(float input[INPUT_SIZE]) {
    static float h1[HIDDEN1];
    static float h2[HIDDEN2];
    static float out[OUTPUT_SIZE];

    for (int i = 0; i < HIDDEN1; i++) {
        float s = B1[i];
        for (int j = 0; j < INPUT_SIZE; j++) s += W1[i][j] * input[j];
        h1[i] = relu(s);
    }

    for (int i = 0; i < HIDDEN2; i++) {
        float s = B2[i];
        for (int j = 0; j < HIDDEN1; j++) s += W2[i][j] * h1[j];
        h2[i] = relu(s);
    }

    for (int i = 0; i < OUTPUT_SIZE; i++) {
        float s = B3[i];
        for (int j = 0; j < HIDDEN2; j++) s += W3[i][j] * h2[j];
        out[i] = s;
    }

    softmax(out, OUTPUT_SIZE);

    int best = 0;
    for (int i = 1; i < OUTPUT_SIZE; i++)
        if (out[i] > out[best]) best = i;

    return best;
}

/* ---------------- MAIN ---------------- */
int main() {
    lcd_init();

    lcd_print(0, "Q3 - Digit Recognition");
    printf("Q3 - Handwritten Digit Recognition\n");

    float hu_features[7] = {
        0.21f, 0.03f, 0.001f,
        0.0001f, 0.00001f,
        0.000001f, 0.0000001f
    };

    int predicted = predict_digit(hu_features);

    char line[32];
    sprintf(line, "Predicted Digit: %d", predicted);

    lcd_print(4, line);
    printf("Predicted digit = %d\n", predicted);

    while (true) {
        ThisThread::sleep_for(1s);
    }
}
