/**
 * ©Connell T Hagans II 2026
 * EDITING THIS FILE CAN BREAK THIS PROGRAM AND SHOULD BE DONE IN CAUSTION IF AT ALL
 * will be uploading code to get hub so there is more public information abouty writing drivers for displays
 * Display: https://www.crystalfontz.com/product/cfaf800480e1050sc-800x480-5-inch-color-tft
 * Driver mode: rgb 565 (16 bit colour)
 * D_CLK & DE are positive polarity (refer to data sheet ST7262 Datasheet timing graph)
 * H_SYNC & V_SYNC are negitive polarity (refer to data sheet ST7262 Datasheet timing graph)
 * '(void)' is put infront of every esp-idf function call because im not checking for errors or using the esp-idf error logging functionality
 * The only way to drive the LCD display is by using the LCD API (esp_lcd_panel_rgb.h) provided by espressif sinc it envolves direct register access that espressif /
 *      restricts for security reasons(wifi/bluetooth security)
 */
#include "wsDisplayDriver.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PRIVITE INCLUDES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//STANDARD INCLUDES
#include <stdio.h>

//ESP INCLUDES
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_attr.h"

#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_ops.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PRIVITE DEFINES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//COLORS
#define RED         0X001F
#define GREEN       0X07E0
#define BLUE        0XF800
#define MAGINTA     0XF81F
#define YELLOW      0X07FF
#define CYAN        0XFFE0
#define WHITE       0XFFFF
#define BLACK       0X0000
#define BROWN       0X0253

//FRAME BUFFER
#define FRAMEBUFFERNUM 2
#define BUFFERSIZE (H_COUNT*V_COUNT)
    //must have a least one full row of pixels (800 or H_COUNT)
#define BOUNCEBUFFER (H_COUNT*5 )
    //is used for flushing the whole screen as long as the end number is a whole number
#define BOUNCECOUNT (BUFFERSIZE/BOUNCEBUFFER)

#define X_COUNT H_COUNT
#define Y_COUNT V_COUNT

//COLOR FORMAT (Number of data lines for each color in a pixel)
#define RGB565 (5+6+5)  //realisticly the only one you will need for this project but the others were added as a demastration

//HOMOGENEOUS COORDINATES
#define X_CENTER (X_COUNT/2)
#define Y_CENTER (Y_COUNT/2)

//ASPECT RATIO
#define ASPECTRATIO_X (ASPECTRATIO_WIDTH*ASPECTRATIO_MULTIPLIER)
#define ASPECTRATIO_Y (ASPECTRATIO_HIGHT*ASPECTRATIO_MULTIPLIER)

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PRIVITE VARIABLES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//DISPLAY CONTROLS
    //is used to make sure every thing else below is in order
static _Bool _isDisplayInit = 0;
    //is used to check if D_CLK is running or is paused
static _Bool _isDisplayRunning = 0;

    //the control handle for the display
esp_lcd_panel_handle_t waveShareDisplay_panel = NULL;
    //configureation setting for waveShareDisplay_panel
esp_lcd_rgb_panel_config_t rgb_panel_config =
{
    .clk_src = LCD_CLK_SRC_DEFAULT, //Select PLL_F160M as the default choice
    .timings = //sets the timings FOR THE LCD signals
    {
        .pclk_hz            = D_CLK_SPEED,
        .h_res              = H_COUNT,
        .hsync_back_porch   = H_BACKPORCH,
        .hsync_front_porch  = H_FRONTPORCH,
        .hsync_pulse_width  = H_PULSWIDTH,
        .v_res              = V_COUNT,
        .vsync_back_porch   = V_BACKPORCH,
        .vsync_front_porch  = V_FRONTPORCH,
        .vsync_pulse_width  = V_PULSWIDTH,
        .flags.pclk_active_neg =1,
    }, 
    .data_width = RGB565,
    .bits_per_pixel = RGB565,
    .de_gpio_num = DE,
    .hsync_gpio_num = H_SYNC,
    .vsync_gpio_num = V_SYNC,
    .pclk_gpio_num =  D_CLK,
    .disp_gpio_num = -1, // not in use
    .data_gpio_nums = {R3,R4,R5,R6,R7,G2,G3,G4,G5,G6,G7,B3,B4,B5,B6,B7},
    .psram_trans_align = DMABURSTSIZE,
    .sram_trans_align = DMABURSTSIZE,
    .flags.fb_in_psram = true, // allocate frame buffers from PSRAM
    .num_fbs = FRAMEBUFFERNUM,
    .bounce_buffer_size_px = BOUNCEBUFFER,
    //.bounce_buffer_size_px = 0,
};

//FRAMER BUFFER
    //will get address from esp lcd function [fb0] (Will be the frame buffer that is displaied)
void* FrameBuffer0 = NULL;
    //will get address from esp lcd function [fb1] (Will be the buffer that is drawn to then uploaded to the main frame buffer)
void* FrameBuffer1 = NULL;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PRIVITE INITS AND DEINITS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//FRAME BUFFER

/**
 * releases the adress of the frame buffer
 */
void deinit_FrameBuffer(void)
{
    FrameBuffer0 = NULL;
    FrameBuffer1 = NULL;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PRIVITE FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//FRAME BUFFER FUNCTIONS:

/**
 * is used in relation to a for loop or a 2d array and output a corrisponding 1d frame buffer array position
 * will abort if out side the _x or _y is bigger than H_COUNT or V_COUNT respectively
 */
unsigned int ToFramBufferArray(const unsigned int _x, const unsigned int _y)
{
    unsigned int x = _x;
    unsigned int y = _y;
    if (_x >= X_COUNT)  
    {
        x = X_COUNT;
    }

    if (_y >= Y_COUNT)  
    {
        y = Y_COUNT;
    }
    
    return ((x)) + ((X_COUNT) * (y));
}

/**
 * is used in relation to a for loop or a 2d array and output a corrisponding 1d frame buffer array position
 */
unsigned int NormalToFramBufferArray(const float _x, const float _y, unsigned short* out_x, unsigned short* out_y)
{
    unsigned int x = 0;
    unsigned int y = 0;
    float f_x = 0.f;
    float f_y = 0.f;

    f_x = _x;
    f_y = _y;
    
    if (_x > (float)ASPECTRATIO_X)  
    {
        f_x = (float)ASPECTRATIO_X;
    }
    else if (_x<-(float)ASPECTRATIO_X)
    {
        f_x = -(float)ASPECTRATIO_X;
    }
    
    if (_y > (float)ASPECTRATIO_Y)  
    {
        f_y = (float)ASPECTRATIO_Y;
    }
    else if (_y<-(float)ASPECTRATIO_Y)
    {
        f_y = -(float)ASPECTRATIO_Y;
    }

    if (_x >= 0.00f)
    {
        x = ((X_CENTER-1)+((float)(X_CENTER/ASPECTRATIO_X)*f_x));
    }
    else
    {
        x = ((X_CENTER)+((float)(X_CENTER/ASPECTRATIO_X)*f_x));
    }

    if (_y >= 0.00f)
    {
        y = ((Y_CENTER-1)+((float)(Y_CENTER/ASPECTRATIO_Y)*f_y));
    }
    else
    {
        y = ((Y_CENTER)+((float)(Y_CENTER/ASPECTRATIO_Y)*f_y));
    }
    
    *out_x = x;
    *out_y = y;
    
    return ((x)) + ((X_COUNT) * (y));
}

/**
 * makes the proper calls and sitches internal vars to the opporiate values
 */
int TurnOnDisplay(void)
{
    //is checked first to make sure the display has been inited
    if (0 == _isDisplayInit)
    {
        return -1;
    }

    if (1 == _isDisplayInit && 0 == _isDisplayRunning)
    {
        //calls ESP-IDF lcd internal code to turn on display signaals
        (void)esp_lcd_panel_disp_on_off(waveShareDisplay_panel,1);
        _isDisplayRunning =1;

        //asigns frame buffer pointers
        if (1 == FRAMEBUFFERNUM)
        {
            (void)esp_lcd_rgb_panel_get_frame_buffer(waveShareDisplay_panel,FRAMEBUFFERNUM,&FrameBuffer0);
        }
        else if (2 == FRAMEBUFFERNUM)
        {
            (void)esp_lcd_rgb_panel_get_frame_buffer(waveShareDisplay_panel,FRAMEBUFFERNUM,&FrameBuffer0, &FrameBuffer1);
        }
        
        return 0;
    }

    //does nothing is the display is already running
    if (1 == _isDisplayInit && 1 == _isDisplayRunning)
    {
        return 1;
    }

    //only returns if something unexspected happens, but this should never be called
    return -404;
}

/**
 * makes the proper calls and sitches internal vars to the opporiate values
 */
int TurnOffDisplay()
{
    //is checked first to make sure the display has been inited
    if (0 == _isDisplayInit)
    {
        return -1;
    }

    if (1 == _isDisplayInit && 1 == _isDisplayRunning)
    {
        //calls ESP-IDF lcd internal code to turn off display signaals
        (void)esp_lcd_panel_disp_on_off(waveShareDisplay_panel,0);
        
        // re-asigns the frame buffer pointers to NULL to makesure nothing funcy is going on
        deinit_FrameBuffer();
        _isDisplayRunning = 0;

        return 0;
    }

    // doesnothing is the display is already off
    if (1 == _isDisplayInit && 0 == _isDisplayRunning)
    {
        return 1;
    }

    //only returns if something unexspected happens, but this should never be called
    return -404;
}

void DrawLine(Vec2D _v1, Vec2D _v2)//WIP
{
    float XMin = _v1.X;
    float XMax = _v2.X;
    float YMin = _v1.Y;
    float YMax = _v2.Y;

    if (XMax < XMin)
    {
        float tempF = XMax;
        XMax = XMin;
        XMin = tempF;
    }
    if (YMax < YMin)
    {
        float tempF = XMax;
        XMax = XMin;
        XMin = tempF;
    }


    
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PUBLIC STRUCTS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PUBLIC VARIABLES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PUBLIC FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//DRAWING FUNCTIONS:


int FlushColorToDisplay(const RGB16 _color)
{
    //checks if the buffer pointers have been inited
    if (NULL == FrameBuffer0 || NULL == FrameBuffer1) 
    {
        return -1;
    }

    //writes to framebuffer1
    for (size_t y = 0; y < V_COUNT; y++)
    {
        for (size_t x = 0; x < H_COUNT; x++)
        {
            ((RGB16*)FrameBuffer1)[ToFramBufferArray(x,y)] = _color;
        }
    }
    //LOADS FRAME BUFFER 1 INTO FRAM BUFFER 0
    (void)esp_lcd_panel_draw_bitmap(waveShareDisplay_panel,0,0,X_COUNT,Y_COUNT,FrameBuffer1);
    
    return 0;
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           HELPER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Allows number to be used to set the RGB16 value
 */
RGB16 ToRGB16( int _red,  int _green, int _blue)
{
    if (_red >0b11111)
    {
        _red = _red &0b11111;
        _red = 0b11111;
    }
    
    if (_red <0)
    {
        _red = _red &0b11111;
        _red = 0;
    }

    if (_green >0b111111)
    {
        _green = _green &0b111111;
        _green = 0b111111;
    }
    
    if (_green <0)
    {
        _green = _green &0b111111;
        _green = 0;
    }

    if (_blue >0b11111)
    {
        _blue = _blue &0b11111;
        _blue = 0b11111;
    }
    
    if (_blue <0)
    {
        _blue = _blue &0b11111;
        _blue = 0;
    }
    
    RGB16 colorCode = 0;
        //bitshifts the _color to the proper bitspace
    colorCode = ((_blue << 11) | (_green << 5) | _red);

    return colorCode;
}

/**
 * 
 */
RGB16 InterpulateRGB16(RGB16 _colorStart, RGB16 _colorEnd, int _positionStart, int _positionEnd, int _positionCurrent)
{

    if (_positionStart == _positionCurrent)
    {
        return _colorStart;
    }
    if (_positionEnd == _positionCurrent)
    {
        return _colorEnd;
    }
    
    uint8_t red1 = 0;
    uint8_t green1 = 0;
    uint8_t blue1 = 0;

    uint8_t red2 = 0;
    uint8_t green2 = 0;
    uint8_t blue2 = 0;

    int8_t redNew = 0;
    int8_t greenNew = 0;
    int8_t blueNew = 0;

    red1 = (0b11111 & _colorStart);
    _colorStart = _colorStart >> 5;
    green1 = (0b111111 & _colorStart);
    _colorStart = _colorStart >> 6;
    blue1 = (0b11111 & _colorStart);

    red2 = (0b11111 & _colorEnd);
    _colorEnd = _colorEnd >> 5;
    green2 = (0b111111 & _colorEnd);
    _colorEnd = _colorEnd >> 6;
    blue2 = (0b11111 & _colorEnd);

    redNew =  red2-red1;
    redNew = (((float)(redNew)/(float)(_positionEnd-_positionStart))*(_positionCurrent-_positionStart)+red1);
    greenNew = green2-green1;
    greenNew = (((float)(greenNew)/(float)(_positionEnd-_positionStart))*(_positionCurrent-_positionStart)+green1);
    blueNew = blue2-blue1;
    blueNew = (((float)(blueNew)/(float)(_positionEnd-_positionStart))*(_positionCurrent-_positionStart)+blue1);


    return ToRGB16(
        (redNew), //red
        (greenNew), //green
        (blueNew) //blue
    );
    
    
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           GETTERS AND SETTERS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           CONSTRUCTOR AND DESTRUCTORW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * will set internal state as init to make sure everything is running before the driver can do any thing else
 * will set the pins, the timers, and the waveShareDisplay_panel 
 */
void WSDisplayDriver(void)
{
    if (0 == _isDisplayInit)
    {        
        //ESP-IDF code the configers waveShareDisplay_panel
        (void)esp_lcd_new_rgb_panel(&rgb_panel_config,&waveShareDisplay_panel);

        //initualization of waveShareDisplay_panel
        (void)esp_lcd_panel_reset(waveShareDisplay_panel); //is called to clean the display
        (void)esp_lcd_panel_init(waveShareDisplay_panel);
        
        //finished
        _isDisplayInit =1;
    }
}

/**
 * will make sure the display screen is turned off first
 * will set internal state as deinit and call proper ESP-IDF and personal codes
 * will release the pins, the timers, and the waveShareDisplay_panel 
 */
void _WSDisplayDriver_(void)
{
    (void)TurnOffDisplay();
    //is called to clean the display
    (void)esp_lcd_panel_reset(waveShareDisplay_panel);
    //releases the pins and timers of the waveShareDisplay_panel
    (void)esp_lcd_panel_del(waveShareDisplay_panel);

    //finished
    _isDisplayInit =0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           TEST AREA (*REMOVE BEFOR FLIGHT*)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int tFlushDisplay(RGB16 _color)
{
    WSDisplayDriver();
    (void)TurnOnDisplay();

    int _resualt = FlushColorToDisplay(_color);

    return _resualt;
}


int tDrawSomething()
{
    WSDisplayDriver();
    int _resualt = TurnOnDisplay();

    Vec3D p1 = {3.f,3.f,1.f};
    Vec3D p2 = {-3.f,3.f,1.f};
    Vec3D p3 = {3.f,-3.f,1.f};
    Vec3D p4 = {-3.f,-3.f,1.f};
    Vec3D p1b = {3.f,3.f,1.2};
    Vec3D p2b = {-3.f,3.f,1.2};
    Vec3D p3b = {3.f,-3.f,1.2};
    Vec3D p4b = {-3.f,-3.f,1.2};

    //checks if the buffer pointers have been inited
    if (NULL == FrameBuffer0 || NULL == FrameBuffer1) 
    {
        return -1;
    }

    //writes to framebuffer1
    uint16_t x = 0;
    uint16_t y = 0;

    NormalToFramBufferArray(p1b.X/p1b.Z,p1b.Y/p1b.Z,&x,&y);
    ((RGB16*)FrameBuffer1)[ToFramBufferArray(x,y)] = CYAN;

    NormalToFramBufferArray(p2b.X/p2b.Z,p2b.Y/p2b.Z,&x,&y);
    ((RGB16*)FrameBuffer1)[ToFramBufferArray(x,y)] = CYAN;

    NormalToFramBufferArray(p3b.X/p3b.Z,p3b.Y/p3b.Z,&x,&y);
    ((RGB16*)FrameBuffer1)[ToFramBufferArray(x,y)] = CYAN;

    NormalToFramBufferArray(p4b.X/p4b.Z,p4b.Y/p4b.Z,&x,&y);
    ((RGB16*)FrameBuffer1)[ToFramBufferArray(x,y)] = CYAN;


    NormalToFramBufferArray(p1.X/p1.Z,p1.Y/p1.Z,&x,&y);
    ((RGB16*)FrameBuffer1)[ToFramBufferArray(x,y)] = CYAN;

    NormalToFramBufferArray(p2.X/p2.Z,p2.Y/p2.Z,&x,&y);
    ((RGB16*)FrameBuffer1)[ToFramBufferArray(x,y)] = CYAN;

    NormalToFramBufferArray(p3.X/p3.Z,p3.Y/p3.Z,&x,&y);
    ((RGB16*)FrameBuffer1)[ToFramBufferArray(x,y)] = CYAN;

    NormalToFramBufferArray(p4.X/p4.Z,p4.Y/p4.Z,&x,&y);
    ((RGB16*)FrameBuffer1)[ToFramBufferArray(x,y)] = CYAN;
  
    //LOADS FRAME BUFFER 1 INTO FRAM BUFFER 0
    (void)esp_lcd_panel_draw_bitmap(waveShareDisplay_panel,0,0,X_COUNT,Y_COUNT,FrameBuffer1);

    return _resualt;
}
