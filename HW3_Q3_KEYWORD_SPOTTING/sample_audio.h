#ifndef SAMPLE_AUDIO_H
#define SAMPLE_AUDIO_H

// Kısa bir ses frame'i (örnek)
static const float AUDIO_FRAME[] = {
     0.10f,  0.20f,  0.35f,  0.60f,  0.90f,  1.00f,  0.80f,  0.40f,
     0.20f,  0.10f,  0.00f, -0.10f, -0.30f, -0.60f, -0.80f, -1.00f,
    -0.70f, -0.40f, -0.20f, -0.10f,  0.00f,  0.10f,  0.30f,  0.60f,
     0.80f,  0.90f,  0.60f,  0.30f,  0.10f,  0.00f, -0.10f, -0.20f
};

static const int AUDIO_FRAME_SIZE = 32;

#endif
