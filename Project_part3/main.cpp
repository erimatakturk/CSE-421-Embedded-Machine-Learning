#include "mbed.h"
#include <cmath>
#include <cstdint>
#include <cstdio>

extern "C" {
#include "stm32746g_discovery.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_sdram.h"
#include "stm32746g_discovery_ts.h"
}

// ---------------- LCD helpers ----------------
static void lcd_init()
{
    BSP_SDRAM_Init();

    BSP_LCD_Init();
    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
    BSP_LCD_SelectLayer(0);

    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_SetFont(&Font16);
}

static void ftoa6(float v, char* buf, size_t n)
{
    int neg = (v < 0.0f);
    float a = neg ? -v : v;

    int ip = (int)a;
    int fp = (int)((a - (float)ip) * 1000000.0f + 0.5f);
    if (fp >= 1000000) { ip += 1; fp = 0; }

    if (neg) snprintf(buf, n, "-%d.%06d", ip, fp);
    else     snprintf(buf, n,  "%d.%06d", ip, fp);
}

// ---------------- Touch (minimal) ----------------
static bool touch_ok = false;

static void ts_init()
{
    touch_ok = (BSP_TS_Init(480, 272) == TS_OK);
}

static bool touch_any(int &tx, int &ty)
{
    if (!touch_ok) return false;
    TS_StateTypeDef st;
    BSP_TS_GetState(&st);
    if (!st.touchDetected) return false;
    tx = st.touchX[0];
    ty = st.touchY[0];
    return true;
}

// ---------------- Error stats ----------------
struct Stats {
    uint32_t n = 0;
    float sum_abs = 0.0f;
    float sum_sq  = 0.0f;

    void reset() { n = 0; sum_abs = 0.0f; sum_sq = 0.0f; }
    void add(float err) {
        n++;
        sum_abs += fabsf(err);
        sum_sq  += err * err;
    }
    float mae()  const { return n ? (sum_abs / (float)n) : 0.0f; }
    float rmse() const { return n ? sqrtf(sum_sq / (float)n) : 0.0f; }
};

// ---------------- "Model" (intentionally imperfect) ----------------
static float wrap_pi(float x)
{
    const float TWO_PI = 6.2831853071795864769f;
    const float PI     = 3.14159265358979323846f;

    x = fmodf(x, TWO_PI);
    if (x >  PI) x -= TWO_PI;
    if (x < -PI) x += TWO_PI;
    return x;
}

// quantize helper (like int8/low resolution effect)
static float quantize(float x, float step)
{
    // round(x/step)*step
    float q = floorf((x / step) + 0.5f);
    return q * step;
}

// Very low-order approximation: sin(x) ~ x - x^3/6 on [-pi/2,pi/2]
static float sin_pred_lowpoly(float x)
{
    const float PI     = 3.14159265358979323846f;
    const float HALFPI = 1.57079632679489661923f;

    x = wrap_pi(x);

    // symmetry to [-pi/2, pi/2]
    if (x >  HALFPI) x = PI - x;
    if (x < -HALFPI) x = -PI - x;

    // emulate quantized model input (tune step to get visible error)
    x = quantize(x, 0.12f);  // <-- hata görmek için 0.08-0.20 arası oynatabilirsin

    float x2 = x * x;
    float y = x * (1.0f - x2 * (1.0f / 6.0f)); // x - x^3/6

    // clamp (model saturates)
    if (y >  1.0f) y = 1.0f;
    if (y < -1.0f) y = -1.0f;
    return y;
}

int main()
{
    lcd_init();
    ts_init();

    bool auto_x = true;
    Stats st;

    float x = -3.14159f;
    const float step = 0.18f;

    uint32_t last_touch_ms = 0;

    while (true) {
        // Touch controls (sade):
        // - Sağ üst dokun: AUTO ON/OFF
        // - Sağ alt dokun: RESET stats
        int tx=0, ty=0;
        uint32_t now = Kernel::get_ms_count();

        if (touch_any(tx, ty) && (now - last_touch_ms > 250)) {
            last_touch_ms = now;

            if (tx >= 240 && ty < 90) {
                auto_x = !auto_x;
            } else if (tx >= 240 && ty > 200) {
                st.reset();
            }
        }

        float real = sinf(x);
        float pred = sin_pred_lowpoly(x);   // <-- BİLEREK HATALI "MODEL"

        float err = pred - real;
        float abs_err = fabsf(err);
        st.add(err);

        float err_pct = 0.0f;
        // yüzdeyi sin(x) küçükken patlatmamak için güvenli hesap
        float denom = fmaxf(0.05f, fabsf(real));
        err_pct = (abs_err / denom) * 100.0f;

        // Render (sade, sabit)
        BSP_LCD_Clear(LCD_COLOR_BLACK);
        BSP_LCD_SetTextColor(LCD_COLOR_WHITE);

        BSP_LCD_DisplayStringAt(0, 5, (uint8_t*)"SIN Tahmini", CENTER_MODE);

        char sx[32], sreal[32], spred[32], sabs[32], spct[32], smae[32], srmse[32];
        ftoa6(x, sx, sizeof(sx));
        ftoa6(real, sreal, sizeof(sreal));
        ftoa6(pred, spred, sizeof(spred));
        ftoa6(abs_err, sabs, sizeof(sabs));
        ftoa6(err_pct, spct, sizeof(spct));
        ftoa6(st.mae(), smae, sizeof(smae));
        ftoa6(st.rmse(), srmse, sizeof(srmse));

        char line[80];

        snprintf(line, sizeof(line), "x        : %s", sx);
        BSP_LCD_DisplayStringAt(20, 55, (uint8_t*)line, LEFT_MODE);

        snprintf(line, sizeof(line), "sin(x)    : %s", sreal);
        BSP_LCD_DisplayStringAt(20, 80, (uint8_t*)line, LEFT_MODE);

        snprintf(line, sizeof(line), "tahmin    : %s", spred);
        BSP_LCD_DisplayStringAt(20, 110, (uint8_t*)line, LEFT_MODE);

        snprintf(line, sizeof(line), "|hata|    : %s", sabs);
        BSP_LCD_DisplayStringAt(20, 140, (uint8_t*)line, LEFT_MODE);

        snprintf(line, sizeof(line), "hata(%%)   : %s", spct);
        BSP_LCD_DisplayStringAt(20, 165, (uint8_t*)line, LEFT_MODE);

        snprintf(line, sizeof(line), "MAE=%s", smae);
        BSP_LCD_DisplayStringAt(20, 195, (uint8_t*)line, LEFT_MODE);

        snprintf(line, sizeof(line), "RMSE=%s  n=%lu", srmse, (unsigned long)st.n);
        BSP_LCD_DisplayStringAt(20, 220, (uint8_t*)line, LEFT_MODE);

        snprintf(line, sizeof(line), "AUTO:%s  (SagUst:Auto  SagAlt:Reset)",
                 auto_x ? "ON" : "OFF");
        BSP_LCD_DisplayStringAt(0, 245, (uint8_t*)line, CENTER_MODE);

        if (auto_x) {
            x += step;
            if (x > 3.14159f) x = -3.14159f;
        }

        ThisThread::sleep_for(250ms);
    }
}
