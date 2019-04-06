#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "struct.h"
#include "image.h"
#include <string.h>


//initialize instructions and the corresponding answers
;INST instruction[19] = {
        {.instruction = "Not Not Not Not Not Up", .answer = 3},
        {.instruction = "Not Left", .answer = 1},
        {.instruction = "Not Not Not Not Left", .answer = 0},
        {.instruction = "Not Not Not Not Not Not Up", .answer = 2},
        {.instruction = "Not Not Not Left", .answer = 1},
        {.instruction = "Not Not Up", .answer = 2},
        {.instruction = "Not Down", .answer = 2},
        {.instruction = "Not Not Not Down", .answer = 2},
        {.instruction = "Not Not Not Not Not Down", .answer = 2},
        {.instruction = "Not Not Not Right", .answer = 0},
        {.instruction = "Not Not Down", .answer = 3},
        {.instruction = "Not Up", .answer = 3},
        {.instruction = "Not Not Left", .answer = 0},
        {.instruction = "Not Right", .answer = 0},
        {.instruction = "Not Not Not Not Not Right", .answer = 0},
        {.instruction = "Not Not Right", .answer = 1},
        {.instruction = "Not Not Not Up", .answer = 3},
        {.instruction = "Not Not Not Not Down", .answer = 3},
        {.instruction = "Not Not Not Not Not Not Left", .answer = 0}
};


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

//subroutine for waitting for user's response
void wait_for_response();

// global variable for determining the game state
bool gameOver = false;

//function to clear the text
void VGA_text_clean() {
    /* assume that the text string fits on one line */
    for(int y=0;y<60;y++){
        for(int x=0;x<80;x++){
            int offset = (y << 7) + x;
            *(character_buffer + offset) = 0;
            ++offset;
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

    int count = 0; //count for the score


    /* Start subroutine
     * Draw the start page, clear all data*/
    start:
    {
        VGA_text_clean(); //clear the text
        count = 0; //clear the score
        unsigned SW_value = (unsigned int) *SW_ptr;// read SW value
        plot_image(0,0, start_page_320x240, 320, 240); //plot the start page

        if(SW_value ==1){ //if SW0 is turned on
            goto new_game; //branch to new game
        } else{ //else stay in this routine
            wait_for_vsync(); // swap front and back buffers on VGA vertical sync
            pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
            goto start;
        }
    }

    /* New game subroutine
     * Here's the main game*/
    new_game:
    {
        gameOver = false; //game over boolean

        //load game page without instruction title here
        plot_image(0,0, game_page_320x240, 320, 240);
        wait_for_vsync(); // swap front and back buffers on VGA vertical sync
        pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer

        while (true) { //infinite loop
            int i = rand() % 19; //random instruction

            //vga text goes here:
            VGA_text_clean();
            VGA_text(32, 10, instruction[i].instruction);

            //giving time to wait for the user response
            wait_for_response();
            //when return -> key is pressed, or SW0 is turned off
            unsigned SW_value = (unsigned int) *SW_ptr;// read SW0 value
            if(SW_value != 1) { //if SW0 is turned off, restart the game; branch to start rountine
                goto start;
            } else { //else if the key pressed

                /*Determine which key is pressed*/
                int key_pressed = -1;
                if (*KEY_EDGE_ptr == 0b0001) { //KEY0
                    key_pressed = 0;
                } else if (*KEY_EDGE_ptr == 0b0010) { //KEY1
                    key_pressed = 1;
                } else if (*KEY_EDGE_ptr == 0b0100) { //KEY2
                    key_pressed = 2;
                } else if (*KEY_EDGE_ptr == 0b1000){ //KEY3
                    key_pressed = 3;
                }
                //clear capture
                *KEY_EDGE_ptr = 0xF;

                //compare the correct answer with the key_pressed
                if (key_pressed == instruction[i].answer) {
                    //show correct response
                    count++;
                } else {
                    //show wrong response
                    gameOver = true; //game is over
                    goto game_over;
                }
            }
        }
    }

    /* Game over subroutine*/
    game_over:
       if(gameOver) {
           VGA_text_clean();

           gameOverloop:
               //plot the game_over image
               plot_image(0,0, game_over_320x240, 320, 240);

               /*count the score*/
               char score_hundred = count / 100; //divide hundredth
               char score_ten = (count - score_hundred * 100) / 10; //divide tenth
               char score_one = count - score_hundred * 100 - score_ten * 10;  //divide ones
               char myScoreString[40]; //declare score string
               if (score_hundred != 0) {
                   myScoreString[0] = score_hundred + '0';
               } else {
                   myScoreString[0] = ' ';
               }
               if (score_hundred == 0 && score_ten == 0) {
                   myScoreString[1] = ' ';
               } else {
                   myScoreString[1] = score_ten + '0';
               }
               myScoreString[2] = score_one + '0';
               myScoreString[3] = '\0';

               //display the score
               VGA_text(42,29,myScoreString);

               int SW_value = *SW_ptr;
               //display count on VGA
               if(SW_value == 0){
                   goto start;
               } else{
                   wait_for_vsync(); // swap front and back buffers on VGA vertical sync
                   pixel_buffer_start = *(pixel_ctrl_ptr + 1); // new back buffer
                   goto gameOverloop;
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

void wait_for_response(){
    while(1){ //infinite loop
        unsigned SW_value = (unsigned int) *SW_ptr;// read SW
        if(*KEY_EDGE_ptr!=0){
            return;
        } else {
            //do nothing
        }
        if(SW_value != 1) {
            return;
        }
    }
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


