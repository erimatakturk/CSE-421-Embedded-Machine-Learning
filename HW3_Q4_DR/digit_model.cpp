#include "digit_model.h"
#include <cmath>

// Offline eğitilmiş model parametreleri
static const float WEIGHTS[7] = {
    1.2f, -0.8f, 0.6f, 0.4f, 0.2f, 0.1f, 0.05f
};

static const float BIAS = -0.9f;

static float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}

int digit_predict(const float hu[7]) {

    float sum = BIAS;
    for (int i = 0; i < 7; i++) {
        sum += WEIGHTS[i] * hu[i];
    }

    float y = sigmoid(sum);
    return (y > 0.5f) ? 1 : 0;
}