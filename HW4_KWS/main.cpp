#include "mbed.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_sdram.h"
#include <cmath>

// ---------- LCD ----------
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

static void lcd_print(int line, const char* txt) {
    BSP_LCD_DisplayStringAt(0, LINE(line), (uint8_t*)txt, CENTER_MODE);
}

// ---------- Labels ----------
static const char* KWS_LABELS[10] = {
    "zero","one","two","three","four",
    "five","six","seven","eight","nine"
};

// ---------- Sample MFCC ----------
static const float SAMPLE_MFCC[26] = {
    -0.12f, 0.33f, 0.18f, -0.44f, 0.21f, 0.09f,
     0.14f, -0.27f, 0.31f, 0.05f, -0.08f, 0.19f,
     0.22f, -0.11f, 0.07f, 0.04f, -0.03f, 0.16f,
     0.09f, 0.02f, -0.06f, 0.08f, 0.11f, -0.09f,
     0.05f, 0.01f
};

// ---------- Weights ----------
static float W1[100][26];
static float W2[100][100];
static float W3[10][100];
static float B1[100], B2[100], B3[10];

// ---------- Buffers (STATIC = SAFE) ----------
static float h1[100];
static float h2[100];
static float out[10];

// ---------- Activation ----------
static inline float relu(float x) { return x > 0 ? x : 0; }

static void softmax(float* x, int n) {
    float m = x[0];
    for(int i=1;i<n;i++) if(x[i]>m) m=x[i];
    float s=0;
    for(int i=0;i<n;i++){ x[i]=expf(x[i]-m); s+=x[i]; }
    for(int i=0;i<n;i++) x[i]/=s;
}

// ---------- Init weights ----------
static void init_weights() {
    for(int i=0;i<100;i++){
        B1[i]=0.01f; B2[i]=0.01f;
        for(int j=0;j<26;j++) W1[i][j]=0.01f;
        for(int j=0;j<100;j++) W2[i][j]=0.01f;
    }
    for(int i=0;i<10;i++){
        B3[i]=0.01f;
        for(int j=0;j<100;j++) W3[i][j]=0.01f;
    }
}

// ---------- Predict ----------
static int kws_predict(const float* x) {
    for(int i=0;i<100;i++){
        float s=B1[i];
        for(int j=0;j<26;j++) s+=W1[i][j]*x[j];
        h1[i]=relu(s);
    }

    for(int i=0;i<100;i++){
        float s=B2[i];
        for(int j=0;j<100;j++) s+=W2[i][j]*h1[j];
        h2[i]=relu(s);
    }

    for(int i=0;i<10;i++){
        float s=B3[i];
        for(int j=0;j<100;j++) s+=W3[i][j]*h2[j];
        out[i]=s;
    }

    softmax(out,10);

    int best=0;
    for(int i=1;i<10;i++) if(out[i]>out[best]) best=i;
    return best;
}

// ---------- MAIN ----------
int main() {
    printf("Q2 - Keyword Spotting (Section 11.7)\n");

    lcd_init();
    lcd_print(1,"Q2 Keyword Spotting");

    init_weights();

    int pred = kws_predict(SAMPLE_MFCC);

    printf("Predicted keyword: %s\n", KWS_LABELS[pred]);

    char buf[32];
    snprintf(buf,sizeof(buf),"Pred: %s",KWS_LABELS[pred]);
    lcd_print(3, buf);

    while(true) {
        ThisThread::sleep_for(1s);
    }
}