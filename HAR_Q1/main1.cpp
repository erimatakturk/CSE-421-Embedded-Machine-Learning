// main.cpp - Q1: Human Activity Recognition using sample_data.h as "database"
// Board: DISCO-F746NG, Mbed OS 6

#include "mbed.h"
#include "stm32f7xx_hal.h"
#include "stm32746g_discovery_sdram.h"
#include "stm32746g_discovery_lcd.h"

#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <chrono>

#include "sample_data.h"  // <-- database burada

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
    BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
    BSP_LCD_SetFont(&Font24);
}

static void lcd_print_line(int line, const char *text) {
    BSP_LCD_DisplayStringAt(0, LINE(line), (uint8_t*)text, CENTER_MODE);
}

// -------- Model sabitleri --------
static const int NUM_CLASSES  = 3;   // 0:Walking, 1:Jogging, 2:Sitting
static const int NUM_FEATURES = 10;  // mean/std/mag özellikleri

// Eğitim verisi (feature space'te)
static float train_features[SAMPLE_DB_SIZE][NUM_FEATURES];
static int   train_labels[SAMPLE_DB_SIZE];

// Bayes parametreleri
static float MEANS[NUM_CLASSES][NUM_FEATURES];
static float CLASS_PRIORS[NUM_CLASSES];
static float INV_COVS[NUM_CLASSES][NUM_FEATURES][NUM_FEATURES];
static float DETS[NUM_CLASSES];

// Sınıf isimleri
static const char* HAR_LABELS[NUM_CLASSES] = {
    "Walking",
    "Jogging",
    "Sitting"
};

// -------- Yardımcı: random float --------
static float randf(float a, float b) {
    return a + (b - a) * (rand() / (float)RAND_MAX);
}

// -------- Feature çıkarma (tek pencere için) --------
// N noktadan oluşan pencere: ax[], ay[], az[] -> out[10]
static void create_features_from_window(const float *ax,
                                        const float *ay,
                                        const float *az,
                                        int N,
                                        float out[NUM_FEATURES])
{
    if (N <= 0) {
        for (int i = 0; i < NUM_FEATURES; ++i) out[i] = 0.0f;
        return;
    }

    float sum_x = 0.0f, sum_y = 0.0f, sum_z = 0.0f;
    float sumsq_x = 0.0f, sumsq_y = 0.0f, sumsq_z = 0.0f;
    float sum_mag = 0.0f, sumsq_mag = 0.0f;
    float max_mag = -1e30f;
    float min_mag =  1e30f;

    for (int i = 0; i < N; ++i) {
        float x = ax[i];
        float y = ay[i];
        float z = az[i];

        sum_x += x;
        sum_y += y;
        sum_z += z;

        sumsq_x += x * x;
        sumsq_y += y * y;
        sumsq_z += z * z;

        float mag = std::sqrt(x * x + y * y + z * z);
        sum_mag    += mag;
        sumsq_mag  += mag * mag;

        if (mag > max_mag) max_mag = mag;
        if (mag < min_mag) min_mag = mag;
    }

    float invN = 1.0f / (float)N;

    float mean_x = sum_x * invN;
    float mean_y = sum_y * invN;
    float mean_z = sum_z * invN;

    float var_x = sumsq_x * invN - mean_x * mean_x;
    float var_y = sumsq_y * invN - mean_y * mean_y;
    float var_z = sumsq_z * invN - mean_z * mean_z;

    if (var_x < 0) var_x = 0;
    if (var_y < 0) var_y = 0;
    if (var_z < 0) var_z = 0;

    float std_x = std::sqrt(var_x);
    float std_y = std::sqrt(var_y);
    float std_z = std::sqrt(var_z);

    float mean_mag = sum_mag * invN;
    float var_mag = sumsq_mag * invN - mean_mag * mean_mag;
    if (var_mag < 0) var_mag = 0;
    float std_mag = std::sqrt(var_mag);

    out[0] = mean_x;
    out[1] = mean_y;
    out[2] = mean_z;
    out[3] = std_x;
    out[4] = std_y;
    out[5] = std_z;
    out[6] = mean_mag;
    out[7] = std_mag;
    out[8] = max_mag;
    out[9] = min_mag;
}

// -------- 1) sample_data.h -> feature space'e taşı --------
static void load_database_and_compute_features() {
    for (int i = 0; i < SAMPLE_DB_SIZE; ++i) {
        const SampleRow &row = SAMPLE_DB[i];
        float ax[1] = { row.ax };
        float ay[1] = { row.ay };
        float az[1] = { row.az };

        float feat[NUM_FEATURES];
        create_features_from_window(ax, ay, az, 1, feat);

        for (int k = 0; k < NUM_FEATURES; ++k) {
            train_features[i][k] = feat[k];
        }
        train_labels[i] = row.cls;
    }

    printf("Loaded %d samples from sample_data.h\n", SAMPLE_DB_SIZE);
}

// -------- 2) Bayes parametrelerini veritabanından hesapla --------
static void compute_bayes_parameters() {
    // Sıfırla
    for (int c = 0; c < NUM_CLASSES; ++c) {
        CLASS_PRIORS[c] = 0.0f;
        for (int k = 0; k < NUM_FEATURES; ++k) {
            MEANS[c][k] = 0.0f;
            for (int j = 0; j < NUM_FEATURES; ++j) {
                INV_COVS[c][k][j] = 0.0f;
            }
        }
        DETS[c] = 1.0f;
    }

    int count_per_class[NUM_CLASSES] = {0};

    // 2.1: Ortalama
    for (int i = 0; i < SAMPLE_DB_SIZE; ++i) {
        int c = train_labels[i];
        count_per_class[c]++;
        for (int k = 0; k < NUM_FEATURES; ++k) {
            MEANS[c][k] += train_features[i][k];
        }
    }
    for (int c = 0; c < NUM_CLASSES; ++c) {
        if (count_per_class[c] > 0) {
            float invNc = 1.0f / (float)count_per_class[c];
            for (int k = 0; k < NUM_FEATURES; ++k) {
                MEANS[c][k] *= invNc;
            }
        }
    }

    // 2.2: Diagonal variance
    float VARS[NUM_CLASSES][NUM_FEATURES];
    for (int c = 0; c < NUM_CLASSES; ++c) {
        for (int k = 0; k < NUM_FEATURES; ++k) {
            VARS[c][k] = 0.0f;
        }
    }

    for (int i = 0; i < SAMPLE_DB_SIZE; ++i) {
        int c = train_labels[i];
        for (int k = 0; k < NUM_FEATURES; ++k) {
            float diff = train_features[i][k] - MEANS[c][k];
            VARS[c][k] += diff * diff;
        }
    }

    for (int c = 0; c < NUM_CLASSES; ++c) {
        float det = 1.0f;
        if (count_per_class[c] > 1) {
            float invNc = 1.0f / (float)count_per_class[c];
            for (int k = 0; k < NUM_FEATURES; ++k) {
                VARS[c][k] *= invNc;
                if (VARS[c][k] < 1e-4f) VARS[c][k] = 1e-4f;
                det *= VARS[c][k];
                INV_COVS[c][k][k] = 1.0f / VARS[c][k];
            }
        } else {
            // Çok az örnek varsa birim covariance
            for (int k = 0; k < NUM_FEATURES; ++k) {
                VARS[c][k] = 1.0f;
                det *= 1.0f;
                INV_COVS[c][k][k] = 1.0f;
            }
        }
        DETS[c] = det;
    }

    // 2.3: Priorlar
    for (int c = 0; c < NUM_CLASSES; ++c) {
        CLASS_PRIORS[c] = (float)count_per_class[c] / (float)SAMPLE_DB_SIZE;
    }

    printf("Bayes parameters computed from sample_data.h\n");
}

// -------- 3) Bayes diskriminant & tahmin --------
static float bayes_discriminant(const float *x, int cls) {
    float diff[NUM_FEATURES];
    for (int k = 0; k < NUM_FEATURES; ++k) {
        diff[k] = x[k] - MEANS[cls][k];
    }

    float quad = 0.0f;
    for (int k = 0; k < NUM_FEATURES; ++k) {
        float inv_var = INV_COVS[cls][k][k]; // diagonal only
        quad += diff[k] * inv_var * diff[k];
    }

    float val = -0.5f * quad;
    val -= 0.5f * std::log(DETS[cls]);
    val += std::log(CLASS_PRIORS[cls] + 1e-9f);

    return val;
}

static int bayes_predict(const float *x) {
    float best_val = -1e30f;
    int best_cls = -1;
    for (int c = 0; c < NUM_CLASSES; ++c) {
        float g = bayes_discriminant(x, c);
        if (g > best_val) {
            best_val = g;
            best_cls = c;
        }
    }
    return best_cls;
}

// ---- Yeni: skorlarla birlikte tahmin (açıklama için) ----
static int bayes_predict_explain(const float *x, float scores[NUM_CLASSES]) {
    float best_val = -1e30f;
    int best_cls = -1;
    for (int c = 0; c < NUM_CLASSES; ++c) {
        float g = bayes_discriminant(x, c);
        scores[c] = g;
        if (g > best_val) {
            best_val = g;
            best_cls = c;
        }
    }
    return best_cls;
}

// -------- MAIN --------
int main() {
    srand((unsigned)time(NULL));

    init_display();
    lcd_print_line(0, "Q1: HAR - sample_data.h");

    printf("== Q1: HAR using sample_data.h as DB ==\n");

    // 1) Veritabanını yükle ve feature'lara çevir
    load_database_and_compute_features();

    // 2) Bayes parametrelerini hesapla
    compute_bayes_parameters();

    // 3) Sürekli random örnek seç ve açıklamalı tahmin yap
    while (true) {
        int test_index = rand() % SAMPLE_DB_SIZE;
        SampleRow test_row = SAMPLE_DB[test_index];

        float ax[1] = { test_row.ax };
        float ay[1] = { test_row.ay };
        float az[1] = { test_row.az };
        float test_feat[NUM_FEATURES];
        create_features_from_window(ax, ay, az, 1, test_feat);

        float scores[NUM_CLASSES];
        int pred_cls = bayes_predict_explain(test_feat, scores);

        // ---- Seri port tarafı ----
        printf("\n--- Yeni Ornek ---\n");
        printf("Index       : %d\n", test_index);
        printf("Gercek sinif: %s (%d)\n", HAR_LABELS[test_row.cls], test_row.cls);
        printf("Ham ivmeler : ax=%.3f  ay=%.3f  az=%.3f\n",
               test_row.ax, test_row.ay, test_row.az);

        printf("Feature vektoru:\n");
        for (int i = 0; i < NUM_FEATURES; ++i) {
            printf("  f[%d] = %.5f\n", i, test_feat[i]);
        }

        printf("\nSinif skorlari (g_c(x)):\n");
        for (int c = 0; c < NUM_CLASSES; ++c) {
            printf("  %s : %.5f\n", HAR_LABELS[c], scores[c]);
        }

        printf("\nTahmin: %s\n", HAR_LABELS[pred_cls]);
        printf("Neden bu sonuca gitti?\n");
        printf("  -> %s sinifinin skoru en yuksek oldugu icin.\n",
               HAR_LABELS[pred_cls]);

        // ---- LCD tarafı ----
        char line1[64];
        char line2[64];
        char line3[64];

        snprintf(line1, sizeof(line1), "idx: %d", test_index);
        snprintf(line2, sizeof(line2), "True: %s", HAR_LABELS[test_row.cls]);
        snprintf(line3, sizeof(line3), "Pred: %s", HAR_LABELS[pred_cls]);

        lcd_print_line(2, line1);
        lcd_print_line(3, line2);
        lcd_print_line(4, line3);

        ThisThread::sleep_for(2s);   // 2 sn'de bir yeni ornek
    }
}