#include "mbed.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

#define USE_EDGEAI 0

extern "C" {
#include "stm32746g_discovery.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"
#include "stm32746g_discovery_sdram.h"
}

#if USE_EDGEAI
extern "C" {
#include "EdgeAI/Inc/ai_platform.h"
#include "EdgeAI/network.h"
#include "EdgeAI/network_data.h"
}
#endif

static constexpr int LCD_W = 480;
static constexpr int LCD_H = 272;

static constexpr int DRAW_X = 10;
static constexpr int DRAW_Y = 20;
static constexpr int DRAW_S = 240;

static constexpr int BTN_X = 270;
static constexpr int BTN_W = 200;
static constexpr int BTN_H = 50;
static constexpr int BTN_CLEAR_Y = 60;
static constexpr int BTN_RECOG_Y = 130;

static uint8_t canvas[DRAW_S * DRAW_S];

static bool in_rect(int x, int y, int rx, int ry, int rw, int rh) {
    return (x >= rx && x < rx + rw && y >= ry && y < ry + rh);
}
static void draw_button(int x, int y, const char* text) {
    BSP_LCD_SetTextColor(LCD_COLOR_DARKGRAY);
    BSP_LCD_FillRect(x, y, BTN_W, BTN_H);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_DrawRect(x, y, BTN_W, BTN_H);
    BSP_LCD_DisplayStringAt(x + 10, y + 15, (uint8_t*)text, LEFT_MODE);
}
static void clear_status_line() {
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_FillRect(BTN_X, 230, BTN_W, 30);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
}
static void show_status(const char* msg, uint32_t color) {
    clear_status_line();
    BSP_LCD_SetTextColor(color);
    BSP_LCD_DisplayStringAt(BTN_X, 230, (uint8_t*)msg, LEFT_MODE);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
}
static void clear_canvas() {
    memset(canvas, 0, sizeof(canvas));
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_FillRect(DRAW_X, DRAW_Y, DRAW_S, DRAW_S);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);
    BSP_LCD_DrawRect(DRAW_X, DRAW_Y, DRAW_S, DRAW_S);
}
static void draw_brush(int x, int y, int r) {
    int cx = x - DRAW_X;
    int cy = y - DRAW_Y;
    if (cx < 0 || cy < 0 || cx >= DRAW_S || cy >= DRAW_S) return;

    for (int dy = -r; dy <= r; ++dy) {
        for (int dx = -r; dx <= r; ++dx) {
            if (dx * dx + dy * dy > r * r) continue;
            int px = cx + dx;
            int py = cy + dy;
            if (px < 0 || py < 0 || px >= DRAW_S || py >= DRAW_S) continue;
            canvas[py * DRAW_S + px] = 255;
            BSP_LCD_DrawPixel(DRAW_X + px, DRAW_Y + py, LCD_COLOR_WHITE);
        }
    }
}

static bool find_bbox(int &minx, int &miny, int &maxx, int &maxy) {
    minx = DRAW_S; miny = DRAW_S; maxx = -1; maxy = -1;
    for (int y = 0; y < DRAW_S; ++y) for (int x = 0; x < DRAW_S; ++x) {
        if (canvas[y * DRAW_S + x] > 0) {
            if (x < minx) minx = x;
            if (y < miny) miny = y;
            if (x > maxx) maxx = x;
            if (y > maxy) maxy = y;
        }
    }
    return (maxx >= 0);
}

static void downsample_28(uint8_t out28[28][28]) {
    memset(out28, 0, 28 * 28);
    int minx, miny, maxx, maxy;
    if (!find_bbox(minx, miny, maxx, maxy)) return;

    const int pad = 6;
    minx = (minx - pad < 0) ? 0 : (minx - pad);
    miny = (miny - pad < 0) ? 0 : (miny - pad);
    maxx = (maxx + pad >= DRAW_S) ? (DRAW_S - 1) : (maxx + pad);
    maxy = (maxy + pad >= DRAW_S) ? (DRAW_S - 1) : (maxy + pad);

    int bw = maxx - minx + 1, bh = maxy - miny + 1;
    int side = (bw > bh) ? bw : bh;
    int cx = (minx + maxx) / 2, cy = (miny + maxy) / 2;

    int sx0 = cx - side / 2, sy0 = cy - side / 2;
    if (sx0 < 0) sx0 = 0; if (sy0 < 0) sy0 = 0;
    if (sx0 + side >= DRAW_S) sx0 = DRAW_S - 1 - side;
    if (sy0 + side >= DRAW_S) sy0 = DRAW_S - 1 - side;
    if (sx0 < 0) sx0 = 0; if (sy0 < 0) sy0 = 0;

    for (int oy = 0; oy < 28; ++oy) for (int ox = 0; ox < 28; ++ox) {
        int x0 = sx0 + (ox * side) / 28;
        int x1 = sx0 + ((ox + 1) * side) / 28;
        int y0 = sy0 + (oy * side) / 28;
        int y1 = sy0 + ((oy + 1) * side) / 28;
        if (x1 <= x0) x1 = x0 + 1;
        if (y1 <= y0) y1 = y0 + 1;

        int sum = 0, cnt = 0;
        for (int y = y0; y < y1; ++y) for (int x = x0; x < x1; ++x) {
            sum += (canvas[y * DRAW_S + x] > 0) ? 1 : 0;
            cnt++;
        }
        int perc = (cnt > 0) ? (sum * 100 / cnt) : 0;
        out28[oy][ox] = (perc >= 20) ? 1 : 0;
    }
}

static void projections(const uint8_t img[28][28], int rowSum[28], int colSum[28]) {
    for (int i = 0; i < 28; ++i) { rowSum[i] = 0; colSum[i] = 0; }
    for (int y = 0; y < 28; ++y) for (int x = 0; x < 28; ++x) {
        rowSum[y] += img[y][x];
        colSum[x] += img[y][x];
    }
}

static void centroid(const uint8_t img[28][28], float &cx, float &cy, int &mass) {
    int sx = 0, sy = 0; mass = 0;
    for (int y = 0; y < 28; ++y) for (int x = 0; x < 28; ++x) {
        if (img[y][x]) { sx += x; sy += y; mass++; }
    }
    if (mass == 0) { cx = 14.f; cy = 14.f; return; }
    cx = (float)sx / (float)mass;
    cy = (float)sy / (float)mass;
}

static int hole_score_center(const uint8_t img[28][28]) {
    int empty = 0;
    for (int y = 9; y <= 18; ++y) for (int x = 9; x <= 18; ++x)
        if (img[y][x] == 0) empty++;
    return empty;
}

static int rule_recognize_digit() {
    uint8_t img[28][28];
    downsample_28(img);

    int rowSum[28], colSum[28];
    projections(img, rowSum, colSum);

    float cx, cy; int mass;
    centroid(img, cx, cy, mass);
    if (mass < 15) return -1;

    int hs = hole_score_center(img);

    int top = 0, bottom = 0, left = 0, right = 0;
    for (int i = 0; i < 14; ++i) {
        top += rowSum[i];
        bottom += rowSum[27 - i];
        left += colSum[i];
        right += colSum[27 - i];
    }
    int maxCol = 0; for (int x = 0; x < 28; ++x) if (colSum[x] > maxCol) maxCol = colSum[x];
    int topBand = 0; for (int y = 0; y < 6; ++y) topBand += rowSum[y];

    if (hs > 70) {
        if (abs((int)cx - 14) <= 2 && abs((int)cy - 14) <= 2) return 8;
        if (top > bottom && cy < 14) return 9;
        if (bottom > top && cy > 14) return 6;
        return 0;
    }
    if (maxCol >= 18 && mass < 220) return 1;
    if (topBand > 20 && top > bottom && right > left) return 7;
    if (cx > 15 && bottom > top && right > left) return 4;
    if (right > left + 10 && abs(top - bottom) < 20) return 3;
    if (left > right + 10 && top > bottom) return 5;
    if (top > bottom && cx > 14) return 2;
    return 2;
}

#if USE_EDGEAI
static ai_handle g_net = AI_HANDLE_NULL;
static bool edgeai_ready = false;

static bool edgeai_init() {

    edgeai_ready = false;
    return false;
}

static int edgeai_predict_digit() {
    return -1;
}
#endif

int main() {
    BufferedSerial pc(USBTX, USBRX, 115200);

    BSP_SDRAM_Init();

    BSP_LCD_Init();
    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
    BSP_LCD_SelectLayer(0);
    BSP_LCD_Clear(LCD_COLOR_BLACK);
    BSP_LCD_SetBackColor(LCD_COLOR_BLACK);
    BSP_LCD_SetTextColor(LCD_COLOR_WHITE);

    BSP_LCD_DisplayStringAt(0, 0, (uint8_t*)"DRAW DIGIT (Rule + EdgeAI opt)", CENTER_MODE);

    if (BSP_TS_Init(LCD_W, LCD_H) != TS_OK) {
        BSP_LCD_DisplayStringAt(0, LINE(5), (uint8_t*)"Touch init FAILED", CENTER_MODE);
        while (true) ThisThread::sleep_for(1s);
    }

    clear_canvas();
    draw_button(BTN_X, BTN_CLEAR_Y, "CLEAR");
    draw_button(BTN_X, BTN_RECOG_Y, "RECOG");
    BSP_LCD_DisplayStringAt(BTN_X, 200, (uint8_t*)"Output:", LEFT_MODE);
    show_status("ready", LCD_COLOR_CYAN);

#if USE_EDGEAI
    edgeai_init();
    if (edgeai_ready) show_status("EdgeAI ready", LCD_COLOR_GREEN);
#endif

    TS_StateTypeDef st {};
    int last_x = -1, last_y = -1;

    while (true) {
        BSP_TS_GetState(&st);

        if (st.touchDetected) {
            int x = st.touchX[0];
            int y = st.touchY[0];

            if (in_rect(x, y, BTN_X, BTN_CLEAR_Y, BTN_W, BTN_H)) {
                clear_canvas();
                show_status("cleared", LCD_COLOR_YELLOW);
                last_x = last_y = -1;
                ThisThread::sleep_for(250ms);
            } else if (in_rect(x, y, BTN_X, BTN_RECOG_Y, BTN_W, BTN_H)) {
                int d = -1;

#if USE_EDGEAI
                if (edgeai_ready) d = edgeai_predict_digit();
#endif
                if (d < 0) d = rule_recognize_digit(); // fallback

                if (d < 0) {
                    show_status("draw a digit", LCD_COLOR_RED);
                } else {
                    char msg[32];
                    snprintf(msg, sizeof(msg), "digit=%d", d);
                    show_status(msg, LCD_COLOR_GREEN);
                }

                ThisThread::sleep_for(250ms);
            } else if (in_rect(x, y, DRAW_X, DRAW_Y, DRAW_S, DRAW_S)) {
                if (last_x >= 0) {
                    int steps = 6;
                    for (int i = 1; i <= steps; ++i) {
                        int ix = last_x + (x - last_x) * i / steps;
                        int iy = last_y + (y - last_y) * i / steps;
                        draw_brush(ix, iy, 6);
                    }
                } else {
                    draw_brush(x, y, 6);
                }
                last_x = x;
                last_y = y;
            }
        } else {
            last_x = last_y = -1;
        }

        ThisThread::sleep_for(10ms);
    }
}
