/**
 * 俄罗斯方块Arduino版，基于WIO_Terminal套件开发
 * LED显示屏分辨率：320*240 
 * IC:ILI9341
 * 
 * 如果需要使用其他Arduino套件，需要修改配置中显示屏分辨率定义和对应按钮引脚定义
 * @author oliverCJ <cgjp123@163.com>
 */

#include <TFT_eSPI.h> // 彩屏扩展库
#include <SPI.h>      // SPI库
#include <math.h>
#include "tetris.h"

#if defined(USE_RF24)
    #include <nRF24L01.h>
    #include <RF24.h>

    RF24 radio(CE_PIN, CSN_PIN);
    const byte address[6] = "00001";
    char receiveData[20] = "";
    int xAxis, yAxis;
#endif

TFT_eSPI tft = TFT_eSPI(); // 屏幕操作实例

short level = 1; // 等级
int rowCount = 0; // 消除行数统计
int currentLeveRowCount = 0; // 当前等级消除行统计
long score = 0; // 总分
//short levelCache = level;
int rowCountCache = rowCount;
long scoreCache = score;
int fallSpeed = FALING_SPEED; // 下落速度
int moveReq = MOVE_FREQ; // 刷新速率
int continueDownMoveReq = moveReq; // 持续按下刷新速率
int continueDownCount = 0; // 持续按下计数

byte buttonCatch = 0; // 0无操作 1上 2右 3下 4左 5旋转 6暂停/开始

long lastButonPauseTime = millis(); // 最后暂停时间
long lastButtonChangeTime = millis(); // 最后旋转时间
long lastButtonDownTime = millis(); // 最后按向下的时间
long lastButtonLeftTime = millis(); // 最后按向左的时间
long lastButtonRightTime = millis(); // 最后按向右的时间
long lastFallTime = millis(); // 最后下落时间

shapeSelect fallingShape; // 当前掉落的方块
shapeSelect nextShape; // 下一个准备掉落的方块

void setup(void) {
    prepare();
    initGame();
}

/**
 * 游戏初始化前准备工作，只需要执行一次
 */
void prepare() {
    #if defined(USE_RF24)
        radio.begin();
        radio.openReadingPipe(1, address);
        radio.setPALevel(RF24_PA_MIN);
        radio.startListening();
    #endif

    // 串口开启，便于调试和输出
    Serial.begin(9600);
    // 初始化按钮
    pinMode(BUTTON_CHANGE, INPUT_PULLUP);
    pinMode(BUTTON_PAUSE, INPUT_PULLUP);
    //pinMode(BUTTON_NONE, INPUT_PULLUP);
    // 初始化5路摇杆
    pinMode(BUTTON_UP, INPUT_PULLUP);
    pinMode(BUTTON_DOWN, INPUT_PULLUP);
    pinMode(BUTTON_LEFT, INPUT_PULLUP);
    pinMode(BUTTON_RIGHT, INPUT_PULLUP);
    // 初始化屏幕
    tft.begin();
    tft.setRotation(2);
}

/**
 * 游戏初始化，每次重新开始游戏都需要执行
 */
void initGame() {
    // 全屏幕初始化
    tft.fillScreen(TFT_BLACK);
    // 游戏边框
    drawOutRect();
    // 初始化游戏区记录
    initGameBoard();
    drawGameBottomInfo();
    serialPrintScore();

    fallingShape = getRandomNewShape();
    nextShape = getRandomNewShape(); // 下一个准备掉落的方块
    drawTetrisShapeByPos(nextShape, GAME_BOARD_WIDTH + GAME_BOARD_LINE_WIDTH + 5, GAME_BOARD_LINE_WIDTH + 2);
}

/**
 * 绘制游戏底部其他信息，分数，消除行，开发人等
 */
void drawGameBottomInfo() {
    tft.setTextColor(TFT_WHITE);
    tft.setFreeFont(FF9);
    tft.drawString("Rows:", 0, GAME_BOARD_HEIGHT + 10);
    tft.drawNumber(rowCount, 55, GAME_BOARD_HEIGHT + 10);
    tft.drawString("Score:", 90, GAME_BOARD_HEIGHT + 10);
    tft.drawNumber(score, 155, GAME_BOARD_HEIGHT + 10);
    tft.drawString("Develop By OliverCJ", 0, GAME_BOARD_HEIGHT + 30);
}

/**
 * 串口输出分数等信息
 */
void serialPrintScore() {
    Serial.print(level);
    Serial.print(",");
    Serial.print(rowCount);
    Serial.print(",");
    Serial.println(score);
}

/**
 * 绘制游戏边框
 */
void drawOutRect() {
    tft.fillRect(0, 0, GAME_BOARD_WIDTH + (GAME_BOARD_LINE_WIDTH * 2), GAME_BOARD_LINE_WIDTH, TFT_GREEN);                                                       // 上
    tft.fillRect(0, GAME_BOARD_HEIGHT + GAME_BOARD_LINE_WIDTH, GAME_BOARD_WIDTH + GAME_BOARD_LINE_WIDTH, GAME_BOARD_LINE_WIDTH, TFT_GREEN);                     // 下
    tft.fillRect(0, GAME_BOARD_LINE_WIDTH, GAME_BOARD_LINE_WIDTH, GAME_BOARD_HEIGHT, TFT_GREEN);                                                                // 左
    tft.fillRect(GAME_BOARD_WIDTH + GAME_BOARD_LINE_WIDTH, GAME_BOARD_LINE_WIDTH, GAME_BOARD_LINE_WIDTH, GAME_BOARD_HEIGHT + GAME_BOARD_LINE_WIDTH, TFT_GREEN); // 右
}

/**
 * 初始化游戏区记录
 */
void initGameBoard()
{
    gameBoard.usedRows = 0;
    for (size_t i = 0; i < gameBoard.totalRows; i++)
    {
        gameBoard.row[i] = 0;
    }
}

/**
 * 游戏结束
 */
void tipsGameOver() {
    // todo 提示游戏结束

    pauseGame();
    initGame();
}

/**
 * 暂停
 * 
 */
void tipsGamePause() {
    // todo 提示
    pauseGame();
}

/**
 * 暂停游戏
 * 
 */
void pauseGame() {
    while (true)
    {
        readButton();
        // 按B键重新开始
        if (buttonCatch == 6 && (millis() - lastButonPauseTime) >= 250)
        {
            lastButonPauseTime = millis();
            break;
        }
        delay(100);
    }
}

void loop() {
    drawGameBoard(); // 重绘游戏区域
    checkIsNeedGenerateNewShape(); // 检查是否需要重新产生方块
    calFalSpeed();  // 计算下落速度
    shapeFalling(); // 自动下落
    catchButton();  // 按键读取
    printScore(); // 分数显示
    delay(50);
}

/**
 * 检查是否需要生成信息的方块
 * 
 */
void checkIsNeedGenerateNewShape() {
    if (fallingShape.complete)
    {
        fallingShape = nextShape;
        // 擦除之前的下一个方块提示
        eraseTetrisShapeByPos(nextShape, GAME_BOARD_WIDTH + GAME_BOARD_LINE_WIDTH + 5, GAME_BOARD_LINE_WIDTH + 2);
        // 重新产生新的方块提示
        nextShape = getRandomNewShape();
        drawTetrisShapeByPos(nextShape, GAME_BOARD_WIDTH + GAME_BOARD_LINE_WIDTH + 5, GAME_BOARD_LINE_WIDTH + 2);
        // 如果新产生的方块直接发生边缘碰撞，则说明已经到顶
        if (!isValidPosition(0, 0))
        {
            // 最后剩余的空行数
            short lastEmptyRows = gameBoard.totalRows - gameBoard.usedRows;
            if (lastEmptyRows > 0)
            {
                // 为了保证完整描绘，取实际方块形状和剩余行数的高度差
                short heightDiff = fallingShape.sp.childShape[fallingShape.rotation].shapHeight - lastEmptyRows;
                if (heightDiff > 0)
                {
                    fallingShape.posY = fallingShape.posY - heightDiff * PIXEL_SIZE;
                }
            }
            drawTetrisShape(fallingShape);
            // 重新描绘边框，避免被覆盖
            drawOutRect();
            tipsGameOver();
        }
    }
}

/**
 * 显示分数，使用历史分数对比，避免一直刷新输出浪费资源
 * 
 */
void printScore() {
    if (rowCountCache != rowCount)
    {
        tft.fillRect(55, GAME_BOARD_HEIGHT + 10, 30, 20, TFT_BLACK);
        tft.drawNumber(rowCount, 55, GAME_BOARD_HEIGHT + 10);
        rowCountCache = rowCount;
    }
    if (scoreCache != score)
    {
        tft.fillRect(155, GAME_BOARD_HEIGHT + 10, 85, 20, TFT_BLACK);
        tft.drawNumber(score, 155, GAME_BOARD_HEIGHT + 10);
        scoreCache = score;
        // 分数变动输出到串口
        serialPrintScore();
    }
}

/**
 * @brief 读取rf24 joystick
 * 
 */
void readRF24Radio() {
    #if defined(USE_RF24)
    String msg;
    if (radio.available()) {
        radio.read(&receiveData, sizeof(receiveData));
        msg = receiveData;
        if (msg.startsWith("x-"))
        {
            xAxis = (msg.substring(2)).toInt();
            if (xAxis < JOYSTICK_ANGLOG_X_CENTER) // LEFT
            {
                buttonCatch = 4;
            }
            else if (xAxis > JOYSTICK_ANGLOG_X_CENTER) // RIGHT
            {
                buttonCatch = 2;
            } else
            {
                buttonCatch = 0;
            }
            
        } else if (msg.startsWith("y-"))
        {
            yAxis = (msg.substring(2)).toInt();
            if (yAxis < JOYSTICK_ANGLOG_Y_CENTER) // down
            {
                buttonCatch = 3;
            }
            else if (yAxis > JOYSTICK_ANGLOG_Y_CENTER) // up
            {
                buttonCatch = 1;
            } else {
                buttonCatch = 0;
            }
        } else if (msg == "A" || msg == "B" || msg == "C" || msg == "D")
        {
            buttonCatch = 5;
        }
        else if (msg == "start") // 暂停或开始
        {
            buttonCatch = 6;
        }
        else if (msg == "select") // 选择
        {
            /* code */
        } 
    }
    #endif
}

void readButton()
{
    buttonCatch = 0;
    #if defined(USE_RF24)
    readRF24Radio();
    #else
    if (digitalRead(BUTTON_PAUSE) == LOW) {
        buttonCatch = 6;
    }
    else if (digitalRead(BUTTON_CHANGE) == LOW || digitalRead(BUTTON_UP) == LOW)
    {
        buttonCatch = 5;
    }
    else if (digitalRead(BUTTON_DOWN) == HIGH)
    {
        buttonCatch = 0;
    }
    else if (digitalRead(BUTTON_DOWN) == LOW)
    {
        buttonCatch = 3;
    }
    else if (digitalRead(BUTTON_LEFT) == LOW)
    {
        buttonCatch = 4;
    }
    else if (digitalRead(BUTTON_RIGHT) == LOW)
    {
        buttonCatch = 2;
    }
    #endif
}

/**
 * 按钮读取，不同的按钮触发不同的效果
 */
void catchButton()
{
    // 等掉落到游戏区域内才能开始进行按钮触发
    if (fallingShape.posY <= GAME_BOARD_LINE_WIDTH) {
        return;
    }
    readButton();
    // 暂停按钮
    if (buttonCatch == 6 && (millis() - lastButonPauseTime) >= 250)
    {
        lastButonPauseTime = millis();
        return tipsGamePause();
    }
    // 旋转按钮
    if (buttonCatch == 5 && (millis() - lastButtonChangeTime) >= 250)
    {
        eraseTetrisShape(fallingShape); // 擦除
        fallingShape.rotation = (fallingShape.rotation + 1) % fallingShape.sp.childLength; // 循环旋转
        // 如果旋转后在边缘外，则转回去
        if (!isValidPosition(0, 0)) {
            fallingShape.rotation = (fallingShape.rotation - 1) % fallingShape.sp.childLength;
        }
        lastButtonChangeTime = millis();
        drawTetrisShape(fallingShape); // 重新绘制
        
    }
    // 中断持续向下，恢复刷新速率为当前正常速率
    if (buttonCatch == 0)
    {
        continueDownMoveReq = moveReq;
        continueDownCount = 0;
    }
    // 向下加速掉落
    if (buttonCatch == 3 && (millis() - lastButtonDownTime) >= continueDownMoveReq)
    {
        if (isValidPosition(0, PIXEL_SIZE))
        {
            eraseTetrisShape(fallingShape); // 擦除
            fallingShape.posY += PIXEL_SIZE;
            lastButtonDownTime = millis(); // 最后按向下的时间
            drawTetrisShape(fallingShape); // 重新绘制
            continueDownCount += 1;
            // 模拟重力加速，持续按键时间越长，下坠速度越快
            continueDownMoveReq -= continueDownCount * 20;
            if (continueDownMoveReq < 0)
            {
                continueDownMoveReq = 0;
            }
        }
    }
    // 向左移动
    else if (buttonCatch == 4 && (millis() - lastButtonLeftTime) >= moveReq)
    {
        if (isValidPosition(-PIXEL_SIZE, 0))
        {
            eraseTetrisShape(fallingShape); // 擦除
            fallingShape.posX -= PIXEL_SIZE;
            lastButtonLeftTime = millis(); // 最后按向左的时间
            drawTetrisShape(fallingShape); // 重新绘制
        }
    }
    // 向右移动
    else if (buttonCatch == 2 && (millis() - lastButtonRightTime) >= moveReq)
    {
        if (isValidPosition(PIXEL_SIZE, 0))
        {
            eraseTetrisShape(fallingShape); // 擦除
            fallingShape.posX += PIXEL_SIZE;
            lastButtonRightTime = millis(); // 最后按向右的时间
            drawTetrisShape(fallingShape);  // 重新绘制
        }
    }
}

/**
 * 边缘碰撞检测，返回true表示没有碰撞
 * 
 * @param adjustPosX 
 * @param adjustPosY 
 * @return true 
 * @return false 
 */
bool isValidPosition(int adjustPosX, int adjustPosY) {
    shapeItem fallingShapeItem = fallingShape.sp.childShape[fallingShape.rotation];

    for (size_t x = 0; x < fallingShapeItem.shapWidth; x++)
    {
        for (size_t y = 0; y < fallingShapeItem.shapHeight; y++)
        {
            // 检查对应方块位置是空的话则跳过
            if (bitRead(fallingShapeItem.p, y * (fallingShapeItem.shapWidth) + x) == 0)
            {
                continue;
            }
            int checkPosX = fallingShape.posX + x * PIXEL_SIZE + adjustPosX;
            int checkPosY = fallingShape.posY + y * PIXEL_SIZE + adjustPosY;
            // 检查是否在游戏区域内
            if (!checkIsOnGameBoard(checkPosX, checkPosY))
            {
                return false;
            }
            // 检查是否触及游戏区域已落下的方块
            int gameBoardRow = gameBoard.totalRows - ceil(checkPosY / PIXEL_SIZE) - 1;
            int gameBoardCol = ceil(checkPosX / PIXEL_SIZE);
            if (bitRead(gameBoard.row[gameBoardRow], gameBoardCol) == 1)
            {
                return false;
            }
        }
    }
    return true;
}

/**
 * 检查坐标是否在游戏区域内
 * 
 * @param posX 
 * @param posY 
 * @return true 
 * @return false 
 */
bool checkIsOnGameBoard(int posX, int posY) {
    return posX >= 2 && posX < (GAME_BOARD_WIDTH + GAME_BOARD_LINE_WIDTH) && posY >= 2 && posY < (GAME_BOARD_HEIGHT + GAME_BOARD_LINE_WIDTH);
}

/**
 * 方块自动下落
 */
void shapeFalling()
{
    // 下落
    if ((millis() - lastFallTime) >= fallSpeed)
    {
        if (!isValidPosition(0, PIXEL_SIZE)) // 检测是否已经触底
        {
            fallingShape.complete = true;
            addShapeToGameBoard(); // 将方块加入区域
            int removeLines = removeLine(); // 消除行计算
            rowCount += removeLines;
            currentLeveRowCount += removeLines;
            score += scoreBase[removeLines] * level; // 分数计算
        } else {
            if (fallingShape.posY < GAME_BOARD_LINE_WIDTH)
            {
                fallingShape.posY = GAME_BOARD_LINE_WIDTH;
            }
            else
            {
                eraseTetrisShape(fallingShape);
                fallingShape.posY += PIXEL_SIZE;
            }
            drawTetrisShape(fallingShape);
            lastFallTime = millis();
        }
    }
}

/**
 * 将已触底的方块加入到游戏存储区域
 * 同时更新已用到的行，便于后续绘制底图
 */
void addShapeToGameBoard() {
    shapeItem fallingShapeItem = fallingShape.sp.childShape[fallingShape.rotation];
    int gameBoardRow = gameBoard.totalRows - ceil(fallingShape.posY / PIXEL_SIZE) - 1;
    int gameBoardCol = ceil(fallingShape.posX / PIXEL_SIZE);
    if (gameBoard.usedRows < (gameBoardRow + 1))
    {
        gameBoard.usedRows = gameBoardRow + 1;
    }
    
    for (size_t y = 0; y < fallingShapeItem.shapHeight; y++)
    {
        for (size_t x = 0; x < fallingShapeItem.shapWidth; x++)
        {
            if (bitRead(fallingShapeItem.p, y * fallingShapeItem.shapWidth + x) == 1)
            {
                bitSet(gameBoard.row[gameBoardRow - y], gameBoardCol + x);
            }
        }
    }
}

/**
 * 计算是否有可以消除的行，并进行消除
 * 返回消除的行数
 * @return int 
 */
int removeLine() {
    int removeLines = 0;
    int removeCache[] = {};
    if (gameBoard.usedRows > 0) {
        // 从上向下遍历查找
        short usedRowForIndex = gameBoard.usedRows - 1;
        for (int i = usedRowForIndex; i > -1; i--)
        {
            // 检查行是否填充完毕，完毕后则需要消除
            if (checkLineComplete(i))
            {
                // 将这一行消除
                gameBoard.row[i] = 0;
                removeCache[removeLines] = i;
                // 将上面的移下来
                for (size_t j = i; j < usedRowForIndex; j++)
                {
                    gameBoard.row[j] = gameBoard.row[j + 1];
                }
                removeLines += 1;
            }
        }
        if (removeLines > 0) {
            // 移除效果
            drawRemoveGameBoardLine(removeLines, removeCache);
            for (size_t i = 0; i < removeLines; i++)
            {
                gameBoard.row[usedRowForIndex - i] = 0; // 顶部移除
            }
            drawGameBoardAll();
            gameBoard.usedRows -= removeLines;
        }
    }
    return removeLines;
}

/**
 * 消除行消除效果绘制（从左向右逐步消失）
 * 
 * @param removeLines 
 * @param removeLineCache 
 */
void drawRemoveGameBoardLine(int removeLines, int removeLineCache[])
{
    for (size_t i = 0; i < gameBoard.totalCols; i++)
    {
        int realPosX = GAME_BOARD_LINE_WIDTH + i * PIXEL_SIZE;
        for (size_t j = 0; j < removeLines; j++)
        {
            int realPosy = (GAME_BOARD_HEIGHT + GAME_BOARD_LINE_WIDTH) - (removeLineCache[j] + 1) * PIXEL_SIZE;
            drawTetrisPixel(realPosX, realPosy, TFT_BLACK);
        }
        delay(20);
    }
    
}

/**
 * 检查对应行是否填充满
 * 
 * @param row 
 * @return true 
 * @return false 
 */
bool checkLineComplete(int row) {
    for (size_t i = 0; i < gameBoard.totalCols; i++)
    {
        if (bitRead(gameBoard.row[row], i) == 0)
        {
            return false;
        }
    }
    return true;
}

/**
 * 基于清除的行数，计算游戏者的等级以及相应的下落速度
 */
void calFalSpeed(){
    // 根据当前等级消除行数，计算是否达到升级要求
    if (ceil(currentLeveRowCount / levelUpRows[level]) > 0)
    {
        level += 1;
    }
    if (level > MAX_LEVEL) {
        level = MAX_LEVEL;
    }
    fallSpeed = FALING_SPEED - (level * 50);
    moveReq = MOVE_FREQ - (level * 10);
    if (fallSpeed <= 80)
    {
        fallSpeed = 80;
    }
}

/**
 * 获取随机的新形状
 * 
 * @return shapeSelect 
 */
shapeSelect getRandomNewShape()
{
    randomSeed(analogRead(0));
    int shapeIndex = random(0, 7);
    shape getShape = shapesArray[shapeIndex];
    //int rotationIndex = random(0, getShape.childLength);
    shapeSelect select = {
        sp : getShape,
        rotation : 0,
        posX : ceil(GAME_BOARD_WIDTH / PIXEL_SIZE / 2 ) * PIXEL_SIZE + GAME_BOARD_LINE_WIDTH,
        posY : GAME_BOARD_LINE_WIDTH,
        complete: false,
    };
    return select;
}

/**
 *  绘制游戏区域, 仅绘制有填充颜色的
 *  主要用于循环中，减少绘制量，避免屏幕闪烁
 *  只绘制已用到行，节省资源
 */
void drawGameBoard() {
    for (size_t i = 0; i < gameBoard.usedRows && i < gameBoard.totalRows; i++)
    {
        for (size_t j = 0; j < gameBoard.totalCols; j++)
        {
            int realPosX = GAME_BOARD_LINE_WIDTH + j * PIXEL_SIZE;
            int realPosy = GAME_BOARD_HEIGHT + GAME_BOARD_LINE_WIDTH - (i + 1) * PIXEL_SIZE;
            if (bitRead(gameBoard.row[i], j) == 1)
            {
                drawTetrisPixel(realPosX, realPosy, tft.color565(192, 192, 192));
            }
        }
    }
}

/**
 * 绘制游戏区域，包括黑块和有颜色的，从下向上，从左向右绘制，
 * 消除后重绘使用，不可用于循环中使用，会导致闪烁
 * 只绘制已用到行，节省资源
 */
void drawGameBoardAll()
{
    for (size_t i = 0; i < gameBoard.usedRows && i < gameBoard.totalRows; i++)
    {
        for (size_t j = 0; j < gameBoard.totalCols; j++)
        {
            int realPosX = GAME_BOARD_LINE_WIDTH + j * PIXEL_SIZE;
            int realPosy = GAME_BOARD_HEIGHT + GAME_BOARD_LINE_WIDTH - (i + 1) * PIXEL_SIZE;
            if (bitRead(gameBoard.row[i], j) == 0)
            {
                drawTetrisPixel(realPosX, realPosy, TFT_BLACK);
            } else {
                drawTetrisPixel(realPosX, realPosy, tft.color565(192, 192, 192));
            }
        }
    }
}

/**
 * 根据方块内实时坐标位置擦除形状
 * 仅擦除方块所在位置，节省资源
 * @param select 
 */
void eraseTetrisShape(shapeSelect select)
{
    shape getShape = select.sp;
    shapeItem currentShape = select.sp.childShape[select.rotation];
    for (size_t i = 0; i < currentShape.shapHeight; i++)
    {
        for (size_t j = 0; j < currentShape.shapWidth; j++)
        {
            if (bitRead(currentShape.p, i * currentShape.shapWidth + j) != 0)
            {
                drawTetrisPixel(select.posX + j * PIXEL_SIZE, select.posY + i * PIXEL_SIZE, TFT_BLACK);
            }
        }
    }
}

/**
 * 擦除形状，根据目标位置,这时会忽略方块内实时的坐标位置
 * 仅擦除方块所在位置，节省资源
 * @param select 
 * @param posX 
 * @param posY 
 */
void eraseTetrisShapeByPos(shapeSelect select, int posX, int posY)
{
    shape getShape = select.sp;
    shapeItem currentShape = select.sp.childShape[select.rotation];
    for (size_t i = 0; i < currentShape.shapHeight; i++)
    {
        for (size_t j = 0; j < currentShape.shapWidth; j++)
        {
            if (bitRead(currentShape.p, i * currentShape.shapWidth + j) != 0)
            {
                drawTetrisPixel(posX + j * PIXEL_SIZE, posY + i * PIXEL_SIZE, TFT_BLACK);
            }
        }
    }
}

/**
 * 根据方块内实时坐标位置绘制方块
 * 仅绘制方块所在位置，节省资源
 * @param select 
 * @return char 
 */
char drawTetrisShape(shapeSelect select)
{
    shape getShape = select.sp;
    shapeItem currentShape = select.sp.childShape[select.rotation];
    // 绘制形状
    for (size_t i = 0; i < currentShape.shapHeight; i++)
    {
        for (size_t j = 0; j < currentShape.shapWidth; j++)
        {
            int realPosX = j * PIXEL_SIZE + select.posX;
            int realPosY = i * PIXEL_SIZE + select.posY;
            if (bitRead(currentShape.p, i * currentShape.shapWidth + j) != 0)
            {
                drawTetrisPixel(realPosX, realPosY, getShape.color);
            }
        }
    }

    return getShape.shapeChar;
}

/**
 * 在指定位置绘制方块，忽略实时坐标
 * 仅绘制方块所在位置，节省资源
 * @param select 
 * @param posX 
 * @param posY 
 * @return char 
 */
char drawTetrisShapeByPos(shapeSelect select, int posX, int posY)
{
    shape getShape = select.sp;
    shapeItem currentShape = select.sp.childShape[select.rotation];
    // 绘制形状
    for (size_t i = 0; i < currentShape.shapHeight; i++)
    {
        for (size_t j = 0; j < currentShape.shapWidth; j++)
        {
            int realPosX = j * PIXEL_SIZE + posX;
            int realPosY = i * PIXEL_SIZE + posY;
            if (bitRead(currentShape.p, i * currentShape.shapWidth + j) != 0)
            {
                drawTetrisPixel(realPosX, realPosY, getShape.color);
            }
        }
    }

    return getShape.shapeChar;
}

/**
 * 绘制像素块
 * 
 * @param x 
 * @param y 
 * @param color 
 */
void drawTetrisPixel(int x, int y, int color)
{
    tft.drawRect(x, y, PIXEL_SIZE, PIXEL_SIZE, TFT_BLACK);
    tft.fillRect(x + 1, y + 1, PIXEL_SIZE - 2, PIXEL_SIZE - 2, color);
}
