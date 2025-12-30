#include "mbed.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_sdram.h"
#include <cmath>

// ================= LCD =================
static void lcd_init() {
    BSP_SDRAM_Init();
    BSP_LCD_Init();
    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
    BSP_LCD_SelectLayer(0);
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
    BSP_LCD_SetFont(&Font20);
}

static void lcd_print(int line, const char* txt) {
    BSP_LCD_DisplayStringAt(0, LINE(line), (uint8_t*)txt, CENTER_MODE);
}

// ================= LABELS =================
static const char* KWS_LABELS[10] = {
    "zero","one","two","three","four",
    "five","six","seven","eight","nine"
};

// ================= SAMPLE MFCC =================
static const float SAMPLE_MFCC[26] = {
    -0.12f, 0.33f, 0.18f, -0.44f, 0.21f, 0.09f,
     0.14f,-0.27f, 0.31f, 0.05f,-0.08f, 0.19f,
     0.22f,-0.11f, 0.07f, 0.04f,-0.03f, 0.16f,
     0.09f, 0.02f,-0.06f, 0.08f, 0.11f,-0.09f,
     0.05f, 0.01f
};

// ================= SMALL NN =================
// 26 -> 10 -> 10

static const float W1[10][26] = {{0.01f}};
static const float B1[10] = {0};

static const float W2[10][10] = {{0.01f}};
static const float B2[10] = {0};

static inline float relu(float x) { return x > 0 ? x : 0; }

static void softmax(float* x, int n) {
    float m = x[0];
    for(int i=1;i<n;i++) if(x[i]>m) m=x[i];
    float s=0;
    for(int i=0;i<n;i++){ x[i]=expf(x[i]-m); s+=x[i]; }
    for(int i=0;i<n;i++) x[i]/=s;
}

static int kws_predict(const float* x) {
    float h[10], out[10];

    for(int i=0;i<10;i++){
        float s=B1[i];
        for(int j=0;j<26;j++) s+=W1[i][j]*x[j];
        h[i]=relu(s);
    }

    for(int i=0;i<10;i++){
        float s=B2[i];
        for(int j=0;j<10;j++) s+=W2[i][j]*h[j];
        out[i]=s;
    }

    softmax(out,10);

    int best=0;
    for(int i=1;i<10;i++) if(out[i]>out[best]) best=i;
    return best;
}

// ================= MAIN =================
int main() {
    printf("=== Section 12.8 Keyword Spotting ===\n");

    lcd_init();
    lcd_print(1, "12.8 Keyword Spotting");

    int pred = kws_predict(SAMPLE_MFCC);

    printf("Predicted keyword: %s\n", KWS_LABELS[pred]);

    char buf[32];
    snprintf(buf,sizeof(buf),"Pred: %s",KWS_LABELS[pred]);
    lcd_print(3, buf);

    while(true) {
        ThisThread::sleep_for(1s);
    }
}
