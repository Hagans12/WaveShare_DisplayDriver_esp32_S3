/**
 * ©Connell T Hagans II 2026
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
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/ledc.h"
#include "driver/gptimer.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PRIVITE DEFINES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PRIVITE VARIABLES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static _Bool _isDisplayInit = 0; //is used to check is pins have init befor doing any internal functions
static _Bool _isD_CLKInit = 0;   //is used to check to make sure D_CLK isnt being re init
static _Bool _isD_CLKRunning = 0;   //is used to check to make sure D_CLK running or paused


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PRIVITE INITS AND DEINITS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void initD_CLK()
{
    if (0 == _isD_CLKInit)
    {
        // 1. Configure timer
        ledc_timer_config_t ledc_timer = {
            .speed_mode       = LEDC_LOW_SPEED_MODE,
            .timer_num        = LEDC_TIMER_0,
            .duty_resolution  = LEDC_TIMER_1_BIT, // 50% duty cycle
            .freq_hz          = 25000000,            // 25 MHz clock
            .clk_cfg          = LEDC_AUTO_CLK
        };
        (void)ledc_timer_config(&ledc_timer);

        // 2. Configure channel
        ledc_channel_config_t ledc_channel = {
            .gpio_num       = D_CLK,
            .speed_mode     = LEDC_LOW_SPEED_MODE,
            .channel        = LEDC_CHANNEL_0,
            .timer_sel      = LEDC_TIMER_0,
            .duty           = 1, // 50% duty for 1-bit resolution
            .hpoint         = 0
        };
        (void)ledc_channel_config(&ledc_channel);

    }
    else if (1 == _isD_CLKInit && 0 == _isD_CLKRunning)
    {
        (void)ledc_timer_resume(LEDC_LOW_SPEED_MODE,LEDC_TIMER_0);
    }
    _isD_CLKInit = 1;
    _isD_CLKRunning = 1;
    
}

void _initD_CLK_()
{
    if (1 == _isD_CLKRunning)
    {
        (void)ledc_timer_pause(LEDC_LOW_SPEED_MODE,LEDC_TIMER_0);
        (void)gpio_set_level (D_CLK,0);
    }
    _isD_CLKRunning = 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PRIVITE FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PUBLIC VARIABLES
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PUBLIC FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           GETTERS AND SETTERS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * Returns stat of _isDisplayInit
 */
_Bool IsDisplayDriverInit(void)
{
    return _isDisplayInit;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           CONSTRUCTOR AND DESTRUCTORW
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/**
 * will set internal state as init to make sure everything is running before the driver can do any thing else
 * will set the pin direction and init levels to 0
 */
void WSDisplayDriver(void)
{
    if (1 == _isDisplayInit)
    {
        return;
    }
    
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

    //sets internal state to true 
    _isDisplayInit = 1;
}

/**
 * 
 */
void _WSDisplayDriver_(void)
{
    //pauses the D_CLK to turn off the display to prevent artifacts
    _initD_CLK_();

    //sets internal state to false
    _isDisplayInit = 0;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           TESTER FUNCTIONS (*REMOVE BEFOR FLIGHT*)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void t_initRed()
{
    if(0 == _isDisplayInit)
    {
        WSDisplayDriver();
    }

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
}

void t_runBuffer()
{
    
}

/**
 * turns the screen red
 * @return 0 was called but failed
 * @return -1 display was not inited
 * @return 5 pin set up for red color was set   
 * @return 4 exited from v loop
 * @return 3 exited from h loop
 * @return 2 exited from p loop
 * @return 1 was called and completed
 */
 int t_RedDisplayTest(void)
 {
    //is init to zero meaning test has been entered
    int _funcState = 0;

    WSDisplayDriver(); //inits display driver

    if (0 ==_isDisplayInit)
    {
        _funcState = -1;
        return _funcState;
    }
    t_initRed();
    initD_CLK();
    vTaskDelay(10000/portTICK_PERIOD_MS);
    _initD_CLK_();
    
    _funcState = 1;
    return _funcState;

 }
