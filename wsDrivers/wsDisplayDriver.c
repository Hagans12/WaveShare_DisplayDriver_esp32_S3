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
unsigned int NormalToFramBufferArray(const float _x, const float _y, short* out_x, short* out_y)
{
    unsigned int x = 0;
    unsigned int y = 0;
    float f_x = 0.f;
    float f_y = 0.f;

    f_x = _x;
    f_y = _y;
    

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


void DrawLine(Vertex _v1, Vertex _v2){
    //vector breakdown
    float X_V1 = _v1.Coordinate.XY.X;
    float X_V2 = _v2.Coordinate.XY.X;
    float Y_V1 = _v1.Coordinate.XY.Y;
    float Y_V2 = _v2.Coordinate.XY.Y;

    //unmodified cordinets
    short X1 = 0;
    short Y1 = 0;

    short X2 = 0;
    short Y2 = 0;


    //ycounting range
    short ithStart = 0;
    short ithEnd = 0;
    //x counting range
    short jthStart = 0;
    short jthEnd = 0;

        //gets the pixel counting range
        //starting pixels
    NormalToFramBufferArray(X_V1,Y_V1,&X1,&Y1);
        //ending pixels
    NormalToFramBufferArray(X_V2,Y_V2,&X2,&Y2);

    //init start and stop points for x and y
    if (Y1 > Y2)
    {
        ithEnd = Y1;
        ithStart = Y2;
    }
    else
    {
        ithEnd = Y2;
        ithStart = Y1;
    }

    if (X1 > X2)
    {
        jthEnd = X1;
        jthStart = X2;
    }
    else
    {
        jthEnd = X2;
        jthStart = X1;
    }
    


    //checks for special slope conditions
    //if slope is a vertical line
    if ((X_V2 - X_V1) == 0.f)
    {
        for (size_t i = ithStart; i <= ithEnd; i++)
        {
            size_t CurrentPixel = ToFramBufferArray(jthEnd,i);
            ((RGB16*)FrameBuffer1)[CurrentPixel] = InterpulateRGB16(_v1.VertexColor,_v2.VertexColor,Y1,Y2,i);
            
        }
    }
    //if slope is a flat line
    else if ((Y_V2 - Y_V1) == 0.f)
    {
        for (size_t j = jthStart; j <= jthEnd; j++)
        {
            size_t CurrentPixel = ToFramBufferArray(j,ithEnd);
            ((RGB16*)FrameBuffer1)[CurrentPixel] = InterpulateRGB16(_v1.VertexColor,_v2.VertexColor,X1,X2,j);
            
        }
    }
    else
    {
        float correctSlope = ((float)(Y_V2 - Y_V1))/((float)(X_V2 - X_V1));

        for (size_t i = ithStart; i <= ithEnd; i++)
        {
            for (size_t j = jthStart; j <= jthEnd; j++)
            {
                if (i == ((correctSlope*(j-X1))+Y1))
                {
                    size_t CurrentPixel = ToFramBufferArray(j,i);
                    RGB16 NewColor = InterpulateRGB16(_v1.VertexColor,_v2.VertexColor,X1,X2,j);
                    ((RGB16*)FrameBuffer1)[CurrentPixel] = NewColor;
                }
                 
                
            }
            
        }
    }

    
    
    //LOADS FRAME BUFFER 1 INTO FRAM BUFFER 0
    (void)esp_lcd_panel_draw_bitmap(waveShareDisplay_panel,0,0,X_COUNT,Y_COUNT,FrameBuffer1);
    
}


void FillShape(Vertex _v1, Vertex _v2, Vertex _v3)//WIP
{
    //vector breakdown
    float X_V1 = _v1.Coordinate.XY.X;
    float X_V2 = _v2.Coordinate.XY.X;
    float X_V3 = _v3.Coordinate.XY.X;
    float Y_V1 = _v1.Coordinate.XY.Y;
    float Y_V2 = _v2.Coordinate.XY.Y;
    float Y_V3 = _v3.Coordinate.XY.Y;

    //unmodified cordinets
    short X1 = 0;
    short Y1 = 0;

    short X2 = 0;
    short Y2 = 0;

    short X3 = 0;
    short Y3 = 0;


    //ycounting range
    short ithStart = 0;
    short ithEnd = 0;
    //x counting range
    short jthStart = 0;
    short jthEnd = 0;

        
        //Vertex 1 pixels
    NormalToFramBufferArray(X_V1,Y_V1,&X1,&Y1);
        //Vertex 2 pixels
    NormalToFramBufferArray(X_V2,Y_V2,&X2,&Y2);
        //Vertex 3 pixels
    NormalToFramBufferArray(X_V3,Y_V3,&X3,&Y3);

    //gets the pixel counting range
    //init start and stop points for x and y
    if (X1 > X2)
    {
        if (X1 > X3)
        {
            jthEnd = X1;
        }
        else
        {
            jthEnd = X3;
        }
        
        
    }
    else
    {
        if (X2 > X3)
        {
            jthEnd = X2;
        }
        else
        {
            jthEnd = X3;
        }
        
    }

    if (X1<X2)
    {
        if (X1 < X3)
        {
            jthStart = X1;
        }
        else
        {
            jthStart = X3;
        }
        
        
    }
    else
    {
        if (X2 < X3)
        {
            jthStart = X2;
        }
        else
        {
            jthStart = X3;
        }
        
    }

    if (Y1 > Y2)
    {
        if (Y1 > Y3)
        {
            ithEnd = Y1;
        }
        else
        {
            ithEnd = Y3;
        }
        
        
    }
    else
    {
        if (Y2 > Y3 )
        {
            ithEnd = Y2;
        }
        else
        {
            ithEnd = Y3;
        }
        
    }

    if (Y1 < Y2)
    {
        if (Y1 < Y3)
        {
            ithStart = Y1;
        }
        else
        {
            ithStart = Y3;
        }
        
        
    }
    else
    {
        if (Y2 < Y3)
        {
            ithStart = Y2;
        }
        else
        {
            ithStart = Y3;
        }
        
    }

    //vectors for the cross product of the vertex and the pixels to be displayed
    int vec1[2] = {(X2-X1),(Y2-Y1)};
    int vec2[2] = {(X3-X2),(Y3-Y2)};
    int vec3[2] = {(X1-X3),(Y1-Y3)};

    int veca[2] = {(X3-X1),(Y3-Y1)};

    //for normalizing the color cordinates to a barycentric coordinates system
    float area = ((vec1[0]*veca[1])-(vec1[1]*veca[0]));

    for (size_t i = ithStart; i <= ithEnd; i++)
    {
        for (size_t j =jthStart; j <= jthEnd; j++)
        {
            
            // more vectors for the cross product of the vertex and the pixels to be displayed
            int vec1p[2] = {(j-X1),(i-Y1)};
            int vec2p[2] = {(j-X2),(i-Y2)};
            int vec3p[2] = {(j-X3),(i-Y3)};

            
            //the cross product
            int w1 = ((vec1[0]*vec1p[1])-(vec1[1]*vec1p[0]));
            int w2 = ((vec2[0]*vec2p[1])-(vec2[1]*vec2p[0]));
            int w3 = ((vec3[0]*vec3p[1])-(vec3[1]*vec3p[0]));

            


            if (w1 >=0 && w2 >=0 && w3 >=0) // checkis if the pixels falls inbetween the triangle
            {
                
                //the modifires for the color based on the pixels location
                float alpha = (float)w1 / (float)area;
                float beta = (float)w2 / (float)area;
                float gamma = (float)w3 / (float)area;
                

                RGB16 ColorHold = 0; //a color place holder so the orginal color doent get modified

                uint8_t red1 = 0;
                uint8_t green1 = 0;
                uint8_t blue1 = 0;

                uint8_t red2 = 0;
                uint8_t green2 = 0;
                uint8_t blue2 = 0;

                uint8_t red3 = 0;
                uint8_t green3 = 0;
                uint8_t blue3 = 0;

                int8_t redNew = 0;
                int8_t greenNew = 0;
                int8_t blueNew = 0;

                //breaks down the color into its three componets of red green and blue
                ColorHold = _v1.VertexColor;

                

                //color 1
                red1 = (0b11111 & ColorHold);
                ColorHold = ColorHold >> 5;
                green1 = (0b111111 & ColorHold);
                ColorHold = ColorHold >> 6;
                blue1 = (0b11111 & ColorHold);

                ColorHold = _v2.VertexColor;
                //color 2
                red2 = (0b11111 & ColorHold);
                ColorHold = ColorHold >> 5;
                green2 = (0b111111 & ColorHold);
                ColorHold = ColorHold >> 6;
                blue2 = (0b11111 & ColorHold);

                ColorHold = _v3.VertexColor;
                //color 3
                red3 = (0b11111 & ColorHold);
                ColorHold = ColorHold >> 5;
                green3 = (0b111111 & ColorHold);
                ColorHold = ColorHold >> 6;
                blue3 = (0b11111 & ColorHold);

                //the colors adjustments for each othe the color channels
                redNew = (beta*red1)+(gamma*red2)+(alpha*red3);
                greenNew = (beta*green1)+(gamma*green2)+(alpha*green3);
                blueNew = (beta*blue1)+(gamma*blue2)+(alpha*blue3);

                RGB16 NewColor = ToRGB16(redNew,greenNew,blueNew);//combines the the three channels into a single varriable

                //loads the pixel into the buffer array7
                size_t CurrentPixel = ToFramBufferArray(j,i);
                ((RGB16*)FrameBuffer1)[CurrentPixel] = NewColor;
                
            }
            
            

        }
        
    }
    
    //(void)esp_lcd_panel_draw_bitmap(waveShareDisplay_panel,0,0,X_COUNT,Y_COUNT,FrameBuffer1);



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

    Vertex CubeVertex[] = {
        {.Coordinate.XYZ = {-3.f,-3.f,1.f},.VertexColor = WHITE},{.Coordinate.XYZ ={-3.f,3.f,1.f},.VertexColor = WHITE},{.Coordinate.XYZ ={3.f,3.f,1.f},.VertexColor = WHITE},{.Coordinate.XYZ ={3.f,-3.f,1.f},.VertexColor = WHITE},  //front verties
        {.Coordinate.XYZ ={-3.f,-3.f,3.2,},.VertexColor = BLACK},{.Coordinate.XYZ ={-3.f,3.f,3.2},.VertexColor = BLACK},{.Coordinate.XYZ ={3.f,3.f,3.2},.VertexColor = BLACK},{.Coordinate.XYZ ={3.f,-3.f,3.2},.VertexColor = BLACK}};   //back verties
    
        /*THIS MATTER THE MOST VERTEIES MUST BE PUT IN THE CORRECT ORDER OR WILL NOT RENDER PROPERLY*/
        unsigned int CubeFaceConstrcution[][3] = {{6,5,8},{6,8,7},  //back
                                              {2,6,7},{2,7,3},  //up
                                              {5,1,4},{5,4,8},  //down
                                              {5,6,2},{5,2,1},  //left
                                              {4,3,7},{4,7,8},  //right
                                              {3,2,1},{4,3,1}}; //front

    Mesh3D ACube =
    {
        .NumberOfFaces = 10,
        .NumberOfVertices = 8,
        .VertexArray = CubeVertex,
        .FaceConstruction = CubeFaceConstrcution
    };

    //checks if the buffer pointers have been inited
    if (NULL == FrameBuffer0 || NULL == FrameBuffer1) 
    {
        return -1;
    }


   for (size_t i = 0; i < ACube.NumberOfFaces; i++)
    {
        //sets up the vertexs
        Vertex Vertex1 = {.Coordinate.XY.X = ((ACube.VertexArray[ACube.FaceConstruction[i][0]-1].Coordinate.XYZ.X)/(ACube.VertexArray[ACube.FaceConstruction[i][0]-1].Coordinate.XYZ.Z)), .Coordinate.XY.Y = ((ACube.VertexArray[ACube.FaceConstruction[i][0]-1].Coordinate.XYZ.Y)/(ACube.VertexArray[ACube.FaceConstruction[i][0]-1].Coordinate.XYZ.Z)),.VertexColor=(ACube.VertexArray[ACube.FaceConstruction[i][0]-1].VertexColor)};
        Vertex Vertex2 = {.Coordinate.XY.X = ((ACube.VertexArray[ACube.FaceConstruction[i][1]-1].Coordinate.XYZ.X)/(ACube.VertexArray[ACube.FaceConstruction[i][1]-1].Coordinate.XYZ.Z)), .Coordinate.XY.Y = ((ACube.VertexArray[ACube.FaceConstruction[i][1]-1].Coordinate.XYZ.Y)/(ACube.VertexArray[ACube.FaceConstruction[i][1]-1].Coordinate.XYZ.Z)),.VertexColor=(ACube.VertexArray[ACube.FaceConstruction[i][1]-1].VertexColor)};
        Vertex Vertex3 = {.Coordinate.XY.X = ((ACube.VertexArray[ACube.FaceConstruction[i][2]-1].Coordinate.XYZ.X)/(ACube.VertexArray[ACube.FaceConstruction[i][2]-1].Coordinate.XYZ.Z)), .Coordinate.XY.Y = ((ACube.VertexArray[ACube.FaceConstruction[i][2]-1].Coordinate.XYZ.Y)/(ACube.VertexArray[ACube.FaceConstruction[i][2]-1].Coordinate.XYZ.Z)),.VertexColor=(ACube.VertexArray[ACube.FaceConstruction[i][2]-1].VertexColor)};

        FillShape(Vertex1,Vertex2,Vertex3);

        //draws the lines
        //DrawLine(Vertex1,Vertex2);
        //DrawLine(Vertex2,Vertex3);
        //DrawLine(Vertex3,Vertex1);
        //DrawLine(Vertex1,Vertex3);
    }

    Vertex ColorSquarVertex[] = {
        {.Coordinate.XYZ = {-4.f,4.f,1.f},.VertexColor = WHITE},{.Coordinate.XYZ = {-7.f,1.f,1.f},.VertexColor = RED},{.Coordinate.XYZ ={-7.f,7.f,1.f},.VertexColor = GREEN},{.Coordinate.XYZ ={-1.f,7.f,1.f},.VertexColor = BLUE},{.Coordinate.XYZ ={-1.f,1.f,1.f},.VertexColor = YELLOW}};
    unsigned int ColorSquarConstrcution[][3] = {{3,2,1},{4,3,1},
                                              {5,4,1},{2,5,1}};

    Mesh3D ColorSquar =
    {
        .NumberOfFaces = 4,
        .NumberOfVertices = 5,
        .VertexArray = ColorSquarVertex,
        .FaceConstruction = ColorSquarConstrcution
    };

    for (size_t i = 0; i < ColorSquar.NumberOfFaces; i++)
    {
        //sets up the vertexs
        Vertex Vertex1 = {.Coordinate.XY.X = ((ColorSquar.VertexArray[ColorSquar.FaceConstruction[i][0]-1].Coordinate.XYZ.X)/(ColorSquar.VertexArray[ColorSquar.FaceConstruction[i][0]-1].Coordinate.XYZ.Z)), .Coordinate.XY.Y = ((ColorSquar.VertexArray[ColorSquar.FaceConstruction[i][0]-1].Coordinate.XYZ.Y)/(ColorSquar.VertexArray[ColorSquar.FaceConstruction[i][0]-1].Coordinate.XYZ.Z)),.VertexColor=(ColorSquar.VertexArray[ColorSquar.FaceConstruction[i][0]-1].VertexColor)};
        Vertex Vertex2 = {.Coordinate.XY.X = ((ColorSquar.VertexArray[ColorSquar.FaceConstruction[i][1]-1].Coordinate.XYZ.X)/(ColorSquar.VertexArray[ColorSquar.FaceConstruction[i][1]-1].Coordinate.XYZ.Z)), .Coordinate.XY.Y = ((ColorSquar.VertexArray[ColorSquar.FaceConstruction[i][1]-1].Coordinate.XYZ.Y)/(ColorSquar.VertexArray[ColorSquar.FaceConstruction[i][1]-1].Coordinate.XYZ.Z)),.VertexColor=(ColorSquar.VertexArray[ColorSquar.FaceConstruction[i][1]-1].VertexColor)};
        Vertex Vertex3 = {.Coordinate.XY.X = ((ColorSquar.VertexArray[ColorSquar.FaceConstruction[i][2]-1].Coordinate.XYZ.X)/(ColorSquar.VertexArray[ColorSquar.FaceConstruction[i][2]-1].Coordinate.XYZ.Z)), .Coordinate.XY.Y = ((ColorSquar.VertexArray[ColorSquar.FaceConstruction[i][2]-1].Coordinate.XYZ.Y)/(ColorSquar.VertexArray[ColorSquar.FaceConstruction[i][2]-1].Coordinate.XYZ.Z)),.VertexColor=(ColorSquar.VertexArray[ColorSquar.FaceConstruction[i][2]-1].VertexColor)};

        FillShape(Vertex1,Vertex2,Vertex3);

        //draws the lines
        DrawLine(Vertex1,Vertex2);
        DrawLine(Vertex2,Vertex3);
        DrawLine(Vertex3,Vertex1);
        DrawLine(Vertex1,Vertex3);
    }
    

    return _resualt;
}
