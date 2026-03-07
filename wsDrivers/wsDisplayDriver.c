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
    
    if (_x > 1.f)  
    {
        f_x = 1.f;
    }
    else if (_x<-1.f)
    {
        f_x = -1.f;
    }
    
    if (_y > 1.f)  
    {
        f_y = 1.f;
    }
    else if (_y<-1.f)
    {
        f_y = -1.f;
    }

    if (_x >= 0.00f)
    {
        x = ((X_CENTER-1)+((float)X_CENTER*f_x));
    }
    else
    {
        x = ((X_CENTER)+((float)X_CENTER*f_x));
    }

    if (_y >= 0.00f)
    {
        y = ((Y_CENTER-1)+((float)Y_CENTER*f_y));
    }
    else
    {
        y = ((Y_CENTER)+((float)Y_CENTER*f_y));
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

int Build2DLine(const vertex2D _vertexList[2])
{
    //checks if the buffer pointers have been inited
    if (NULL == FrameBuffer0 || NULL == FrameBuffer1) 
    {
        return -1;
    }

    unsigned short X_Max = 0;
    unsigned short Y_Max = 0;
    unsigned short X_Min = 0;
    unsigned short Y_Min = 0;

    unsigned short V0_X = 0;
    unsigned short V0_Y = 0;
    unsigned short V1_X = 0;
    unsigned short V1_Y = 0;

    //Assigns the properties above to the corrisponding values so that the next functions can draw the proper lines
    (void)NormalToFramBufferArray(_vertexList[0].Vertex.X,_vertexList[0].Vertex.Y,&V0_X,&V0_Y);
    (void)NormalToFramBufferArray(_vertexList[1].Vertex.X,_vertexList[1].Vertex.Y,&V1_X,&V1_Y);

    //assigns the min max values of x and y for the the two vertexes
    if (V1_X > V0_X)
    {
        X_Min = V0_X;
        X_Max = V1_X;
    }
    else
    {
        X_Max = V0_X;
        X_Min = V1_X;
    }

    if (V1_Y > V0_Y)
    {
        Y_Min = V0_Y;
        Y_Max = V1_Y;
    }
    else
    {
        Y_Max = V0_Y;
        Y_Min = V1_Y;
    }

    //trackers for the screen coordiniets (pixel locaion)
    uint16_t y = 0;
    uint16_t x = 0;

    //checks to make sure the slope is not 0
    if (0 == V1_Y-V0_Y)
    {
        for (x = X_Min; x <= X_Max; x++)
        {
            y = Y_Min;

            ((RGB16*)FrameBuffer1)[ToFramBufferArray(x,y)] = _vertexList[0].Color;
        }
    }
    //checks to make sure the slop is not infinet
    else if (0 == V1_X-V0_X)
    {
        for (y = Y_Min; y <= Y_Max; y++)
        {
            x = X_Min;

            ((RGB16*)FrameBuffer1)[ToFramBufferArray(x,y)] = _vertexList[0].Color;
        }
    }
    //uses the slope formula to check pixel placement [y = {(y2-y1)/(x2-x1)*x+b}]
    else
    {
        float DaSlope = (float)(V1_Y-V0_Y)/(float)(V1_X-V0_X);
        //scans through the min and max x values to plot the apporiate y value
        for (x = X_Min; x <= X_Max; x++)
        {
            y = ((DaSlope)*(x-V0_X))+V0_Y;

            //incerts the coordinent to frame buffer 1
            ((RGB16*)FrameBuffer1)[ToFramBufferArray(x,y)] = _vertexList[0].Color;
        }
    }
    
    //make sure that the lcd_bitmap start and stop values are different
    X_Max++;
    Y_Max++;

    //loads fram buffer 1 into frame buffer 0 
    (void)esp_lcd_panel_draw_bitmap(waveShareDisplay_panel,X_Min,Y_Min,X_Max,Y_Max,FrameBuffer1);

    return 0;
}

int Build3DLine(const vertex3D _vertexList[2])
{
    //checks if the buffer pointers have been inited
    if (NULL == FrameBuffer0 || NULL == FrameBuffer1) 
    {
        return -1;
    }

    return 1;
}

int Build2DShape(const mesh2D* _mesh)
{
    //checks if the buffer pointers have been inited
    if (NULL == FrameBuffer0 || NULL == FrameBuffer1) 
    {
        return -1;
    }

    triangle2D* Triangle = NULL;

    for (size_t m = 0; m < _mesh->TriangleCount; m++)
    {
        Triangle = _mesh->TriangleList;
        vertex2D Line1[2] = {Triangle[m].vertex0,Triangle[m].vertex1};
        vertex2D Line2[2] = {Triangle[m].vertex1,Triangle[m].vertex2};
        vertex2D Line3[2] = {Triangle[m].vertex2,Triangle[m].vertex0};
        
        Build2DLine(Line1);
        Build2DLine(Line2);
        Build2DLine(Line3);
    }

    Triangle = NULL;

    return 0;
}
int Build3DShape(const mesh3D* _mesh)
{
    //checks if the buffer pointers have been inited
    if (NULL == FrameBuffer0 || NULL == FrameBuffer1) 
    {
        return -1;
    }
    return 1;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           HELPER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Allows number to be used to set the RGB16 value
 */
RGB16 ToRGB16(unsigned int _red, unsigned int _green, unsigned int _blue)
{
    if (_red > 31 || _green > 63 || _blue > 31)
    {
        abort();
    }
    
    RGB16 colorCode = 0;
        //bitshifts the _color to the proper bitspace
    colorCode = ((_blue << 11) | (_green << 5) | _red);

    return colorCode;
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
