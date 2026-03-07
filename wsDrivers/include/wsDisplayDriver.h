/**
 * ©Connell T Hagans II 2026
 * will be uploading code to get hub so there is more public information abouty writing drivers for displays
 * Display: https://www.crystalfontz.com/product/cfaf800480e1050sc-800x480-5-inch-color-tft
 * Driver mode: rgb 565 (16 bit colour)
 * Using the esp32-s3
 * The only way to drive the LCD display is by using the LCD API provided by espressif sinc it envolves direct register access that espressif /
 *      restricts for security reasons(wifi/bluetooth security)
 * RGB565 format will be used
 * the display will used normalized cordinets between -1 to 1
 * 
 * PROGRESS:
 * [x] Display output(usins two frame buffer and a bounce buffer)
 * [ ] Incorparate a Homogeneous Coordinates system (center of the screen is (0,0) / IDK if i wanna make the the coordinents 3D or 2D)
 * [ ] Add drawing primitives <pixel, lines, rectangles, triangles, circles, rounded rectangles, bitmap>
 * [ ] Add/ expose helper functions to make coding easier
 * [ ] Add task creation to update the frame buffer
 * (maybe make the vectors aarays.)
 * [ ] refactor code to make it easier to read, faster to display, and to remove useless code
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
    #define D_CLK_SPEED    14*1000*1000 // 10MHz

    #define H_COUNT         800
    #define H_FRONTPORCH    48 
    #define H_BACKPORCH     12
    #define H_PULSWIDTH     4

    #define V_COUNT         480
    #define V_FRONTPORCH    12
    #define V_BACKPORCH     12
    #define V_PULSWIDTH     4

//DMA
    #define DMABURSTSIZE 32


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           CONSTRUCTOR AND DESTRUCTOR
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Init of display driver
 */
void WSDisplayDriver(void);
/**
 * Deinit of display driver
 * his function must be called befor shutting down the display physically. errors will still pop up but they show up more with out this being called
 */
void _WSDisplayDriver_(void);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           PUBLIC STRUCTS / TYPEDEFS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    //a uint16 type to represent the RGB565 color scheme
typedef unsigned short RGB16;

    //A Vector/Point to repesented in 2D space
typedef struct vec2D
{
    float X; //side to side
    float Y; //up to down
}vec2D;
    //A Vector/Point to repesented in 3D space
typedef struct vec3D
{
    float X; //side to side
    float Y; //up to down
    float Z; //front to back
}vec3D;
    //A primitive component to build triangles in 2D space
typedef struct vertex2D
{
    vec2D Vertex;
    RGB16 Color;
}vertex2D;
    //A primitive component to build triangles in 3D space
typedef struct vertex3D
{
    vec3D Vertex;
    RGB16 Color;
}vertex3D;
    //A primitive to build shapes in 2D space
typedef struct triangle2D
{
    vertex2D vertex0;
    vertex2D vertex1;
    vertex2D vertex2;
}triangle2D;
    //A primitive to build shapes in 3D space
typedef struct triangle3D
{
    vertex3D vertex0;
    vertex3D vertex1;
    vertex3D vertex2;
}triangle3D;
    //A primitive to build an object in 2D space
    //the origine must use normalized cordinates between -1 to 1
typedef struct mesh2D
{
    unsigned long TriangleCount;
    triangle2D* TriangleList;
    vec2D Traslation;
    vec2D Origin;
}mesh2D;
    //A primitive to build an object in 3D space
typedef struct mesh3D
{
    triangle3D* TriangleList;
    vec3D Rotation;
    vec3D Traslation;
    vec3D Origin;
}mesh3D;


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
 * @return 1 if already running
 * @return 0 is called properly
 */
int TurnOnDisplay(void);

/**
 * this can be called after WSDisplayDriver() and TurnOnDisplay() has been called
 * will return 1 if ran properly
 * @return -1 = Display not properly inted
 * @return 1 if already off
 * @return 0 is called properly
 */
int TurnOffDisplay(void);

//DRAWING FUNCTIONS
/**
 * will wipe the whole display with one color
 * @return -1 if the Display has not been turned on properly
 */
int FlushColorToDisplay(const RGB16 _color);

/**
 * A function that will displaye the line to the display
 * @return -1 if the Display has not been turned on properly
 */
int Build2DLine(const vertex2D _vertexList[2]);

/**
 * A function that will displaye the line to the display
 * @return -1 if the Display has not been turned on properly
 * @note: not implamented yet
 */
int Build3DLine(const vertex3D _vertexList[2]);

/**
 * A function that will displaye the Mesh object to the display
 * @return -1 if the Display has not been turned on properly
 */
int Build2DShape(const mesh2D* _mesh);

/**
 * A function that will displaye the Mesh object to the display
 * @return -1 if the Display has not been turned on properly
 * @note: not implamented yet
 */
int Build3DShape(const mesh3D* _mesh);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           HELPER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Allows number to be used to set a RGB16 value
 * red and blue must be 31 - 0 and green must be 63 - 0
 * any else will abort
 */
RGB16 ToRGB16(unsigned int _red, unsigned int _green, unsigned int _blue);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           GETTERS AND SETTERS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//GETTERS

//SETTERS


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           TESTER FUNCTIONS (*REMOVE BEFOR FLIGHT*)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int tFlushDisplay(RGB16 _color);

#endif