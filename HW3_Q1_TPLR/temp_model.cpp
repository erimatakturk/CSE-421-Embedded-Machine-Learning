#include "temp_model.h"

// y = w0*x0 + w1*x1 + ... + w4*x4 + b
static const float WEIGHTS[5] = {
    0.28f, 0.25f, 0.22f, 0.15f, 0.08f
};

static const float BIAS = 0.12f;

float predict_temperature(const float prev[5]) {
    float y = BIAS;
    for (int i = 0; i < 5; i++) {
        y += WEIGHTS[i] * prev[i];
    }
    return y;
}
