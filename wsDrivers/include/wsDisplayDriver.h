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

//ASPECT RATIO
    #define ASPECTRATIO_WIDTH 5
    #define ASPECTRATIO_HIGHT 3
    #define ASPECTRATIO_MULTIPLIER 2


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

    //a float struct used to construct 4d vectors and potiotions
typedef struct
{
    float X;
    float Y;
    float Z;
    float W;
}Vec4D;

    //a float struct used to construct 3d vectors and potiotions
typedef struct
{
    float X;
    float Y;
    float Z;
}Vec3D;

    //a float struct used to construct 2d vectors and potiotions
typedef struct
{
    float X;
    float Y;
}Vec2D;

typedef union
{
    Vec2D XY;
    Vec3D XYZ;
    Vec4D XYZW;
} Coordinate;


typedef struct
{
    Coordinate Coordinate;
    RGB16 VertexColor;
}Vertex;



    //a data structure for contaning and minuplating 3d objects
typedef struct
{
    unsigned int NumberOfVertices;
    unsigned int NumberOfFaces;
    Vertex* VertexArray;
    unsigned int (*FaceConstruction)[3];
}Mesh3D;



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
 */
void DrawLine(Vertex _v1, Vertex _v2);



/**
 * A function that will displaye the solid shape to the display
 */
void FillShape(Vertex _v1, Vertex _v2, Vertex _v3);


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           HELPER FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Allows number to be used to set a RGB16 value
 * red and blue must be 31 - 0 and green must be 63 - 0
 * any else will abort
 */
RGB16 ToRGB16( int _red, int _green,  int _blue);

/**
 * 
 */
RGB16 InterpulateRGB16(RGB16 _colorStart, RGB16 _colorEnd, int _positionStart, int _positionEnd, int _positionCurrent);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           GETTERS AND SETTERS
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//GETTERS

//SETTERS


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                           TESTER FUNCTIONS (*REMOVE BEFOR FLIGHT*)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int tFlushDisplay(RGB16 _color);

int tDrawSomething();

#endif