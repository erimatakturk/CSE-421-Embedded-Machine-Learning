#include "mbed.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_sdram.h"
#include <cmath>
#include <cstring>

// ---------- Serial ----------
FileHandle *mbed::mbed_override_console(int) {
    static UnbufferedSerial pc(USBTX, USBRX, 115200);
    return &pc;
}

// ---------- LCD ----------
void lcd_init() {
    BSP_SDRAM_Init();
    BSP_LCD_Init();
    BSP_LCD_LayerDefaultInit(0, 0xC0000000);
    BSP_LCD_SelectLayer(0);
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
    BSP_LCD_SetFont(&Font24);
}

void lcd_print(int line, const char *txt) {
    BSP_LCD_DisplayStringAt(0, LINE(line), (uint8_t*)txt, CENTER_MODE);
}

// ---------- HAR labels ----------
static const char* LABELS[6] = {
    "Walking",
    "Jogging",
    "Upstairs",
    "Downstairs",
    "Sitting",
    "Standing"
};

// ---------- MLP dimensions ----------
#define INPUT_DIM   10
#define HIDDEN_DIM  32
#define OUTPUT_DIM  6


static float W1[HIDDEN_DIM][INPUT_DIM] = {
    {0.12f,-0.08f,0.03f,0.04f,0.11f,0.02f,0.09f,-0.01f,0.05f,0.07f},
};
static float B1[HIDDEN_DIM] = {0};

static float W2[OUTPUT_DIM][HIDDEN_DIM] = {
    {0.10f,-0.05f,0.02f},
};
static float B2[OUTPUT_DIM] = {0};

// ---------- Math helpers ----------
float relu(float x) {
    return x > 0 ? x : 0;
}

void softmax(float *x, int n) {
    float maxv = x[0];
    for (int i = 1; i < n; i++) if (x[i] > maxv) maxv = x[i];

    float sum = 0;
    for (int i = 0; i < n; i++) {
        x[i] = expf(x[i] - maxv);
        sum += x[i];
    }
    for (int i = 0; i < n; i++) x[i] /= sum;
}

int mlp_predict(float *input) {
    float hidden[HIDDEN_DIM] = {0};
    float output[OUTPUT_DIM] = {0};

    // Layer 1
    for (int i = 0; i < HIDDEN_DIM; i++) {
        for (int j = 0; j < INPUT_DIM; j++)
            hidden[i] += W1[i][j] * input[j];
        hidden[i] = relu(hidden[i] + B1[i]);
    }

    // Output layer
    for (int i = 0; i < OUTPUT_DIM; i++) {
        for (int j = 0; j < HIDDEN_DIM; j++)
            output[i] += W2[i][j] * hidden[j];
        output[i] += B2[i];
    }

    softmax(output, OUTPUT_DIM);

    int best = 0;
    for (int i = 1; i < OUTPUT_DIM; i++)
        if (output[i] > output[best]) best = i;

    return best;
}

int main() {
    lcd_init();

    lcd_print(1, "HW4 - Q1");
    lcd_print(3, "HAR (11.6)");

    printf("=== Section 11.6 HAR ===\n");
    
    float features[INPUT_DIM] = {
        0.02f, 0.98f, 0.10f,
        0.15f, 0.12f, 0.08f,
        1.02f, 0.20f,
        1.30f, 0.85f
    };

    int pred = mlp_predict(features);

    printf("Predicted Activity: %s\n", LABELS[pred]);

    char buf[40];
    snprintf(buf, sizeof(buf), "Activity:");
    lcd_print(6, buf);
    lcd_print(7, LABELS[pred]);

    while (true) {
        ThisThread::sleep_for(1s);
    }
}
