#ifndef SAMPLE_ACCEL_H
#define SAMPLE_ACCEL_H

struct AccelSample {
    float ax;
    float ay;
    float az;
    int label;   // 0 = Walking, 1 = Jogging
};

// Kitaba uygun örnekler
static const AccelSample ACCEL_DB[] = {
    {0.1f, 0.9f, 0.2f, 0},
    {0.2f, 1.0f, 0.3f, 0},
    {0.3f, 1.1f, 0.2f, 0},

    {1.2f, 2.0f, 1.5f, 1},
    {1.3f, 2.1f, 1.6f, 1},
    {1.1f, 1.9f, 1.4f, 1}
};

static const int ACCEL_DB_SIZE = 6;

#endif
