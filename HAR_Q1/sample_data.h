// sample_data.h
#pragma once

// Basit bir satır yapısı: [class_id, ax, ay, az]
struct SampleRow {
    int cls;     // 0 = Walking, 1 = Jogging, 2 = Sitting
    float ax;
    float ay;
    float az;
};

// Buraya veritabanı satırlarını ekle
// İstediğin kadar satır ekleyebilirsin.
static const SampleRow SAMPLE_DB[] = {
    // cls,   ax,     ay,     az
    {0,  0.52f,  1.02f,  9.70f},  // Walking
    {0,  0.48f,  0.95f,  9.85f},
    {0,  0.60f,  1.10f,  9.65f},

    {1,  2.10f,  2.05f,  8.80f},  // Jogging
    {1,  1.90f,  2.20f,  8.90f},
    {1,  2.30f,  1.80f,  8.70f},

    {2,  0.02f,  0.01f,  9.78f},  // Sitting
    {2, -0.05f, -0.03f,  9.82f},
    {2,  0.00f,  0.04f,  9.75f}
};

// Toplam satır sayısı:
static const int SAMPLE_DB_SIZE = sizeof(SAMPLE_DB) / sizeof(SampleRow);
