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
#define BUFFERSIZE (H_COUNT*V_COUNT)

#define BUFFER_X_MAX (H_COUNT-1)
#define BUFFER_Y_MAX (V_COUNT-1)


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PRIVITE VARIABLES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//DISPLAY CONTROLS
static _Bool _isDisplayInit = 0;        //is used to make sure every thing else below is in order
static _Bool _isDisplayRunning = 0;       //is used to check if D_CLK is running or is paused

esp_lcd_panel_handle_t waveShareDisplay_panel = NULL; //the control handle for the display

//FRAMER BUFFER
void* MainFrameBuffer = NULL; //will get address from esp lcd function [fb0] (Will be the frame buffer that is displaied)
void* DrawingFrameBuffer = NULL; //will get address from esp lcd function [fb1] (Will be the buffer that is drawn to then uploaded to the main frame buffer)


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PRIVITE INITS AND DEINITS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//FRAME BUFFER
/**
 * gets the adress of the frame buffer if the display has been inited
 */
void init_FrameBuffer(void)
{
    if (NULL == DrawingFrameBuffer && 1 == _isDisplayInit)
    {
        printf("FrameBuffer Ininted\n ");
        esp_lcd_rgb_panel_get_frame_buffer(waveShareDisplay_panel,NUM_OF_FRAMEBUFFERS,&MainFrameBuffer,&DrawingFrameBuffer);
    }
    
}

/**
 * releases the adress of the frame buffer
 */
void deinit_FrameBuffer(void)
{
    DrawingFrameBuffer = NULL; 
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PRIVITE FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * is used in relation to a for loop or a 2d array and output a corrisponding 1d frame buffer array position
 * will abort if out side the frame buffer
 */
unsigned int ToFramBufferArray(unsigned int _x, unsigned int _y)
{
    if (_x < H_COUNT && _y < V_COUNT)  
    {
        return ((_x)) + ((800) * (_y));
    }
    abort();
}


int TurnOnDisplay(void) //makes the proper calls and sitches internal vars to the opporiate values
{
    if (0 == _isDisplayInit) //is checked first to make sure the display has been inited
    {
        return -1;
    }
    if (1 == _isDisplayInit && 0 == _isDisplayRunning)
    {
        (void)esp_lcd_panel_disp_on_off(waveShareDisplay_panel,1); //calls ESP-IDF lcd internal code to turn on display signaals
        _isDisplayRunning =1;
        init_FrameBuffer();
        esp_lcd_rgb_panel_restart(waveShareDisplay_panel);
        return 1;
    }
    if (1 == _isDisplayInit && 1 == _isDisplayRunning) //does nothing is the display is already running
    {
        return 0;
    }
    return -404; //only returns if something unexspected happens, but this should never be called
}

int TurnOffDisplay()//makes the proper calls and sitches internal vars to the opporiate values
{
    if (0 == _isDisplayInit) //is checked first to make sure the display has been inited
    {
        return -1;
    }
    if (1 == _isDisplayInit && 1 == _isDisplayRunning)
    {
        (void)esp_lcd_panel_disp_on_off(waveShareDisplay_panel,0); //calls ESP-IDF lcd internal code to turn off display signaals
        _isDisplayRunning = 0;
        return 1;
    }
    if (1 == _isDisplayInit && 0 == _isDisplayRunning) // doesnothing is the display is already off
    {
        return 0;
    }
    return -404; //only returns if something unexspected happens, but this should never be called
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PUBLIC VARIABLES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PUBLIC FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    

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
        
        esp_lcd_rgb_panel_config_t rgb_panel_config = //configureation setting for waveShareDisplay_panel
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
                .flags.pclk_active_neg =1
            }, 
            .data_width = RGB565,
            .bits_per_pixel = 0, // set to zero so it can match the data width
            .de_gpio_num = DE,
            .hsync_gpio_num = H_SYNC,
            .vsync_gpio_num = V_SYNC,
            .pclk_gpio_num =  D_CLK,
            .disp_gpio_num = -1, // not in use
            .data_gpio_nums = {R3,R4,R5,R6,R7,G2,G3,G4,G5,G6,G7,B3,B4,B5,B6,B7},
            .flags.fb_in_psram = true, // allocate frame buffers from PSRAM
            .num_fbs = NUM_OF_FRAMEBUFFERS,
            .dma_burst_size = 64, //idk why just needs to be in the power of 2's and 256 is the recommanded burst size for a 32bit system and 16 x 16 = 256 cannot exide 64 since its external memorry
        };

        esp_lcd_new_rgb_panel(&rgb_panel_config,&waveShareDisplay_panel); //ESP-IDF code the configers waveShareDisplay_panel

        //initualization of waveShareDisplay_panel
        esp_lcd_panel_reset(waveShareDisplay_panel); //is called to clean the display
        esp_lcd_panel_init(waveShareDisplay_panel);
        

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
    deinit_FrameBuffer();
    esp_lcd_panel_reset(waveShareDisplay_panel); //is called to clean the display
    esp_lcd_panel_del(waveShareDisplay_panel); //releases the pins and timers of the waveShareDisplay_panel

    //finished
    _isDisplayInit =0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           TEST AREA (*REMOVE BEFOR FLIGHT*)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define tBUFFERSIZE (H_COUNT*(V_COUNT/4))

uint16_t _test_color_data[tBUFFERSIZE]; // Upload Buffer

/**
 * is a function that cycles through all advaliable colors
 */
static long callCount = 0; //is used to progress through the funtion for each time it is called
int makeColor()
{  
    
    const unsigned int scalar = 50;// Do not make zero
    uint16_t Color = (WHITE/2)*(callCount/scalar); //is derived from the sine function ((PI/2)*time)

    callCount++; //updates callCounter +1

    return Color;
}

/**
 * upload buffer updats and display buffer upload to DMA
 */
void t_updateDisplay()
{
    esp_lcd_panel_draw_bitmap(waveShareDisplay_panel,0,0,800,120,_test_color_data);//updates the psram buffer for the DMA
    for (int i = 0; i < tBUFFERSIZE; i++)
    {
        _test_color_data[i] = makeColor();
    }
    esp_lcd_panel_draw_bitmap(waveShareDisplay_panel,0,120,800,240,_test_color_data);
    for (int i = 0; i < tBUFFERSIZE; i++)
    {
        _test_color_data[i] = makeColor();
    }
    esp_lcd_panel_draw_bitmap(waveShareDisplay_panel,0,240,800,360,_test_color_data);
    for (int i = 0; i < tBUFFERSIZE; i++)
    {
        _test_color_data[i] = makeColor();
    }
    esp_lcd_panel_draw_bitmap(waveShareDisplay_panel,0,360,800,480,_test_color_data);
    
    callCount =0;
    for (size_t y = 0; y < V_COUNT; y++) //CURSED
    {
        for (size_t x = 0; x < H_COUNT; x++)
        {
            ((uint16_t*)DrawingFrameBuffer)[ToFramBufferArray(x,y)] = BLACK;
        }
        
    }
    for (size_t y = 0; y < V_COUNT; y++) //CURSED
    {
        for (size_t x = 0; x < H_COUNT; x++)
        {
            ((uint16_t*)DrawingFrameBuffer)[ToFramBufferArray(y,y)] = WHITE;
        }
        
    }
    esp_lcd_rgb_panel_restart(waveShareDisplay_panel);
    esp_lcd_panel_draw_bitmap(waveShareDisplay_panel,1,1,800,480,DrawingFrameBuffer);
}

/**
 * inits the color buffer and the display then displays the test resualts
 */
int t_initDisplayTest(void)
{

    for (int i = 0; i < tBUFFERSIZE; i++) //upload buffer init
    {
        _test_color_data[i] = makeColor();
    }
    
    WSDisplayDriver();  //display set up
    
    (void)TurnOnDisplay();
    t_updateDisplay(); //internal fuction called to load the display buffer
    
  
    return 1;
}

void t_bufferTest(void)
{
    if (NULL != DrawingFrameBuffer)
    {
        for (size_t y = 0; y < V_COUNT; y++)
        {
            for (size_t x = 0; x < H_COUNT; x++)
            {
                ((uint16_t*)DrawingFrameBuffer)[ToFramBufferArray(x,y)] = BROWN;
            }
            
        }
        for (size_t y = 0; y < V_COUNT; y++)
        {
            for (size_t x = 0; x < H_COUNT; x++)
            {
                if ((x >((H_COUNT/2)-10) && x< ((H_COUNT/2)+10)) && (y >((V_COUNT/2)-10) && y< ((V_COUNT/2)+10)))
                {
                    ((uint16_t*)DrawingFrameBuffer)[ToFramBufferArray(x,y)] = CYAN;
                }
            }
            
        }
        for (size_t y = 0; y < V_COUNT; y++)
        {
            for (size_t x = 0; x < H_COUNT; x++)
            {
                if (x==(H_COUNT/2) || y == (V_COUNT/2))
                {
                    ((uint16_t*)DrawingFrameBuffer)[ToFramBufferArray(x,y)] = RED;
                }
            }
            
        }
        
        for (size_t y = 0; y < V_COUNT; y++)
        {
            for (size_t x = 0; x < H_COUNT; x++)
            {
                if (y < 2 || y > (BUFFER_Y_MAX-2) )
                {
                    ((uint16_t*)DrawingFrameBuffer)[ToFramBufferArray(x,y)] = WHITE;
                }
            }
            
        }
        
        for (size_t y = 0; y < V_COUNT; y++)
        {
            for (size_t x = 0; x < H_COUNT; x++)
            {
                if (x < 2 || x > (BUFFER_X_MAX-2) )
                {
                    ((uint16_t*)DrawingFrameBuffer)[ToFramBufferArray(x,y)] = WHITE;
                }
            }
            
        }
        
        //((uint16_t*)DrawingFrameBuffer)[ToFramBufferArray(0,0)] = BLACK;
        
        esp_lcd_panel_draw_bitmap(waveShareDisplay_panel,0,0,H_COUNT,V_COUNT,DrawingFrameBuffer);
        //esp_lcd_rgb_panel_restart(waveShareDisplay_panel);
        esp_lcd_panel_draw_bitmap(waveShareDisplay_panel,0,0,H_COUNT,V_COUNT,DrawingFrameBuffer);
        esp_lcd_rgb_panel_restart(waveShareDisplay_panel);
        esp_lcd_panel_draw_bitmap(waveShareDisplay_panel,0,0,H_COUNT,V_COUNT,DrawingFrameBuffer);
        esp_lcd_panel_draw_bitmap(waveShareDisplay_panel,0,0,H_COUNT,V_COUNT,DrawingFrameBuffer);
        
    }
    

    
    
}

/**
 * inits the buffer and the display then displays the test resualts
 * @note: This code or device is buggy so you may have to reset the esp32-s3 5 - 7 times
 */
int t_DisplayBufferTest(void)
{
    printf("\033[0;31mDisplay init = %s\nFrame Buffer Adress1 = %p\nFrame Buffer Adress2 = %p\033[0m\n",_isDisplayInit ? "True" : "False",MainFrameBuffer,DrawingFrameBuffer);
    WSDisplayDriver();  //display set up
    (void)TurnOnDisplay();
    t_bufferTest();
    esp_lcd_rgb_panel_restart(waveShareDisplay_panel);
    printf("\033[0;31mDisplay init = %s\nFrame Buffer Adress1 = %p\nFrame Buffer Adress2 = %p\033[0m\n",_isDisplayInit ? "True" : "False",MainFrameBuffer,DrawingFrameBuffer);

    /**
     * these should be called but is instead called in the main loop for my personal demostartion
    vTaskDelay(1000/portTICK_PERIOD_MS);
    _WSDisplayDriver_();
     */
    return 1;
}
