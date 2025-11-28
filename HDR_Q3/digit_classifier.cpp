// digit_classifier.cpp
#include "digit_classifier.h"
#include <cmath>

// Kaç prototip digit'im var (her biri 8x8 = 64 boyutlu vektör)
static const int NUM_TEMPLATES = 10;     // 0..9  için 1'er şablon
static const int NUM_DIGIT_CLASSES = 10; // 0..9

// Basit oyuncak 8x8 imajlar.
// Gerçek uygulamada bunlar offline datasetten hesaplanıp buraya gömülür.
// Burada sadece "şeklen" bir şeyler koyuyoruz.
static const float DIGIT_TEMPLATES[NUM_TEMPLATES][DIGIT_IMG_SIZE] = {
    // Digit 0
    {
        0,1,1,1,1,1,1,0,
        1,1,0,0,0,0,1,1,
        1,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,1,
        1,1,0,0,0,0,1,1,
        0,1,1,1,1,1,1,0
    },
    // Digit 1
    {
        0,0,0,1,1,0,0,0,
        0,0,1,1,1,0,0,0,
        0,1,0,1,1,0,0,0,
        0,0,0,1,1,0,0,0,
        0,0,0,1,1,0,0,0,
        0,0,0,1,1,0,0,0,
        0,0,0,1,1,0,0,0,
        0,1,1,1,1,1,1,0
    },
    // Digit 2
    {
        0,1,1,1,1,1,1,0,
        1,0,0,0,0,0,0,1,
        0,0,0,0,0,0,0,1,
        0,0,0,0,0,0,1,0,
        0,0,0,0,0,1,0,0,
        0,0,0,0,1,0,0,0,
        0,0,0,1,0,0,0,0,
        1,1,1,1,1,1,1,1
    },
    // Digit 3
    {
        0,1,1,1,1,1,1,0,
        1,0,0,0,0,0,0,1,
        0,0,0,0,0,0,0,1,
        0,0,0,1,1,1,1,0,
        0,0,0,0,0,0,0,1,
        0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,1,
        0,1,1,1,1,1,1,0
    },
    // Digit 4
    {
        0,0,0,0,1,1,0,0,
        0,0,0,1,1,1,0,0,
        0,0,1,0,1,1,0,0,
        0,1,0,0,1,1,0,0,
        1,0,0,0,1,1,0,0,
        1,1,1,1,1,1,1,1,
        0,0,0,0,1,1,0,0,
        0,0,0,0,1,1,0,0
    },
    // Digit 5
    {
        1,1,1,1,1,1,1,1,
        1,0,0,0,0,0,0,0,
        1,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,0,
        0,0,0,0,0,0,0,1,
        0,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,1,
        0,1,1,1,1,1,1,0
    },
    // Digit 6
    {
        0,0,1,1,1,1,0,0,
        0,1,0,0,0,0,0,0,
        1,0,0,0,0,0,0,0,
        1,1,1,1,1,1,1,0,
        1,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,1,
        0,1,1,1,1,1,1,0
    },
    // Digit 7
    {
        1,1,1,1,1,1,1,1,
        0,0,0,0,0,1,1,0,
        0,0,0,0,1,1,0,0,
        0,0,0,1,1,0,0,0,
        0,0,1,1,0,0,0,0,
        0,0,1,1,0,0,0,0,
        0,0,1,1,0,0,0,0,
        0,0,1,1,0,0,0,0
    },
    // Digit 8
    {
        0,1,1,1,1,1,1,0,
        1,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,1,
        0,1,1,1,1,1,1,0,
        1,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,1,
        0,1,1,1,1,1,1,0
    },
    // Digit 9
    {
        0,1,1,1,1,1,1,0,
        1,0,0,0,0,0,0,1,
        1,0,0,0,0,0,0,1,
        0,1,1,1,1,1,1,1,
        0,0,0,0,0,0,0,1,
        0,0,0,0,0,0,0,1,
        0,0,0,0,0,0,1,0,
        0,1,1,1,1,1,0,0
    }
};

// Her template'in label'i (0..9)
static const int TEMPLATE_LABELS[NUM_TEMPLATES] = {
    0,1,2,3,4,5,6,7,8,9
};

static float euclidean_distance(const float *a, const float *b, int n) {
    float sum = 0.0f;
    for (int i = 0; i < n; ++i) {
        float d = a[i] - b[i];
        sum += d * d;
    }
    return std::sqrt(sum);
}

int digit_predict(const float img[DIGIT_IMG_SIZE]) {
    // En yakın (min mesafeli) template'i bul
    float best_dist = 1e30f;
    int best_label = -1;

    for (int t = 0; t < NUM_TEMPLATES; ++t) {
        float d = euclidean_distance(img, DIGIT_TEMPLATES[t], DIGIT_IMG_SIZE);
        if (d < best_dist) {
            best_dist = d;
            best_label = TEMPLATE_LABELS[t];
        }
    }
    return best_label;
}

// Demo için: test imajı olarak template'lerden birini döndür
void get_test_image(int idx, float img_out[DIGIT_IMG_SIZE]) {
    int t = idx % NUM_TEMPLATES;
    for (int i = 0; i < DIGIT_IMG_SIZE; ++i) {
        img_out[i] = DIGIT_TEMPLATES[t][i];
    }
}
