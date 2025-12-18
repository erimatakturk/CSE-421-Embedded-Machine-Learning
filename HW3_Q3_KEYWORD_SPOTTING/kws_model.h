#ifndef KWS_MODEL_H
#define KWS_MODEL_H

void compute_mfcc(const float *audio, int N, float mfcc[5]);
int keyword_predict(const float mfcc[5]);

#endif
