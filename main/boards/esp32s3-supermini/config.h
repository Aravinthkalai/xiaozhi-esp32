#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

// ============================================================
// ESP32-S3 SuperMini
// MAX98357A + INMP441 + SSD1306 128x64 OLED
// 4MB Flash / 2MB PSRAM / 512KB SRAM
// ============================================================

#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// Simplex I2S mode
#define AUDIO_I2S_METHOD_SIMPLEX

// INMP441 Microphone
#define AUDIO_I2S_MIC_GPIO_WS    GPIO_NUM_4
#define AUDIO_I2S_MIC_GPIO_SCK   GPIO_NUM_5
#define AUDIO_I2S_MIC_GPIO_DIN   GPIO_NUM_6

// MAX98357A Speaker
#define AUDIO_I2S_SPK_GPIO_DOUT  GPIO_NUM_11
#define AUDIO_I2S_SPK_GPIO_BCLK  GPIO_NUM_12
#define AUDIO_I2S_SPK_GPIO_LRCK  GPIO_NUM_13

// SSD1306 OLED
#define DISPLAY_SDA_PIN          GPIO_NUM_7
#define DISPLAY_SCL_PIN          GPIO_NUM_8
#define DISPLAY_WIDTH            128
#define DISPLAY_HEIGHT           64
#define DISPLAY_MIRROR_X         false
#define DISPLAY_MIRROR_Y         false

// Buttons
#define BOOT_BUTTON_GPIO         GPIO_NUM_0
#define TOUCH_BUTTON_GPIO        GPIO_NUM_NC
#define VOLUME_UP_BUTTON_GPIO    GPIO_NUM_NC
#define VOLUME_DOWN_BUTTON_GPIO  GPIO_NUM_NC

// Onboard RGB LED
#define BUILTIN_LED_GPIO         GPIO_NUM_48

#endif // _BOARD_CONFIG_H_