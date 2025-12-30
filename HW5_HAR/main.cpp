#include "mbed.h"
#include "stm32746g_discovery_sdram.h"
#include "stm32746g_discovery_lcd.h"
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
static const char* LABELS[] = {
    "Walking","Jogging","Upstairs",
    "Downstairs","Sitting","Standing"
};

static const int NUM_CLASSES = 6;
static const int WINDOW_SIZE = 25;

// ---------- Simulated accelerometer ----------
static void read_accel(float& x, float& y, float& z) {
    x = (rand() % 200 - 100) / 100.0f;
    y = (rand() % 200 - 100) / 100.0f;
    z = 1.0f + (rand() % 40) / 100.0f;
}

// ---------- Feature extraction ----------
static void extract_features(float ax[], float ay[], float az[], float f[7]) {
    float sx=0,sy=0,sz=0,sx2=0,sy2=0,sz2=0,sm=0;
    for(int i=0;i<WINDOW_SIZE;i++){
        sx+=ax[i]; sy+=ay[i]; sz+=az[i];
        sx2+=ax[i]*ax[i]; sy2+=ay[i]*ay[i]; sz2+=az[i]*az[i];
        sm+=sqrtf(ax[i]*ax[i]+ay[i]*ay[i]+az[i]*az[i]);
    }
    float inv=1.0f/WINDOW_SIZE;
    f[0]=sx*inv; f[1]=sy*inv; f[2]=sz*inv;
    f[3]=sqrtf(sx2*inv-f[0]*f[0]);
    f[4]=sqrtf(sy2*inv-f[1]*f[1]);
    f[5]=sqrtf(sz2*inv-f[2]*f[2]);
    f[6]=sm*inv;
}

// ---------- Classifier ----------
static int classify(const float f[7]) {
    float mag=f[6];
    if(mag>1.7f) return 1;
    if(mag>1.4f) return 0;
    if(mag>1.25f) return 3;
    if(mag>1.15f) return 2;
    if(mag<1.05f) return 4;
    return 5;
}

// ---------- MAIN ----------
int main() {
    srand(time(NULL));
    lcd_init();

    lcd_print(1,"Section 12.9");
    lcd_print(3,"HAR Evaluation");

    float ax[WINDOW_SIZE], ay[WINDOW_SIZE], az[WINDOW_SIZE];
    float feat[7];

    int total=0, correct=0;

    while(true) {
        // Simulated TRUE label
        int true_label = rand() % NUM_CLASSES;

        // Collect window
        for(int i=0;i<WINDOW_SIZE;i++){
            read_accel(ax[i],ay[i],az[i]);
            ThisThread::sleep_for(30ms);
        }

        extract_features(ax,ay,az,feat);
        int pred = classify(feat);

        total++;
        if(pred==true_label) correct++;

        float acc = (float)correct / total * 100.0f;

        // LCD
        char l1[40], l2[40], l3[40];
        snprintf(l1,sizeof(l1),"True: %s",LABELS[true_label]);
        snprintf(l2,sizeof(l2),"Pred: %s",LABELS[pred]);
        snprintf(l3,sizeof(l3),"Acc: %.1f %%",acc);

        lcd_print(6,l1);
        lcd_print(7,l2);
        lcd_print(8,l3);

        // Terminal (rapor için iyi)
        printf("True=%s Pred=%s Acc=%.2f%%\n",
               LABELS[true_label], LABELS[pred], acc);

        ThisThread::sleep_for(1s);
    }
}
