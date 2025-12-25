#include "mbed.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_sdram.h"
#include <cmath>
#include <cstdio>

using namespace std::chrono_literals;

/* ================= Serial ================= */
FileHandle *mbed::mbed_override_console(int) {
    static UnbufferedSerial pc(USBTX, USBRX, 115200);
    return &pc;
}

/* ================= LCD ================= */
void lcd_init() {
    BSP_SDRAM_Init();
    BSP_LCD_Init();
    BSP_LCD_LayerDefaultInit(0, 0xC0000000);
    BSP_LCD_SelectLayer(0);
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    BSP_LCD_SetFont(&Font24);
}

void lcd_print(int line, const char *txt) {
    BSP_LCD_DisplayStringAt(0, LINE(line), (uint8_t*)txt, CENTER_MODE);
}

/* ================= MLP MODEL ================= */

// Input size = 5
// Hidden1 = 10
// Hidden2 = 10
// Output = 1

float W1[10][5] = {
 {0.12, -0.05, 0.08, 0.10, -0.02},
 {0.01, 0.09, -0.04, 0.07, 0.03},
 {0.03, -0.02, 0.06, 0.01, 0.08},
 {0.05, 0.02, 0.04, -0.06, 0.01},
 {0.07, 0.03, 0.02, 0.05, -0.01},
 {0.02, -0.01, 0.03, 0.04, 0.06},
 {0.04, 0.06, -0.02, 0.01, 0.03},
 {0.01, 0.02, 0.05, 0.06, 0.04},
 {0.03, -0.04, 0.01, 0.02, 0.05},
 {0.06, 0.01, 0.03, -0.02, 0.04}
};

float b1[10] = {0.01,0.02,0.01,0.03,0.02,0.01,0.02,0.01,0.03,0.02};

float W2[10][10];
float b2[10] = {0};

float W3[1][10] = {{0.2,0.1,0.15,0.05,0.1,0.08,0.12,0.07,0.09,0.11}};
float b3[1] = {0.5};

/* ================= Utils ================= */
float relu(float x) { return x > 0 ? x : 0; }

/* ================= Inference ================= */
float predict_temperature(float input[5]) {

    float h1[10], h2[10];

    // Layer 1
    for(int i=0;i<10;i++){
        h1[i] = b1[i];
        for(int j=0;j<5;j++)
            h1[i] += W1[i][j] * input[j];
        h1[i] = relu(h1[i]);
    }

    // Layer 2
    for(int i=0;i<10;i++){
        h2[i] = b2[i];
        for(int j=0;j<10;j++)
            h2[i] += W2[i][j] * h1[j];
        h2[i] = relu(h2[i]);
    }

    // Output
    float out = b3[0];
    for(int i=0;i<10;i++)
        out += W3[0][i] * h2[i];

    return out;
}

/* ================= MAIN ================= */
int main() {

    lcd_init();
    lcd_print(1, "11.9 Temp Prediction");

    printf("=== 11.9 Future Temperature Prediction ===\n");

    float lastTemps[5] = {22.1, 22.4, 22.8, 23.0, 23.3};

    float predicted = predict_temperature(lastTemps);

    printf("Input temps: ");
    for(int i=0;i<5;i++) printf("%.2f ", lastTemps[i]);
    printf("\nPredicted next temp: %.2f C\n", predicted);

    char line1[64], line2[64];
    snprintf(line1, sizeof(line1), "Last: %.1f %.1f %.1f",
             lastTemps[2], lastTemps[3], lastTemps[4]);
    snprintf(line2, sizeof(line2), "Next: %.2f C", predicted);

    lcd_print(4, line1);
    lcd_print(6, line2);

    while(true) {
        ThisThread::sleep_for(1s);
    }
}
