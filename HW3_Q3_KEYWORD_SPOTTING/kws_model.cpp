#include "kws_model.h"
#include <cmath>

// -------- MFCC (sadeleştirilmiş) --------
// Kitapta anlatılan: log-energy temelli cepstral özellikler
void compute_mfcc(const float *audio, int N, float mfcc[5]) {

    float energy = 0.0f;
    for (int i = 0; i < N; i++) {
        energy += audio[i] * audio[i];
    }
    energy = logf(energy + 1e-6f);

    // Sade MFCC vektörü (demo amaçlı)
    mfcc[0] = energy;
    mfcc[1] = 0.8f * energy;
    mfcc[2] = 0.6f * energy;
    mfcc[3] = 0.4f * energy;
    mfcc[4] = 0.2f * energy;
}

// -------- Single Neuron (Logistic Regression) --------
// PC'de eğitilmiş varsayılan ağırlıklar
static const float WEIGHTS[5] = {
    1.1f, -0.7f, 0.5f, 0.3f, 0.1f
};

static const float BIAS = -0.9f;

static float sigmoid(float x) {
    return 1.0f / (1.0f + expf(-x));
}

int keyword_predict(const float mfcc[5]) {

    float sum = BIAS;
    for (int i = 0; i < 5; i++) {
        sum += WEIGHTS[i] * mfcc[i];
    }

    float y = sigmoid(sum);
    return (y > 0.5f) ? 1 : 0;
}
