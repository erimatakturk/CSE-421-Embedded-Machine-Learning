#include "mbed.h"
#include "stm32f7xx_hal.h"
#include "stm32746g_discovery_lcd.h"
#include "stm32746g_discovery_ts.h"

FileHandle *mbed::mbed_override_console(int){
    static UnbufferedSerial pc(USBTX, USBRX, 115200);
    return &pc;
}

ADC_HandleTypeDef hadc1;

static void adc1_init_for_temp() {
    __HAL_RCC_ADC1_CLK_ENABLE();
    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV4;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DMAContinuousRequests = DISABLE;
    hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
    HAL_ADC_DeInit(&hadc1);
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
}

static uint16_t adc_read_raw_once() {
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint16_t raw = (uint16_t)HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return raw;
}

static float raw_to_celsius(uint16_t raw) {
    const float VREF = 3.3f, ADC_MAX = 4095.0f, V25 = 0.76f, AVG_SLOPE = 0.0025f;
    float vsense = (raw / ADC_MAX) * VREF;
    return ((vsense - V25) / AVG_SLOPE) + 25.0f;
}

int main() {

    BSP_LCD_Init();
    BSP_LCD_LayerDefaultInit(0, LCD_FB_START_ADDRESS);
    BSP_LCD_SelectLayer(0);
    BSP_LCD_Clear(LCD_COLOR_WHITE);
    BSP_LCD_SetBackColor(LCD_COLOR_WHITE);
    BSP_LCD_SetTextColor(LCD_COLOR_BLACK);
    BSP_LCD_DisplayStringAt(0, 10, (uint8_t *)"STM32F746G Sicaklik Sensoru", CENTER_MODE);
    
    ThisThread::sleep_for(1000ms);
    printf("STM32F746G Discovery Dahili Sicaklik Sensoru\n");

    adc1_init_for_temp();

    const int N = 10;
    uint32_t sum_raw = 0;
    for (int i = 0; i < N; ++i) {
        uint16_t r = adc_read_raw_once();
        sum_raw += r;
        printf("Okuma %02d: %.2f C\n", i+1, raw_to_celsius(r));
        ThisThread::sleep_for(200ms);
    }
    float avg = raw_to_celsius((uint16_t)((float)sum_raw / N));
    printf("------------------------------\n");
    printf("Ortalama Sicaklik: %.2f C\n", avg);
    char buffer[64];
    sprintf(buffer, "Ortalama Sicaklik: %.2f C", avg);
    BSP_LCD_DisplayStringAt(0, 100, (uint8_t *)buffer, CENTER_MODE);
}