#ifndef DIGIT_CLASSIFIER_H
#define DIGIT_CLASSIFIER_H

// 8x8 image -> 64 piksel
#define DIGIT_IMG_SIZE 64

// En fazla 10 sınıf (0..9)
int digit_predict(const float img[DIGIT_IMG_SIZE]);

// Demo için: test imajlarını elde etmek
// idx: 0..N kullan, fonksiyon ilgili örnek imajı img_out'a kopyalar
void get_test_image(int idx, float img_out[DIGIT_IMG_SIZE]);

#endif // DIGIT_CLASSIFIER_H
