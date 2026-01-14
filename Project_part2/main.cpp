#include "mbed.h"

extern "C" {
#include "stm32746g_discovery.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_audio.h"
}

#if __has_include("network.h") && __has_include("network_data.h")
  #define USE_EDGEAI 1
  extern "C" {
    #include "network.h"
    #include "network_data.h"
    #include "ai_platform.h"
  }
#else
  #define USE_EDGEAI 0
#endif

// ---------------------- Ayarlar ----------------------
#define AUDIO_FREQ_HZ     16000U
#define AUDIO_BIT_RES     16U

#define BLOCK_FRAMES      1024U
#define MAX_CH            2U

static uint16_t g_audio_dma[2 * BLOCK_FRAMES * MAX_CH];

#define CAPTURE_MS        1400U
#define CAPTURE_SAMPLES   ((AUDIO_FREQ_HZ * CAPTURE_MS) / 1000U)

#define FRAME_SAMPLES     320U
#define FEAT_LEN          32U

static const char* WORDS[4] = {"help", "run", "stop", "jump"};
static const uint32_t COLORS[4] = { LCD_COLOR_RED, LCD_COLOR_GREEN, LCD_COLOR_BLUE, LCD_COLOR_YELLOW };

static float g_templ[4][FEAT_LEN];
static bool  g_trained[4] = {false,false,false,false};

// aktif kanal sayısı (1 veya 2)
static uint32_t g_in_ch = 1;

static volatile uint32_t g_half_ready = 0;
static volatile uint32_t g_full_ready = 0;

extern "C" void BSP_AUDIO_IN_HalfTransfer_CallBack(void) { g_half_ready = 1; }
extern "C" void BSP_AUDIO_IN_TransferComplete_CallBack(void) { g_full_ready = 1; }
extern "C" void BSP_AUDIO_IN_Error_CallBack(void) {}

// ---------------------- LCD ----------------------
static void lcd_msg(const char* l1, const char* l2 = nullptr) {
    BSP_LCD_Clear(LCD_COLOR_WHITE);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DisplayStringAt(0, 40, (uint8_t*)l1, CENTER_MODE);
    if (l2) BSP_LCD_DisplayStringAt(0, 80, (uint8_t*)l2, CENTER_MODE);
}

static void lcd_color(uint32_t color, const char* text) {
    BSP_LCD_Clear(color);
    BSP_LCD_SetBackColor(color);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DisplayStringAt(0, 60, (uint8_t*)text, CENTER_MODE);
}

// ---------------------- DSP yardımcı ----------------------
static inline uint32_t avg_abs_level_u16_onech(const uint16_t* buf, uint32_t frames, uint32_t ch) {
    uint64_t sum = 0;
    for (uint32_t i = 0; i < frames; i++) {
        uint16_t u = buf[i * ch];
        int32_t s = (int32_t)u - 32768;
        if (s < 0) s = -s;
        sum += (uint32_t)s;
    }
    return (uint32_t)(sum / frames);
}

static inline void u16_to_s16_mono(const uint16_t* in, uint32_t frames, uint32_t ch, int16_t* out) {
    for (uint32_t i = 0; i < frames; i++) {
        int32_t s = (int32_t)in[i * ch] - 32768;
        if (s > 32767) s = 32767;
        if (s < -32768) s = -32768;
        out[i] = (int16_t)s;
    }
}

static bool get_block(uint16_t* out, uint32_t timeout_ms) {
    const uint32_t half_samples = BLOCK_FRAMES * g_in_ch;
    uint32_t t0 = Kernel::get_ms_count();

    while ((Kernel::get_ms_count() - t0) < timeout_ms) {
        if (g_half_ready) {
            g_half_ready = 0;
            memcpy(out, &g_audio_dma[0], half_samples * sizeof(uint16_t));
            return true;
        }
        if (g_full_ready) {
            g_full_ready = 0;
            memcpy(out, &g_audio_dma[half_samples], half_samples * sizeof(uint16_t));
            return true;
        }
        ThisThread::sleep_for(2ms);
    }
    return false;
}

// ---------------------- Feature ----------------------
static void extract_feature(const int16_t* pcm, uint32_t n, float feat[FEAT_LEN]) {
    const uint32_t frames = n / FRAME_SAMPLES;
    static float tmp[256];
    uint32_t fcount = (frames > 256) ? 256 : frames;

    for (uint32_t f = 0; f < fcount; f++) {
        uint64_t sum = 0;
        const int16_t* p = pcm + f * FRAME_SAMPLES;
        for (uint32_t i = 0; i < FRAME_SAMPLES; i++) {
            int32_t s = p[i];
            if (s < 0) s = -s;
            sum += (uint32_t)s;
        }
        tmp[f] = (float)sum / (float)FRAME_SAMPLES;
    }

    for (uint32_t k = 0; k < FEAT_LEN; k++) {
        uint32_t a = (k * fcount) / FEAT_LEN;
        uint32_t b = ((k + 1) * fcount) / FEAT_LEN;
        if (b <= a) b = a + 1;
        if (b > fcount) b = fcount;

        float s = 0.f;
        for (uint32_t i = a; i < b; i++) s += tmp[i];
        feat[k] = s / (float)(b - a);
    }

    float mean = 0.f;
    for (uint32_t i = 0; i < FEAT_LEN; i++) mean += feat[i];
    mean /= (float)FEAT_LEN;

    float var = 0.f;
    for (uint32_t i = 0; i < FEAT_LEN; i++) {
        float d = feat[i] - mean;
        var += d * d;
    }
    var /= (float)FEAT_LEN;

    float stdv = sqrtf(var) + 1e-6f;
    for (uint32_t i = 0; i < FEAT_LEN; i++) feat[i] = (feat[i] - mean) / stdv;
}

static float l2_dist(const float a[FEAT_LEN], const float b[FEAT_LEN]) {
    float s = 0.f;
    for (uint32_t i = 0; i < FEAT_LEN; i++) {
        float d = a[i] - b[i];
        s += d * d;
    }
    return s;
}

// ---------------------- Capture ----------------------
static bool capture_utterance(int16_t* out_pcm, uint32_t out_n) {
    static uint16_t block_u16[BLOCK_FRAMES * MAX_CH];
    static int16_t  block_s16[BLOCK_FRAMES];

    // noise floor (400ms)
    uint32_t t0 = Kernel::get_ms_count();
    uint64_t acc = 0;
    uint32_t cnt = 0;

    while ((Kernel::get_ms_count() - t0) < 400) {
        if (!get_block(block_u16, 250)) continue;
        acc += avg_abs_level_u16_onech(block_u16, BLOCK_FRAMES, g_in_ch);
        cnt++;
    }
    uint32_t noise = cnt ? (uint32_t)(acc / cnt) : 200;
    uint32_t th = noise + 550;

    // trigger bekle (max 4sn) + 120ms pre-roll
    t0 = Kernel::get_ms_count();
    bool trig = false;

    const uint32_t PRE_S = (AUDIO_FREQ_HZ * 120) / 1000; // mono sample
    static int16_t prebuf[2048];
    uint32_t prelen = 0;

    while ((Kernel::get_ms_count() - t0) < 4000) {
        if (!get_block(block_u16, 300)) continue;

        u16_to_s16_mono(block_u16, BLOCK_FRAMES, g_in_ch, block_s16);

        for (uint32_t i = 0; i < BLOCK_FRAMES; i++) {
            if (prelen < PRE_S) prebuf[prelen++] = block_s16[i];
            else {
                memmove(prebuf, prebuf + 1, (PRE_S - 1) * sizeof(int16_t));
                prebuf[PRE_S - 1] = block_s16[i];
            }
        }

        uint32_t lvl = avg_abs_level_u16_onech(block_u16, BLOCK_FRAMES, g_in_ch);

        if (lvl < noise + 200) {
            noise = (uint32_t)(0.97f * noise + 0.03f * lvl);
            if (noise < 50) noise = 50;
            th = noise + 550;
        }

        if (lvl > th) { trig = true; break; }
    }
    if (!trig) return false;

    // capture: pre-roll + out_n
    uint32_t written = 0;
    uint32_t take = (prelen < out_n) ? prelen : out_n;
    for (uint32_t i = 0; i < take; i++) out_pcm[written++] = prebuf[prelen - take + i];

    while (written < out_n) {
        if (!get_block(block_u16, 400)) return false;
        u16_to_s16_mono(block_u16, BLOCK_FRAMES, g_in_ch, block_s16);

        for (uint32_t i = 0; i < BLOCK_FRAMES && written < out_n; i++) {
            out_pcm[written++] = block_s16[i];
        }
    }
    return true;
}

// ---------------------- Audio init ----------------------
static bool audio_start() {
    BSP_LCD_Init();
    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
    BSP_LCD_SelectLayer(0);
    BSP_LCD_DisplayOn();

    lcd_msg("Audio init...");

    g_in_ch = 1;
#if defined(BSP_AUDIO_IN_Stop)
    BSP_AUDIO_IN_Stop(0);
#endif

    uint8_t ret = 0xFF;

#if defined(INPUT_DEVICE_DIGITAL_MICROPHONE_2)
    ret = BSP_AUDIO_IN_InitEx(INPUT_DEVICE_DIGITAL_MICROPHONE_2, AUDIO_FREQ_HZ, AUDIO_BIT_RES, g_in_ch);
    if (ret != AUDIO_OK) {
        g_in_ch = 2;
        ret = BSP_AUDIO_IN_InitEx(INPUT_DEVICE_DIGITAL_MICROPHONE_2, AUDIO_FREQ_HZ, AUDIO_BIT_RES, g_in_ch);
    }
#else
    ret = BSP_AUDIO_IN_Init(AUDIO_FREQ_HZ, AUDIO_BIT_RES, g_in_ch);
    if (ret != AUDIO_OK) {
        g_in_ch = 2;
        ret = BSP_AUDIO_IN_Init(AUDIO_FREQ_HZ, AUDIO_BIT_RES, g_in_ch);
    }
#endif

    if (ret != AUDIO_OK) {
        lcd_msg("AUDIO IN INIT FAILED");
        return false;
    }

    const uint32_t total_samples = 2 * BLOCK_FRAMES * g_in_ch;
    if (BSP_AUDIO_IN_Record((uint16_t*)g_audio_dma, total_samples) != AUDIO_OK) {
        lcd_msg("AUDIO IN RECORD FAILED");
        return false;
    }

    lcd_msg("Audio OK", (g_in_ch == 2) ? "stereo->mono" : "mono");
    ThisThread::sleep_for(400ms);
    return true;
}

// ---------------------- EdgeAI init (opsiyonel) ----------------------
#if USE_EDGEAI
static ai_handle g_net = AI_HANDLE_NULL;
static uint8_t* g_acts = nullptr;
static bool g_edgeai_ok = false;

static bool edgeai_try_init() {
    // Network create
    ai_error err = ai_network_create(&g_net, AI_NETWORK_DATA_CONFIG);
    if (err.type != AI_ERROR_NONE) return false;

    // activations malloc (macro varsa kullan, yoksa params_get ile fallback)
#if defined(AI_NETWORK_DATA_ACTIVATIONS_SIZE)
    g_acts = (uint8_t*)malloc(AI_NETWORK_DATA_ACTIVATIONS_SIZE);
    if (!g_acts) return false;

    ai_network_params p = AI_NETWORK_PARAMS_INIT(
        AI_NETWORK_DATA_WEIGHTS(ai_network_data_weights_get()),
        AI_NETWORK_DATA_ACTIVATIONS(g_acts)
    );
    if (!ai_network_init(g_net, &p)) return false;
#else
    ai_network_params p;
    if (!ai_network_data_params_get(&p)) return false;
    if (!ai_network_init(g_net, &p)) return false;
#endif

    return true;
}
#endif

// ---------------------- Train + Recognize ----------------------
static bool train_word(int widx) {
    static int16_t pcm[CAPTURE_SAMPLES];
    char l1[64];

    snprintf(l1, sizeof(l1), "SAY: %s", WORDS[widx]);
    lcd_msg(l1, "Konusmaya basla...");

    if (!capture_utterance(pcm, CAPTURE_SAMPLES)) {
        lcd_msg("Ses gelmedi", "Tekrar dene...");
        ThisThread::sleep_for(700ms);
        return false;
    }

    extract_feature(pcm, CAPTURE_SAMPLES, g_templ[widx]);
    g_trained[widx] = true;

    lcd_msg("Kayit OK", WORDS[widx]);
    ThisThread::sleep_for(500ms);
    return true;
}

static int recognize(const float feat[FEAT_LEN], float* out_best) {
    float best = 1e30f;
    int best_id = -1;

    for (int w = 0; w < 4; w++) {
        if (!g_trained[w]) continue;
        float d = l2_dist(feat, g_templ[w]);
        if (d < best) { best = d; best_id = w; }
    }

    if (out_best) *out_best = best;
    return best_id;
}

int main() {
    BufferedSerial pc(USBTX, USBRX, 115200);

    if (!audio_start()) {
        while (true) ThisThread::sleep_for(500ms);
    }

    // EdgeAI: sadece "deneyip durum göster"
#if USE_EDGEAI
    if (edgeai_try_init()) {
        g_edgeai_ok = true;
        lcd_msg("EdgeAI OK", "Template mode devam");
        ThisThread::sleep_for(600ms);
    } else {
        lcd_msg("EdgeAI yok/hatali", "Template mode devam");
        ThisThread::sleep_for(600ms);
    }
#else
    lcd_msg("EdgeAI not found", "Template mode");
    ThisThread::sleep_for(600ms);
#endif

    lcd_msg("Egitim", "help/run/stop/jump");
    ThisThread::sleep_for(700ms);

    for (int w = 0; w < 4; w++) {
        while (!train_word(w)) {}
    }

    lcd_msg("Hazir", "help/run/stop/jump");
    ThisThread::sleep_for(500ms);

    static int16_t pcm[CAPTURE_SAMPLES];
    float feat[FEAT_LEN];

    const float ACCEPT_LIMIT = 260.0f;

    while (true) {
        if (!capture_utterance(pcm, CAPTURE_SAMPLES)) continue;

        extract_feature(pcm, CAPTURE_SAMPLES, feat);

        float best = 0.f;
        int cmd = recognize(feat, &best);

        char dbg[64];
        snprintf(dbg, sizeof(dbg), "dist=%.1f", best);

        if (cmd < 0 || best > ACCEPT_LIMIT) {
            lcd_msg("Bilinmeyen", dbg);
            ThisThread::sleep_for(350ms);
            lcd_msg("Dinliyorum...", "help/run/stop/jump");
            continue;
        }

        lcd_color(COLORS[cmd], WORDS[cmd]);
        ThisThread::sleep_for(550ms);
        lcd_msg("Dinliyorum...", "help/run/stop/jump");
    }
}
