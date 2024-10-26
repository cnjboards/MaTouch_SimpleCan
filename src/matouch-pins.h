#pragma once

#if defined(STATION_A)
// Defines for MaTouch 2.1" display
    // disply backlight
    #define TFT_BL 38

    // dial button function
    #define BUTTON_PIN 14 // dial press button gpio

    // dial rotate encoder
    #define ENCODER_CLK 13 // dial rotate CLK (pulses)
    #define ENCODER_DT 10  // dial rotate DT (direction)

    // display I2C pins
    #define I2C_SDA_PIN 17 
    #define I2C_SCL_PIN 18 
#else
    // pin definitions for MaTouch 1.28
    #define TFT_BLK 7
    #define TFT_RES 11

    #define TFT_CS 15
    #define TFT_MOSI 13
    #define TFT_MISO 12
    #define TFT_SCLK 14
    #define TFT_DC 21

    #define TOUCH_INT 40
    #define TOUCH_SDA 38
    #define TOUCH_SCL 39
    #define TOUCH_RST 16

    #define BUTTON_PIN 6
    #define ENCODER_CLK 9 // CLK
    #define ENCODER_DT 10 // DT
#endif