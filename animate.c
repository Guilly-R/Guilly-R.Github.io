#include "address_map_nios2.h"
#include "nios2_ctrl_reg_macros.h"
#include <stdlib.h>
#include <string.h>

//requirments, use led screen, use all buttons and switches

// Button map Swicthes 1-8 is number input, 9 is reset, Button 0 is enter and 1 is reset

volatile int timeout = 0;

#define HEX_BASE 0xFF200020
#define KEY_BASE 0xFF200050
#define SW_BASE  0xFF200040


#define CHAR_COLS 80
#define CHAR_ROWS 60

int resolution_x = 160;
int resolution_y = 120;

// VGA pointers
volatile char* VGA_CHAR = (volatile char*)FPGA_CHAR_BASE;
volatile short* VGA_PIX = (volatile short*)FPGA_ONCHIP_BASE;

int win_score = 10;   //  If implemnt high score you can just put 100

// Varable sectionsm 
int A, B;
int correct_answer;
char etext[32];

int score = 0;
int lives = 3;

int paused = 1;   // game starts paused until SW9 turned on
int LOSE = 0;
int WIN = 0; // 1 if ended by reaching win_score, 0 if by lives

// Wall animation  
int wall_left_x = 0;
int wall_right_x = 0;

#define WALL_WIDTH 6
#define WALL_SPEED 1

#define WALL_BLUE  0x001F
#define WALL_RED   0xF800
#define BG_COLOR   0x0000

int wall_step_counter = 0;
#define WALL_DELAY 2500   // adjust speed of wall movement (lower = faster)

// String section
void itoa_simple(int x, char* buf) {
    char tmp[12];
    int i = 0;
    if (x == 0) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    while (x > 0) {
        tmp[i++] = '0' + (x % 10);
        x /= 10;
    }
    int j = 0;
    while (i > 0) {
        buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
}

// Character buffer helpers 
void CHCLEAR(void) {
    for (int r = 0; r < CHAR_ROWS; r++)
        for (int c = 0; c < CHAR_COLS; c++)
            VGA_CHAR[(r << 7) + c] = ' ';
}

void VGA_clear_row(int y) {
    for (int c = 0; c < CHAR_COLS; c++)
        VGA_CHAR[(y << 7) + c] = ' ';
}

void VGA_text(int x, int y, const char* s) {
    int off = (y << 7) + x;
    while (*s) VGA_CHAR[off++] = *s++;
}


// Pixel buffer helpers 
void Pclear(void) {
    for (int row = 0; row < resolution_y; row++)
        for (int col = 0; col < resolution_x; col++)
            *(short*)(FPGA_ONCHIP_BASE + (row << 9) + (col << 1)) = BG_COLOR;
}

void WALL(int x1, int y1, int x2, int y2, short color) {
    for (int row = y1; row <= y2; row++)
        for (int col = x1; col <= x2; col++)
            *(short*)(FPGA_ONCHIP_BASE + (row << 9) + (col << 1)) = color;
}

void Aclear(void) {
    CHCLEAR();
    Pclear();
}

// Wall control 
void ReWall(void) {
    wall_left_x = 0;
    wall_right_x = resolution_x - WALL_WIDTH;
    Pclear();
    WALL(wall_left_x, 0, wall_left_x + WALL_WIDTH - 1, resolution_y - 1, WALL_BLUE);
    WALL(wall_right_x, 0, wall_right_x + WALL_WIDTH - 1, resolution_y - 1, WALL_RED);
    wall_step_counter = 0;
}

void Wallmove(void) {
    if (++wall_step_counter < WALL_DELAY) return;
    wall_step_counter = 0;

    // erase old
    WALL(wall_left_x, 0, wall_left_x + WALL_WIDTH - 1, resolution_y - 1, BG_COLOR);
    WALL(wall_right_x, 0, wall_right_x + WALL_WIDTH - 1, resolution_y - 1, BG_COLOR);

    // move in
    wall_left_x += WALL_SPEED;
    wall_right_x -= WALL_SPEED;

    // draw new
    WALL(wall_left_x, 0, wall_left_x + WALL_WIDTH - 1, resolution_y - 1, WALL_BLUE);
    WALL(wall_right_x, 0, wall_right_x + WALL_WIDTH - 1, resolution_y - 1, WALL_RED);
}

int WallE(void) { // Wall Center checker
    int center_x = resolution_x / 2;
    return (wall_left_x + WALL_WIDTH >= center_x) || (wall_right_x <= center_x);
}

// Seven-segment display for the score
static const unsigned char seven_seg[10] =
{ 0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F };

void segdisplay(int value) {
    volatile int* HEX = (volatile int*)HEX_BASE;
    int d0 = value % 10;
    int d1 = (value / 10) % 10;
    *HEX = seven_seg[d0] | (seven_seg[d1] << 8);
}

void equation(void) {
    A = (rand() % 12) + 1;
    B = (rand() % 12) + 1;
    int op = rand() & 1;   //If rand numb is 0 then add, if rand numb is 1 then mult, and no lets not do sub and divide

    if (op == 0)
        correct_answer = A + B;
    else
        correct_answer = A * B;

    char as[8], bs[8];
    itoa_simple(A, as);
    itoa_simple(B, bs);

    int i = 0;
    for (int X = 0; as[X]; X++) etext[i++] = as[X];
    etext[i++] = ' ';
    if (op == 0) {
        etext[i] = '+';
    }
    else {
        etext[i] = 'x';
    }
    i++;
    etext[i++] = ' ';
    for (int Y = 0; bs[Y]; Y++) etext[i++] = bs[Y];
    etext[i++] = ' ';
    etext[i++] = '=';
    etext[i++] = ' ';
    etext[i++] = '?';
    etext[i] = '\0';
}

void NEWGAME(void) {
    score = 0;
    lives = 3;
    LOSE = 0;
    WIN = 0;
    segdisplay(score);
    Aclear();
    equation();
    ReWall();
}

void PAUSEDS(void) {
    Aclear();
    VGA_text(26, 28, "|| To start switch SW9 on ||");
}

void GAMEOVER(void) {
    Aclear();
    VGA_text(34, 24, "*GAME OVER*");
    if (WIN)
        VGA_text(24, 26, "(o^^o) Score goal reached! (o^^o)");
    else
        VGA_text(26, 26, "xnx No lives remaining! xnx");
    VGA_text(27, 29, "Press Button 1 to restart");
}

int main(void) {

    volatile int* KEY = (volatile int*)KEY_BASE;
    volatile int* SW = (volatile int*)SW_BASE;

    srand(1);
    Aclear();
    segdisplay(0);
    VGA_text(26, 28, "Flip SW9 to start");

    int prev_keys = 0xFFFFFFFF;
    int prev_sw9 = 0;

    const int EQ_Y = 20;
    const int ANS_Y = 23;
    const int LIFE_Y = 25;

    while (1) {
        int keys = *KEY;
        int sw = *SW;
        int sw9 = (sw >> 9) & 1;

        int key0_now = ((keys & 0x1) == 0);
        int key1_now = ((keys & 0x2) == 0);
        int key0_prev = ((prev_keys & 0x1) == 0);
        int key1_prev = ((prev_keys & 0x2) == 0);

        if (LOSE) {
            GAMEOVER();
            // restart only on KEY1
            if (!key1_prev && key1_now) {
                paused = 1;          // require SW9 to start again
                NEWGAME();    // reset score/lives/equation/walls
                paused = 1;
                VGA_text(26, 28, "Flip SW9 to start");
            }
            prev_keys = keys;
            prev_sw9 = sw9;
            continue;
        }

        //  SW9 START / PAUSE 
        if (!prev_sw9 && sw9) {   // rising edge: SW9 turned ON
            paused = 0;
            NEWGAME();
        }

        if (prev_sw9 && !sw9) {   // falling edge: SW9 turned OFF
            paused = 1;
            PAUSEDS();
        }

        //  KEY1 reset while running (when SW9 ON) 
        if (!key1_prev && key1_now && sw9 && !paused) {
            NEWGAME();
        }

        if (paused) {
            prev_keys = keys;
            prev_sw9 = sw9;
            continue;
        }


        // wall movers
        Wallmove();

        // Wall checker
        if (WallE()) {
            lives--;
            if (lives <= 0) {
                LOSE = 1;
                WIN = 0;
                prev_keys = keys;
                prev_sw9 = sw9;
                continue;
            }
            else {
                Aclear();
                equation();
                ReWall();
            }
        }

        //  Display equation, answer and lives
        VGA_clear_row(EQ_Y);
        VGA_text((CHAR_COLS / 2) - (strlen(etext) / 2),
            EQ_Y,
            etext);

        int user_answer = sw & 0xFF; // SW0-SW7
        char buf[8];

        VGA_clear_row(ANS_Y);
        itoa_simple(user_answer, buf);
        VGA_text(30, ANS_Y, "Your Answer:");
        VGA_text(46, ANS_Y, buf);

        VGA_clear_row(LIFE_Y);
        char life_line[16] = "Lives: ";
        itoa_simple(lives, buf);
        strcat(life_line, buf);
        VGA_text((CHAR_COLS / 2) - (strlen(life_line) / 2),
            LIFE_Y,
            life_line);

        // KEY0 to enter answer
        if (!key0_prev && key0_now) {
            CHCLEAR();   // clear text (not pixels) so walls persist

            if (user_answer == correct_answer) {
                score++;
                segdisplay(score);

                if (score >= win_score) {
                    LOSE = 1;
                    WIN = 1;
                    prev_keys = keys;
                    prev_sw9 = sw9;
                    continue;
                }

                // Correct: new equation and reset walls
                equation();
                ReWall();

            }
            else {
                // Incorrect: lose life, DO NOT reset walls
                lives--;
                if (lives <= 0) {
                    LOSE = 1;
                    WIN = 0;
                    prev_keys = keys;
                    prev_sw9 = sw9;
                    continue;
                }
                // If still alive, keep current equation & walls
            }
        }

        prev_keys = keys;
        prev_sw9 = sw9;
    }

    return 0;
}