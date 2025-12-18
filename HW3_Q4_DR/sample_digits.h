#ifndef SAMPLE_DIGITS_H
#define SAMPLE_DIGITS_H

struct DigitSample {
    float hu[7];   // Hu moments
    int label;     // 0 = digit '0', 1 = digit 'not 0'
};

// Kitaptaki 10.9 örneklerine uygun
static const DigitSample DIGIT_DB[] = {
    {{0.21f, 0.02f, 0.01f, 0.00f, 0.00f, 0.00f, 0.00f}, 0},
    {{0.22f, 0.03f, 0.01f, 0.01f, 0.00f, 0.00f, 0.00f}, 0},
    {{0.85f, 0.44f, 0.32f, 0.10f, 0.02f, 0.01f, 0.01f}, 1},
    {{0.90f, 0.50f, 0.35f, 0.12f, 0.03f, 0.02f, 0.01f}, 1}
};

static const int DIGIT_DB_SIZE = 4;

#endif