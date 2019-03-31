#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "struct.h"
#include <string.h>
#include <math.h>

//function for image input
//load initial image for instructions


//initialize instructions
INST instruction[13] = {
        {.instruction = "Not Left", .answer = 1},
        {.instruction = "Not Right", .answer = 0},
        {.instruction = "Not Not up", .answer = 2},
        {.instruction = "Not Down", .answer = 2},
        {.instruction = "Not Not Down", .answer = 3},
        {.instruction = "Not Up", .answer = 3},
        {.instruction = "Not Not Left", .answer = 0},
        {.instruction = "Not Not Right", .answer = 1},
        {.instruction = "Not Not Not Left", .answer = 1},
        {.instruction = "Not Not Not Right", .answer = 0},
        {.instruction = "Not Not Not Up", .answer = 3},
        {.instruction = "Not Not Not Down", .answer = 2},
        {.instruction = "Not Not Not Down", .answer = 2},
        {.instruction = "Not Not Not Down", .answer = 2}
};

//interval timer to count the reaction time down

// global variables for I/O devices addresses
volatile int *LEDR_ptr = (int *) 0xFF200000;
volatile int *SW_ptr = (int *) 0xFF200040;
volatile int *KEY_EDGE_ptr = (int *) 0xFF20005C;
volatile char *character_buffer = (char *) 0xC9000000;// VGA character buffer
volatile int *pixel_ctrl_ptr = (int *) 0xFF203020; // pixel controller
volatile int pixel_buffer_start;


// subroutine for plotting text on the screen
void VGA_text(int x, int y, char *text_ptr);

// subroutine for plotting a pixel on the screen
void plot_pixel(int x, int y, short int line_color) {
    *(short *) (pixel_buffer_start + (y << 10) + (x << 1)) = line_color;
}

// subroutine for plotting an image given a specific location and image array
void plot_image(int initialX, int initialY, int imageArray[], unsigned width, unsigned height);

// subroutine for clearing the whole screen by writing black to every pixel
void clear_screen();

// boolean function for switching the front&back VGA buffer,
// and return to the caller when the plotting is finished
bool wait_for_vsync();

// global varible for determining the game state
bool gameOn = false;
bool gameOver = false;

void wait_for_response(){
    while(1){
        if(KEY_EDGE_ptr!=0){
            return;
        }
    }
}

int main() {

    /* set front pixel buffer to start of FPGA On-chip memory */
    *(pixel_ctrl_ptr + 1) = 0xC8000000; // first store the address in the
    // back buffer
    /* now, swap the front/back buffers, to set the front buffer location */
    wait_for_vsync();
    /* initialize a pointer to the pixel buffer, used by drawing functions */
    pixel_buffer_start = *pixel_ctrl_ptr;
    clear_screen(); // pixel_buffer_start points to the pixel buffer
    /* set back pixel buffer to start of SDRAM memory */
    *(pixel_ctrl_ptr + 1) = 0xC0000000;

    unsigned SW_value;
//    unsigned score;
//    unsigned score_hundred = 0;
//    unsigned score_ten = 0;
//    unsigned score_one = 0;
//    unsigned oneSecCount;

    start:
    {
        unsigned SW_value = (unsigned int) *SW_ptr;// read SW
        //load start page
        if(SW_value ==1){
            goto new_game;
        }else{
            goto start;
        }
    }

    new_game:
    {
        int count=0;
        bool moveOn = true;
        //load game page without instruction title here

        while (moveOn) {
            int i = rand() % 14;
            char *textOutput = instruction[i].instruction;
            //giving time to wait for the user response
            wait_for_response();

            //if edge capture
            int key_pressed = -1;
            if (KEY_EDGE_ptr == 0x0001) {
                key_pressed = 0;
            } else if (KEY_EDGE_ptr == 0x0010) {
                key_pressed = 1;
            } else if (KEY_EDGE_ptr == 0x0100) {
                key_pressed = 2;
            } else {
                key_pressed = 3;
            }
            //clear capture

            //compare result with the key_pressed
            if (key_pressed == instruction[i].answer) {
                //show correct response
                count++;

            } else {
                //show wrong response
                goto game_over;
            }
            //clear edge_capture
            int KEY_release = *KEY_EDGE_ptr;
            *KEY_EDGE_ptr = 0xF;

            unsigned SW_value = (unsigned int) *SW_ptr;// read SW
            if (SW_value == 0){
                goto start;
            }

        }
    }
    game_over:
    {
        //show game over image
        //display count on VGA
        if(SW_value == 0){
            goto start;
        }else{
            goto game_over;
        }
    }
}

// subroutine for plotting text on the screen
void VGA_text(int x, int y, char *text_ptr) {
    /* assume that the text string fits on one line */
    int offset = (y << 7) + x;

    while (*(text_ptr)) // while it hasn't reach the null-terminating char in the string
    {
        // write to the character buffer
        *(character_buffer + offset) = *(text_ptr);
        ++text_ptr;
        ++offset;
    }
}


// function for swapping two intergers
void swap(int *left, int *right) {
    int temp = *left;
    *left = *right;
    *right = temp;
}



// subroutine for plotting an image given a specific location and image array
void plot_image(int initialX, int initialY, int imageArray[], unsigned width, unsigned height) {

    int i = 0; // index for pixel colours in the image array

    for (unsigned y = 0; y < height; y++) {
        for (unsigned x = 0; x < width; x++) {
            int plotX = initialX + x;
            int plotY = initialY + y;

            // check for magenta, which is selected as a substitute of the alpha(transparent) colour
            // when the pixel is out of bound, ignore it
            if (imageArray[i] != 0b1111100000011111 && plotX >= 0 && plotY >= 0 && plotX < 320 && plotY < 240)
                plot_pixel(plotX, plotY, imageArray[i]);

            i++; // switch to the next pixel colour
        }
    }
}

// subroutine for clearing the whole screen by writing black to every pixel
void clear_screen() {
    for (int y = 0; y < 240; y++) {
        for (int x = 0; x < 320; x++) {
            plot_pixel(x, y, 0);
        }
    }
}

// boolean function for switching the front&back VGA buffer,
// and return to the caller when the plotting is finished
bool wait_for_vsync() {
    // register for storing the plotting status
    register int status;

    // write to switch the front&back VGA buffer
    *pixel_ctrl_ptr = 1;

    // keep getting the plotting status until the plotting is finished
    // which is denoted by status "1"
    status = *(pixel_ctrl_ptr + 3);
    while ((status & 0x01) != 0) {
        status = *(pixel_ctrl_ptr + 3);
    }

    return true;
}





