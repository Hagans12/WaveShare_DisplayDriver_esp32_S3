/**
 * ©Connell T Hagans II 2026
 * will be uploading code to get hub so there is more public information abouty writing drivers for displays
 * Display: https://www.crystalfontz.com/product/cfaf800480e1050sc-800x480-5-inch-color-tft
 * Driver mode: rgb 565 (16 bit colour)
 * Using the esp32-s3
 * The only way to drive the LCD display is by using the LCD API provided by espressif sinc it envolves direct register access that espressif /
 *      restricts for security reasons(wifi/bluetooth security)
 * RGB565 format will be used
 * This driver is not fully fleshed out and the device may need to be reset a couple of times befor the display shows up again
 */
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           HEADER GUARD
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef WAVESHARE_DISPLAY_DRIVER_H
#define WAVESHARE_DISPLAY_DRIVER_H

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           USER DEFINES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Color and Control Pin Hookups
 * set pins that are not connected to '-1'
 * MAKE SURE THESE PINS DO NOT GET USE ANYWARE ELSE
 */

//RED
    #define R3      1
    #define R4      2
    #define R5      42
    #define R6      41
    #define R7      40
//GREEN
    #define G2      39
    #define G3      0
    #define G4      45
    #define G5      48
    #define G6      47
    #define G7      21
//BLUE
    #define B3      14
    #define B4      38
    #define B5      18
    #define B6      17
    #define B7      10
//CONTROL
    #define D_CLK   7   //PIXIEL / DATA CLOCL (6 is used for testing and 7 for actual display)
    #define H_SYNC  46  //HORIZONTIL SYNC
    #define V_SYNC  3   //VIRTICAL SYNC
    #define DE      5   //DATA ENABLE

/**
 * Definitions for the physical display
 * The dcumentation got me mostly there but the front and back porch had to be played with. same with the D_CLCK_SPEED
 */
//TIMING
    #define D_CLK_SPEED     16*1000*1000 // must between 23MHz - 14MHz

    #define H_COUNT         800
    #define H_FRONTPORCH    48          //this number ended up being mor stable
    #define H_BACKPORCH     8
    #define H_PULSWIDTH     4

    #define V_COUNT         480
    #define V_FRONTPORCH    10
    #define V_BACKPORCH     10
    #define V_PULSWIDTH     4

//DATA
    #define NUM_OF_FRAMEBUFFERS 2

// COLOR FORMAT (Number of data lines for each color in a pixel)
    #define RGB888 (8+8+8)
    #define RGB565 (5+6+5)  //realisticly the only one you will need for this project but the others were added as a demastration
    #define RGB666 (6+6+6)


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           CONSTRUCTOR AND DESTRUCTOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void WSDisplayDriver(void);     //init of display driver
/**
 * This function must be called befor shutting down the display physically. errors will still pop up but they show up more with out this being called
 */
void _WSDisplayDriver_(void);   //deinit of display driver


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PUBLIC VARIABLES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PUBLIC FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * this can be called after WSDisplayDriver() has been called
 * will return 1 if ran properly
 * @return -1 = Display not properly inted
 * @return 0 if already running
 * @return 1 is called properly
 */
int TurnOnDisplay(void);

/**
 * this can be called after WSDisplayDriver() and TurnOnDisplay() has been called
 * will return 1 if ran properly
 * @return -1 = Display not properly inted
 * @return 0 if already off
 * @return 1 is called properly
 */
int TurnOffDisplay(void);
    

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           GETTERS AND SETTERS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//GETTERS

//SETTERS


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           TESTER FUNCTIONS (*REMOVE BEFOR FLIGHT*)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int t_initDisplayTest(void); // just a test fuction to turn the screen with a color range test. Should be called alone(set D_CLK_SPEED to 16MHz for clearer resualts)
int t_DisplayBufferTest(void); // just a test fuction to help pin point the for corners and the center. Should be called alone

#endif