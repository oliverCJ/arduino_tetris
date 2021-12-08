#include "Free_Fonts.h"

//#define USE_RF24 1 // 是否启用rf24进行控制。不需要则注释掉
#define CE_PIN D2
#define CSN_PIN D3
#define JOYSTICK_ANGLOG_X_CENTER 512
#define JOYSTICK_ANGLOG_Y_CENTER 500
#define JOYSTICK_ANGLOG_X_DEV 5 // X方向摇杆误差
#define JOYSTICK_ANGLOG_Y_DEV 10 // Y方向摇杆误差

#define SCREEN_WIDTH 240 // 屏幕宽
#define SCREEN_HIGHT 320 // 屏幕高  
#define PIXEL_SIZE 15 // 像素块大小
#define GAME_BOARD_LINE_WIDTH 2 // 游戏区域边框线宽
#define GAME_BOARD_WIDTH 195 // 游戏区域宽
#define GAME_BOARD_HEIGHT 270 // 游戏区域高
#define FALING_SPEED 500 // 下落速度ms
#define MOVE_FREQ 150 // 移动刷新率ms,避免移动太快
#define MAX_LEVEL 15 // 最高等级,按行数升级，需要消除327680行
#define LEVEL_UP_LINE 20 // 升级行数

#define BUTTON_UP WIO_5S_LEFT // 向上，等价于旋转
#define BUTTON_DOWN WIO_5S_RIGHT // 向下
#define BUTTON_LEFT WIO_5S_DOWN // 向左
#define BUTTON_RIGHT WIO_5S_UP // 向右
#define BUTTON_CHANGE WIO_KEY_A // 旋转
#define BUTTON_PAUSE WIO_KEY_B // 暂停
#define BUTTON_NONE WIO_KEY_C // 未用，暂留

typedef struct shapeItem
{
    int p;            // 形状描述，注意描述bit从右向左，从下往上
    short shapWidth;  // 形状宽
    short shapHeight; // 形状高
};

typedef struct shape
{
    char shapeChar;         // 形状对应字符描述
    int color;              // 形状颜色
    short childLength;      // 子形状个数
    shapeItem childShape[4]; // 形状对应各种形变
};

// 选中形状
typedef struct shapeSelect
{
    shape sp;
    int rotation; // 旋转方向
    int posX; // 实时位置
    int posY; // 实时位置
    bool complete; // 是否触底
};

typedef struct board
{
    short totalRows;                          // 总行数
    short usedRows;                           // 已使用行数
    short totalCols;                          // 总列数
    long row[GAME_BOARD_HEIGHT / PIXEL_SIZE]; // 注意描述bit方向，从右向左
};

// S形状，反Z
shape shapeS = {
    'S',
    TFT_BLUE,
    2,
    {
        {0b101101, 2, 3},
        {0b011110, 3, 2},
    }};
// Z形状
shape shapeZ = {
    'Z',
    TFT_GREEN,
    2,
    {
        {0b011110, 2, 3},
        {0b110011, 3, 2},
    }};
shape shapeI = {
    'I',
    TFT_YELLOW,
    2,
    {
        {0b1111, 1, 4},
        {0b1111, 4, 1},
    }};
shape shapeO = {
    'O',
    TFT_WHITE,
    1,
    {
        {0b1111, 2, 2},
    }};
shape shapeL = {
    'L',
    TFT_ORANGE,
    4,
    {
        {0b110101, 2, 3},
        {0b001111, 3, 2},
        {0b101011, 2, 3},
        {0b111100, 3, 2},
    }};
shape shapeJ = {
    'J',
    TFT_ORANGE,
    4,
    {
        {0b111010, 2, 3},
        {0b111001, 3, 2},
        {0b010111, 2, 3},
        {0b100111, 3, 2},
    }};
shape shapeT = {
    'T',
    TFT_CYAN,
    4,
    {
        {0b101110, 2, 3},
        {0b111010, 3, 2},
        {0b011101, 2, 3},
        {0b010111, 3, 2},
    }};
shape shapesArray[] = {shapeS, shapeZ, shapeI, shapeO, shapeL, shapeJ, shapeT};

//游戏区域
board gameBoard = {
    GAME_BOARD_HEIGHT / PIXEL_SIZE,
    0,
    GAME_BOARD_WIDTH / PIXEL_SIZE,
    {}
};

// 分数，一次性消除的行对应不同分值，最大只能同时消除4行
int scoreBase[] = { 0, 40, 100, 300, 800 };
// 每个等级对应升级的所需行数，最大15级
int levelUpRows[] = {0, 20, 30, 40, 55, 75, 100, 135, 215, 275, 380, 515, 705, 960, 1210, 9999};