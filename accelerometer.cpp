// main.cpp
#include "mbed.h"
#include "stm32f7xx_hal.h"

// --- BSP headers (STM32Cube/Discovery BSP must be present in project) ---
#include "stm32746g_discovery_sdram.h"
#include "stm32746g_discovery_lcd.h"

#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <cstdlib>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


// Console
FileHandle *mbed::mbed_override_console(int){
    static UnbufferedSerial pc(USBTX, USBRX, 115200);
    return &pc;
}

// ----- Feature extraction params -----
const float SAMPLE_RATE_HZ = 20.0f;
const int WINDOW_SIZE = 128;
const int WINDOW_STEP = 64;
const bool USE_SIMULATION = true; // gerçek sensör bağlayınca false yapıp read_accel_hw ekle

static float t_global = 0.0f;
static float dt = 1.0f / SAMPLE_RATE_HZ;
static float randf(float a, float b) { return a + (b-a) * (rand() / (float)RAND_MAX); }

// Simüle ivme (test için)
void read_accel_sim(float &x, float &y, float &z) {
    float f1 = 1.0f, f2 = 0.5f;
    x = 1.0f * sinf(2.0f * M_PI * f1 * t_global) + 0.12f * randf(-1.0f, 1.0f);
    y = 1.0f * sinf(2.0f * M_PI * f1 * t_global + 0.5f) + 0.12f * randf(-1.0f, 1.0f);
    z = 0.5f * sinf(2.0f * M_PI * f2 * t_global) + 0.12f * randf(-1.0f, 1.0f);
    t_global += dt;
}

// ---------------- basic stats ----------------
float mean(const std::vector<float>& v){
    if(v.empty()) return 0.0f;
    float s = std::accumulate(v.begin(), v.end(), 0.0f);
    return s / v.size();
}
float variance(const std::vector<float>& v, float m){
    if(v.size() < 2) return 0.0f;
    float s=0;
    for(auto &x:v) s += (x-m)*(x-m);
    return s / v.size();
}
float stddev(const std::vector<float>& v, float m){ return sqrtf(variance(v,m)); }
float rms(const std::vector<float>& v){
    if(v.empty()) return 0.0f;
    float s=0; for(auto &x:v) s += x*x; return sqrtf(s / v.size());
}

// ---------------- feature struct & extractor ----------------
struct MiniFeatures {
    int wid;
    float mean_x, mean_y, mean_z;
    float rms_x, rms_y, rms_z;
};

MiniFeatures extract_mini(const std::vector<float>& vx,
                          const std::vector<float>& vy,
                          const std::vector<float>& vz, int wid){
    MiniFeatures f; f.wid = wid;
    f.mean_x = mean(vx); f.mean_y = mean(vy); f.mean_z = mean(vz);
    f.rms_x = rms(vx); f.rms_y = rms(vy); f.rms_z = rms(vz);
    return f;
}

// Print CSV to serial
void print_csv(const MiniFeatures &f){
    printf("WID,%d,mean_x,%.3f,mean_y,%.3f,mean_z,%.3f,rms_x,%.3f,rms_y,%.3f,rms_z,%.3f\n",
           f.wid, f.mean_x, f.mean_y, f.mean_z, f.rms_x, f.rms_y, f.rms_z);
}

// LCD helper: print centered at a line (0..)
void lcd_print_line(int line, const char *text){
    BSP_LCD_DisplayStringAt(0, LINE(line), (uint8_t*)text, CENTER_MODE);
}

// main
int main(){
    srand((unsigned)time(NULL));
    ThisThread::sleep_for(800ms);

    // --- INIT LCD & SDRAM (BSP) ---
    BSP_SDRAM_Init(); // SDRAM must be present (DISCOVERY)
    BSP_LCD_Init();
    // init layer 0 framebuffer at SDRAM base (F7 DISCOV)
    BSP_LCD_LayerDefaultInit(0, (uint32_t)0xC0000000);
    BSP_LCD_SelectLayer(0);
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    BSP_LCD_SetTextColor(LCD_COLOR_GREEN);
    BSP_LCD_SetFont(&Font24);

    BSP_LCD_DisplayStringAt(0, LINE(2), (uint8_t*)"Accelerometer Features", CENTER_MODE);
    BSP_LCD_DisplayStringAt(0, LINE(4), (uint8_t*)"Collecting...", CENTER_MODE);

    // Serial header
    printf("Feature extraction (DISCO-F746NG) - window=%d step=%d SR=%.1fHz\n", WINDOW_SIZE, WINDOW_STEP, SAMPLE_RATE_HZ);

    // buffers
    std::vector<float> bx, by, bz;
    bx.reserve(WINDOW_SIZE*2); by.reserve(WINDOW_SIZE*2); bz.reserve(WINDOW_SIZE*2);

    int window_id = 0;
    while(true){
        // read sample
        float ax, ay, az;
        if(USE_SIMULATION) read_accel_sim(ax, ay, az);
        else {
            // TODO: replace with real sensor read (I2C/SPI) and convert to g
            ax = ay = az = 0.0f;
        }

        bx.push_back(ax); by.push_back(ay); bz.push_back(az);

        // rate control
        ThisThread::sleep_for((int)(dt*1000.0f));

        // if enough samples
        if((int)bx.size() >= WINDOW_SIZE){
            // take window
            std::vector<float> wx(bx.begin(), bx.begin()+WINDOW_SIZE);
            std::vector<float> wy(by.begin(), by.begin()+WINDOW_SIZE);
            std::vector<float> wz(bz.begin(), bz.begin()+WINDOW_SIZE);

            MiniFeatures mf = extract_mini(wx, wy, wz, window_id++);
            print_csv(mf);

            if (window_id >= 100) { 
                
                printf("Completed 100 windows. Stopping.\n"); while (true) ThisThread::sleep_for(1s); // ya da break; ve program sonu

            }

            // LCD: show a few key features
            char linebuf[64];
            snprintf(linebuf, sizeof(linebuf), "WID: %d  mean_x: %.2f g", mf.wid, mf.mean_x);
            lcd_print_line(6, linebuf);
            snprintf(linebuf, sizeof(linebuf), "mean_y: %.2f g  mean_z: %.2f g", mf.mean_y, mf.mean_z);
            lcd_print_line(7, linebuf);
            snprintf(linebuf, sizeof(linebuf), "rms_x: %.2f g  rms_y: %.2f g", mf.rms_x, mf.rms_y);
            lcd_print_line(8, linebuf);
            snprintf(linebuf, sizeof(linebuf), "rms_z: %.2f g", mf.rms_z);
            lcd_print_line(9, linebuf);

            // slide (pop front WINDOW_STEP)
            int pop = WINDOW_STEP;
            if(pop >= (int)bx.size()){
                bx.clear(); by.clear(); bz.clear();
            } else {
                bx.erase(bx.begin(), bx.begin()+pop);
                by.erase(by.begin(), by.begin()+pop);
                bz.erase(bz.begin(), bz.begin()+pop);
            }
        }
    }
}
