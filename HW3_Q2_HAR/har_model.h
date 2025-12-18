#ifndef HAR_MODEL_H
#define HAR_MODEL_H

void extract_features(const float *ax,
                      const float *ay,
                      const float *az,
                      int N,
                      float feat[4]);

int har_predict(const float feat[4]);

#endif
