/**
 * ©Connell T Hagans II 2026
 * EDITING THIS FILE CAN BREAK THIS PROGRAM AND SHOULD BE DONE IN CAUSTION IF AT ALL
 * will be uploading code to get hub so there is more public information abouty writing drivers for displays
 * Display: https://www.crystalfontz.com/product/cfaf800480e1050sc-800x480-5-inch-color-tft
 * Driver mode: rgb 565 (16 bit colour)
 * D_CLK & DE are positive polarity (refer to data sheet ST7262 Datasheet timing graph)
 * H_SYNC & V_SYNC are negitive polarity (refer to data sheet ST7262 Datasheet timing graph)
 * '(void)' is put infront of every esp-idf function call because im not checking for errors or using the esp-idf error logging functionality
 */
#include "wsDisplayDriver.h"
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PRIVITE INCLUDES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include <stdio.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/pulse_cnt.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PRIVITE DEFINES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * D_CLK PCNT handle defines
 * needs to more watch points to properly set up the counters and pulses
 */
#define pulscounter_lowlimit -1
#define pulscounter_highlimit (PIXEL_WIDTH + W_FRONTPORCH + W_BACKPORCH + PIXEL_HIGHT + H_FRONTPORCH + H_BACKPORCH)
#define pulscounter_horizontal 800


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PRIVITE VARIABLES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static _Bool _isDisplayInit = 0;        //is used to make sure every thing else below is in order
static _Bool _isDisplayPinsInit = 0;    //is used to check is pins have init befor doing any internal functions
static _Bool _isD_CLKCounterInit = 0;   //is used to check to make sure D_CLK isnt being re init
static _Bool _isD_CLKInit = 0;          //is used to check to make sure D_CLKCounter isnt being re init

static _Bool _isDisplayRunning = 0;       //is used to check if D_CLK is running or is paused

pcnt_unit_handle_t D_CLKPulsCounter = NULL;
pcnt_channel_handle_t D_CLKChannel = NULL;


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PRIVITE INITS AND DEINITS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * D_CLk inits
 * set up timed pulses and puls moniters to set up proper timings for the monitor screen pixel reads
 */
void initD_CLK_Timer(void) //inits the pixel clock to turn on the screen
{
    // 1. Configure timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .timer_num        = LEDC_TIMER_0,
        .duty_resolution  = LEDC_TIMER_1_BIT, // 50% duty cycle
        .freq_hz          = D_CLK_SPEED,            // 25 MHz clock
        .clk_cfg          = LEDC_AUTO_CLK
    };
    (void)ledc_timer_config(&ledc_timer);

    // 2. Configure channel
    ledc_channel_config_t ledc_channel = 
    {
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .timer_sel      = LEDC_TIMER_0,
        .gpio_num       = D_CLK,
        .duty           = 1, // 50% duty for 1-bit resolution
        .hpoint         = 0
    };
    (void)ledc_channel_config(&ledc_channel);
}
void initD_CLK_Counter(void) //inits the puls counter for the horizontial pulses and vertical pulses
{
    //confifgure puls counter units for D_CLK handles (PCNT)
    pcnt_unit_config_t D_CLKHandle_config = 
    {
        .high_limit =   pulscounter_highlimit,
        .low_limit =    pulscounter_lowlimit
    };
    (void)pcnt_new_unit(&D_CLKHandle_config, &D_CLKPulsCounter);
    
    //Configures PCNT channel
    pcnt_chan_config_t D_CLKChannel_config =
    {
        .edge_gpio_num = D_CLK,
        .level_gpio_num = -1
    };
    (void)pcnt_new_channel(D_CLKPulsCounter,&D_CLKChannel_config,&D_CLKChannel);

    //Configure channel actions
    (void)pcnt_channel_set_edge_action
    (
        D_CLKChannel,
        PCNT_CHANNEL_EDGE_ACTION_INCREASE,
        PCNT_CHANNEL_EDGE_ACTION_HOLD
    );
    
    //ENABLE PCNT HANDLE
    (void)pcnt_unit_enable(D_CLKPulsCounter);

}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PRIVITE FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int TurnOnDisplay(void)
{
    if (0 == _isDisplayInit)
    {
        return -1;
    }
    if (1 == _isDisplayInit && 0 == _isDisplayRunning)
    {
        (void)pcnt_unit_start(D_CLKPulsCounter);
        (void)ledc_timer_resume(LEDC_LOW_SPEED_MODE,LEDC_TIMER_0);
        _isDisplayRunning = 1;
        return 1;
    }
    if (1 == _isDisplayInit && 1 == _isDisplayRunning)
    {
        return 0;
    }
    return -404;
}

int TurnOffDisplay()
{
    if (0 == _isDisplayInit)
    {
        return -1;
    }
    if (1 == _isDisplayInit && 1 == _isDisplayRunning)
    {
        //need to be called in revers order to prevent artifacts from showing
        (void)ledc_timer_pause(LEDC_LOW_SPEED_MODE,LEDC_TIMER_0);
        (void)gpio_set_level(D_CLK,0);
        (void)pcnt_unit_stop(D_CLKPulsCounter);
        (void)pcnt_unit_clear_count(D_CLKPulsCounter);
        _isDisplayRunning = 0;
        return 1;
    }
    if (1 == _isDisplayInit && 0 == _isDisplayRunning)
    {
        return 0;
    }
    return -404;
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
 * will set the pin direction and init levels to 0
 */
void WSDisplayDriver(void)
{
    //1. inits every display pin the
    if (0 == _isDisplayPinsInit)
    {
        //init red data pins
        /*
        (void)gpio_set_direction(R0,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(R0,0);
        (void)gpio_set_direction(R1,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(R1,0);
        (void)gpio_set_direction(R2,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(R2,0);
        */
        (void)gpio_set_direction(R3,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(R3,0);
        (void)gpio_set_direction(R4,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(R4,0);
        (void)gpio_set_direction(R5,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(R5,0);
        (void)gpio_set_direction(R6,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(R6,0);
        (void)gpio_set_direction(R7,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(R7,0);

        //init green data pins
        /*
        (void)gpio_set_direction(G0,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(G0,0);
        (void)gpio_set_direction(G1,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(G1,0);
        */
        (void)gpio_set_direction(G2,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(G2,0);
        (void)gpio_set_direction(G3,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(G3,0);
        (void)gpio_set_direction(G4,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(G4,0);
        (void)gpio_set_direction(G5,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(G5,0);
        (void)gpio_set_direction(G6,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(G6,0);
        (void)gpio_set_direction(G7,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(G7,0);

        //init blue data pins
        /*
        (void)gpio_set_direction(B0,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(B0,0);
        (void)gpio_set_direction(B1,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(B1,0);
        (void)gpio_set_direction(B2,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(B2,0);
        */
        (void)gpio_set_direction(B3,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(B3,0);
        (void)gpio_set_direction(B4,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(B4,0);
        (void)gpio_set_direction(B5,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(B5,0);
        (void)gpio_set_direction(B6,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(B6,0);
        (void)gpio_set_direction(B7,GPIO_MODE_OUTPUT);
        (void)gpio_set_level(B7,0);

        //init control pins
        (void)gpio_set_direction(D_CLK,GPIO_MODE_INPUT_OUTPUT);
        (void)gpio_set_level(D_CLK,0);
        (void)gpio_set_direction(H_SYNC,GPIO_MODE_INPUT_OUTPUT);
        (void)gpio_set_level(H_SYNC,0);
        (void)gpio_set_direction(V_SYNC,GPIO_MODE_INPUT_OUTPUT);
        (void)gpio_set_level(V_SYNC,0);
        (void)gpio_set_direction(DE,GPIO_MODE_INPUT_OUTPUT);
        (void)gpio_set_level(DE,0);

        //sets _isDisplayPinsInit to true
        _isDisplayPinsInit = 1;

    }
    //2. inits D_CLK timed pulses
    if (0 == _isD_CLKInit)
    {
        //make sure to call this first befor setting up puls counter
        initD_CLK_Timer();
        //pauses timer so counter does get count up and to make sure the screen does show artifacts during init
        (void)ledc_timer_pause(LEDC_LOW_SPEED_MODE,LEDC_TIMER_0);
        (void)gpio_set_level (D_CLK,0);


        //sets _isD_CLKInit to true
        _isD_CLKInit = 1;
    }
    //3. inits D_CLK puls counter
    if (0 == _isD_CLKCounterInit)
    {
        initD_CLK_Counter();
        //makes sure counter is at zero
        (void)pcnt_unit_clear_count(D_CLKPulsCounter);
        //sets _isD_CLKCounterInit to true
        _isD_CLKCounterInit = 1;
    }
    
    //sets internal state to true if every thing else has been inited
    if (1 == _isDisplayPinsInit && 1 == _isD_CLKInit && 1== _isD_CLKCounterInit)
    {
        _isDisplayInit = 1;
    }
        
}

/**
 * 
 */
void _WSDisplayDriver_(void)
{
    (void)TurnOffDisplay();

    //add code that releases the timer and puls counter
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           TESTER FUNCTIONS (*REMOVE BEFOR FLIGHT*)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int t_initRedTest()
{
    WSDisplayDriver();

    //INIT RED COLOR PINS ONLY
    gpio_set_level(R3,1);
    gpio_set_level(R4,1);
    gpio_set_level(R5,1);
    gpio_set_level(R6,1);
    gpio_set_level(R7,1);
    gpio_set_level(G2,0);
    gpio_set_level(G3,0);
    gpio_set_level(G4,0);
    gpio_set_level(G5,0);
    gpio_set_level(G6,0);
    gpio_set_level(G7,0);
    gpio_set_level(B3,0);
    gpio_set_level(B4,0);
    gpio_set_level(B5,0);
    gpio_set_level(B6,0);
    gpio_set_level(B7,0);

    return TurnOnDisplay();
}

