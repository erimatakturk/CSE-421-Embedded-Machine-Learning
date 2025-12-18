#include "har_model.h"
#include <cmath>

// -------- Feature extraction --------
// mean magnitude + std magnitude + mean z + std z
void extract_features(const float *ax,
                      const float *ay,
                      const float *az,
                      int N,
                      float feat[4]) {

    float sum_mag = 0.0f;
    float sumsq_mag = 0.0f;
    float sum_z = 0.0f;
    float sumsq_z = 0.0f;

    for (int i = 0; i < N; i++) {
        float mag = sqrtf(ax[i]*ax[i] + ay[i]*ay[i] + az[i]*az[i]);
        sum_mag += mag;
        sumsq_mag += mag * mag;

        sum_z += az[i];
        sumsq_z += az[i] * az[i];
    }

    float mean_mag = sum_mag / N;
    float var_mag  = sumsq_mag / N - mean_mag * mean_mag;
    if (var_mag < 0) var_mag = 0;

    float mean_z = sum_z / N;
    float var_z  = sumsq_z / N - mean_z * mean_z;
    if (var_z < 0) var_z = 0;

    feat[0] = mean_mag;
    feat[1] = sqrtf(var_mag);
    feat[2] = mean_z;
    feat[3] = sqrtf(var_z);
}

// -------- Single neuron (offline trained) --------
static const float WEIGHTS[4] = {
    1.2f, 0.8f, 0.6f, 0.4f
};

static const float BIAS = -1.0f;

static float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}

int har_predict(const float feat[4]) {

    float sum = BIAS;
    for (int i = 0; i < 4; i++) {
        sum += WEIGHTS[i] * feat[i];
    }

    return (sigmoid(sum) > 0.5f) ? 1 : 0;
}
