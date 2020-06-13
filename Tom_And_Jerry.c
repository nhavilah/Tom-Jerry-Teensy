/*
Nicholas Havilah
N10469231
CAB202 Assignment 2
*/
#include <stdint.h>
#include <stdio.h>
#include <avr/io.h> 
#include <avr/interrupt.h>
#include <util/delay.h>
#include <cpu_speed.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <graphics.h>
#include <macros.h>
#include "lcd_model.h"
#include "cab202_adc.h"
#include "usb_serial.h"
#include <string.h>
#define FREQ (8000000.0)
#define PRESCALE (8) 

#ifndef M_PI
#define M_PI        3.14159265358979323846264338327950288   /* pi             */
#endif

//define a list of global variables to be used in the game
uint8_t levelnumber = 1;//current level number
uint8_t lives = 5;//remaining lives
uint8_t score = 0;//current score
uint8_t cheese_consumed=0;
int seconds_tens = 0;//used for the seconds part of the game timer
int seconds_ones = 0;//used for the seconds part of the game timer
int minutes_tens = 0;//used for the minutes part of the game timer
int minutes_ones = 0;//used for the minutes part of the game timer
int angle;//defines the angle at which tom moves through the playspace
uint8_t cheese_count=0;//keeps track of the cheese on screen
uint8_t trap_count=0;//keeps track of the traps on screen
uint8_t firework_count=0;//keeps track of the fireworks on screen
uint8_t bowl_Count=0;//keep track of the bowls on screen
double lastCheeseDrawn;//keeps track of the time since cheese was drawn
double lastMouseTrapDrawn;//keeps track of time since the last trap was drawn
double lastBowlDrawn;//keeps track of time since last bowl was drawn
double disabling_time;//keeps track of time until superjerry mode runs out
double tom_dx,tom_dy;//values that convert the angle to x and y values that can be added to coordinates of tom
//define a series of arrays to store the positions of tom and jerry's pixels
//jerry
double jerry_speed;
double jerry_x_coords[13];
double jerry_y_coords[13];
double original_jerry_x_coords[13];
double original_jerry_y_coords[13];
//door
double door_x,door_y;
//tom
double tom_speed;
double tom_x_coords[6];
double tom_y_coords[6];
double original_tom_x_coords[6];
double original_tom_y_coords[6];
//fireworks
double fireworks_x[20],fireworks_y[20],fireworks_dx[20],fireworks_dy[20];
double x_distance[20],y_distance[20],line_distance[20];
//bowl
double bowl_x,bowl_y;
//define a series of int arrays to store the wall positions
double wall_1[4]={18,15,13,25};
double wall_2[4]={25,35,25,45};
double wall_3[4]={45,10,60,10};
double wall_4[4]={58,25,72,30};
//define a series of doubles to use for the gradients and y intercepts of the walls(for collision detection)
double wall_1_m,wall_4_m,wall_1_c,wall_4_c;
//int arrays to store the registers
int PIN[7];
int PIN_NO[7];
//cheese
uint8_t cheese_x[5],cheese_y[5],mousetrap_x[5],mousetrap_y[5];
//bools to start certain parts of gameplay functionality
bool fired = false;//fires the fireworks
bool gamestarted=false;//use this variable to check if the game has started
bool isPaused = false;//use this to puase and unpause the game
bool jerry_hit_wall,tom_hit_wall,firework_wall_collided,cheese_hit_wall;//used for wall collision detection
bool superjerryMode=false;//determines if superjerry mode is active
bool game_over=false;
volatile uint8_t bit_counter[7];//used for the game timer
volatile uint8_t switch_closed[7]; //stores the state of the current switch

void draw_double(uint8_t x, uint8_t y, double value, colour_t colour);
void draw_int(uint8_t x, uint8_t y, int value, colour_t colour);
char buffer[20];
char putty_buffer[100];
//setup door in a random location
void setup_door(){
	door_x = rand()%(LCD_X-3);
	door_y=rand()%(LCD_Y-3);
	if(door_y <= 9){
		door_y = rand()%(LCD_Y-3);
	}
}
//draws the door
void draw_door(){
	if(cheese_consumed >=5){
		draw_pixel(door_x,door_y,FG_COLOUR);
		draw_pixel(door_x+1,door_y,FG_COLOUR);
		draw_pixel(door_x,door_y+1,FG_COLOUR);
		draw_pixel(door_x+1,door_y+1,FG_COLOUR);
		draw_pixel(door_x,door_y+2,FG_COLOUR);
		draw_pixel(door_x+1,door_y+2,FG_COLOUR);
	}
}
//sets up toms speed and direction
void setup_tom(){
	angle = rand() %360;//setup a random angle between 0 and 360 degrees(for the randomised direction
	if(levelnumber==1){
		tom_speed = fmod(rand(),0.4)+0.1;
	}
}
//runs the setup code for the game
void setup(void) {
	set_clock_speed(CPU_8MHz);
	lcd_init(LCD_DEFAULT_CONTRAST);
	lcd_clear();
	//Initialise timer 0 in normal mode so it overflows
	TCCR0A = 0;
	TCCR0B = 4;
	//Enable timer overflow for Timer 0 and Timer 3 and timer 1
	TIMSK0 = 1;
	//Turn on interrupts
	sei();
	//for each element in the bit_counter array, set the value to 0
	for(int i = 0; i < 7;i ++){
		bit_counter[i] = 0;
	}
	//turn on the backlight
	SET_BIT(PORTC,7);
	//setup the leds
	SET_BIT(DDRB, 2);
	SET_BIT(DDRB, 3);
	//Enable all switches to be process as input
	CLEAR_BIT(DDRB,0);//center joystick button
	CLEAR_BIT(DDRB,7);//down joystick button
	CLEAR_BIT(DDRB,1);//left joystick button
	CLEAR_BIT(DDRD,0);//right joystick button
	CLEAR_BIT(DDRD,1);//up joystick button
	//enable the switch buttons for input
	CLEAR_BIT(DDRF,5);//right switch
	CLEAR_BIT(DDRF,6);//left switch
	//initialise adc to use the potentiometers
	adc_init();
	//draw the splash screen for the game
	draw_string(0,0,"NICHOLAS HAVILAH",FG_COLOUR);//student name(shortened so it could fit on the splash screen
	draw_string(20,15,"N10469231",FG_COLOUR);//student number
	draw_string(17,30,"T&J MAYHEM",FG_COLOUR);//game name
	//setup tom to move randomly
	setup_tom();
	//setup the door to spawn randomly
	setup_door();
	//setup jerry's pixel x positions
	jerry_x_coords[0] = 0;
	jerry_x_coords[1]=jerry_x_coords[0];
	jerry_x_coords[2]=jerry_x_coords[0]+1;
	jerry_x_coords[3]=jerry_x_coords[0]+1;
	jerry_x_coords[4]=jerry_x_coords[0]+1;
	jerry_x_coords[5]=jerry_x_coords[0]+2;
	jerry_x_coords[6]=jerry_x_coords[0]+2;
	jerry_x_coords[7]=jerry_x_coords[0]+2;
	jerry_x_coords[8]=jerry_x_coords[0]+3;
	jerry_x_coords[9]=jerry_x_coords[0]+3;
	jerry_x_coords[10]=jerry_x_coords[0]+3;
	jerry_x_coords[11]=jerry_x_coords[0]+4;
	jerry_x_coords[12]=jerry_x_coords[0]+4;
	//setup jerry's pixel y positions
	jerry_y_coords[0] = 9;
	jerry_y_coords[1]=jerry_y_coords[0]+1;
	jerry_y_coords[2]=jerry_y_coords[0]+1;
	jerry_y_coords[3]=jerry_y_coords[0]+2;
	jerry_y_coords[4]=jerry_y_coords[0]+3;
	jerry_y_coords[5]=jerry_y_coords[0]+1;
	jerry_y_coords[6]=jerry_y_coords[0]+2;
	jerry_y_coords[7]=jerry_y_coords[0]+3;
	jerry_y_coords[8]=jerry_y_coords[0]+1;
	jerry_y_coords[9]=jerry_y_coords[0]+2;
	jerry_y_coords[10]=jerry_y_coords[0]+3;
	jerry_y_coords[11]=jerry_y_coords[0];
	jerry_y_coords[12]=jerry_y_coords[0]+1;
	//setup tom's x pixel positions
	tom_x_coords[0] = LCD_X-5;
	tom_x_coords[1] = tom_x_coords[0]+1;
	tom_x_coords[2] = tom_x_coords[0]+1;
	tom_x_coords[3] = tom_x_coords[0]+1;
	tom_x_coords[4] = tom_x_coords[0]+1;
	tom_x_coords[5] = tom_x_coords[0]+2;
	//setup tom's y pixel positions
	tom_y_coords[0] = LCD_Y-9;
	tom_y_coords[1] = tom_y_coords[0];
	tom_y_coords[2] = tom_y_coords[0]+1;
	tom_y_coords[3] = tom_y_coords[0]+2;
	tom_y_coords[4] = tom_y_coords[0]+3;
	tom_y_coords[5] = tom_y_coords[0];
	for(int i = 0; i < 6; i++){
		original_tom_x_coords[i] = tom_x_coords[i];
		original_tom_y_coords[i] = tom_y_coords[i];
	}
	for(int i = 0; i < 13; i++){
		original_jerry_x_coords[i] = jerry_x_coords[i];
		original_jerry_y_coords[i] = jerry_y_coords[i];
	}
	//show the screen
	show_screen();
}
ISR(TIMER0_OVF_vect) {
	//assign the inputs to the correct registers
	//center joystick
	PIN[0]=PINB;
	PIN_NO[0]=0;
	//left joystick
	PIN[1]=PINB;
	PIN_NO[1]=1;
	//right joystick
	PIN[2]=PIND;
	PIN_NO[2]=0;
	//upper joystick
	PIN[3]=PIND;
	PIN_NO[3]=1;
	//lower joystick
	PIN[4]=PINB;
	PIN_NO[4]=7;
	//left button
	PIN[5]=PINF;
	PIN_NO[5]=6;
	//right button
	PIN[6]=PINF;
	PIN_NO[6]=5;
	//updates the switch closed state for each of the teensy siwtches
	for(int i = 0; i < 7; i++){
		//left shifts the bit counter by 1
		bit_counter[i] = bit_counter[i] << 1;
		//bit masks it so after 3 bounces it is considered closed
		bit_counter[i] = bit_counter[i] & 0b00000111;
		bit_counter[i] |= BIT_IS_SET(PIN[i], PIN_NO[i]);
		if (bit_counter[i] == 0b00000111)
		{
			switch_closed[i] = 1;
		}
		if (bit_counter[i] == 0)
		{
			switch_closed[i] = 0;
		}
	}
}
volatile uint32_t cycle_count = 0;
volatile uint32_t superMode_count = 0;
//timer interrupts for timer 3(game clock)
ISR(TIMER3_OVF_vect){
	//wrap the timer functionality in a bool checker that allows the timer to be paused and unpaused
	if(isPaused == false){
		cycle_count++;
	}
	superMode_count++;
}
//converts the timers into the seconds format
//sets up timer for superjerry mode
double get_elapsed_supermode_time(){
	return ( ( superMode_count * 65536.0 + TCNT3 ) * PRESCALE  / FREQ);
}
//sets up game time
double get_elapsed_time(){
	return ( ( cycle_count * 65536.0 + TCNT3 ) * PRESCALE  / FREQ);
}
void draw_double(uint8_t x, uint8_t y, double value, colour_t colour) {
	snprintf(buffer, sizeof(buffer), "%f", value);
	draw_string(x, y, buffer, colour);
}
void draw_int(uint8_t x, uint8_t y, int value, colour_t colour) {
	snprintf(buffer, sizeof(buffer), "%d", value);
	draw_string(x, y, buffer, colour);
}
//sets up bowl to spawn after 5 seconds(for superjerry mode)
void setup_bowl(){
	if(levelnumber==2){
		if(get_elapsed_time()-lastBowlDrawn>=5 && bowl_Count < 1){
			lastBowlDrawn=get_elapsed_time();
			bowl_x=tom_x_coords[3];
			bowl_y=tom_y_coords[3];
			if(bowl_Count==1){
				lastBowlDrawn=get_elapsed_time();
			}
			bowl_Count++;
		}
	}
}
//draw the bowl
void draw_bowl(){
	draw_pixel(bowl_x,bowl_y,FG_COLOUR);
	draw_pixel(bowl_x+1,bowl_y+1,FG_COLOUR);
	draw_pixel(bowl_x+2,bowl_y,FG_COLOUR);
}
//setup mousetraps to spawn 3 seconds after each other
void setup_mousetrap(){
	if(get_elapsed_time()-lastMouseTrapDrawn>=3 && trap_count < 5){
		lastMouseTrapDrawn=get_elapsed_time();
		mousetrap_x[trap_count]=tom_x_coords[3];
		mousetrap_y[trap_count]=tom_y_coords[3];
		if(trap_count==5){
			lastMouseTrapDrawn=get_elapsed_time();
		}
		trap_count++;
	}
}
//setup the usb connection
void setup_usb_connection(){
	usb_init();
	while(!usb_configured()){
		//block until usb is ready
	}
	draw_string(25,25,"YES",FG_COLOUR);
}
//send messages to the terminal
void usb_serial_send(char * message){
	//send the message to the putty terminal
	usb_serial_write((uint8_t *) message, strlen(message));
}
//draw the mousetraps
void draw_trap(){
	//draw_int(25,25,cheese_time_goal,FG_COLOUR);
	for(uint8_t i = 0; i < trap_count;i++){
		draw_pixel(mousetrap_x[i],mousetrap_y[i],FG_COLOUR);
		draw_pixel(mousetrap_x[i]+1,mousetrap_y[i]-1,FG_COLOUR);
		draw_pixel(mousetrap_x[i]+1,mousetrap_y[i],FG_COLOUR);
		draw_pixel(mousetrap_x[i]+2,mousetrap_y[i],FG_COLOUR);
	}
}
//send game state to the terminal
void send_game_state(){
	int16_t char_code=usb_serial_getchar();
	if(char_code=='i'){
		if(char_code>=0){
			//print the timestamp
			snprintf(putty_buffer,sizeof(putty_buffer),"Time: %d%d:%d%d ",minutes_tens,minutes_ones,seconds_tens,seconds_ones);
			usb_serial_send(putty_buffer);
			//print the level numbers
			snprintf(putty_buffer,sizeof(putty_buffer),"Level: %d ",levelnumber);
			usb_serial_send(putty_buffer);
			//print the remaining lives
			snprintf(putty_buffer,sizeof(putty_buffer),"Lives Remaining: %d ",lives);
			usb_serial_send(putty_buffer);
			//print the remaining fireworks
			snprintf(putty_buffer,sizeof(putty_buffer),"Fireworks Used: %d ",firework_count);
			usb_serial_send(putty_buffer);
			//print the mousetraps active on screen
			snprintf(putty_buffer,sizeof(putty_buffer),"Mousetraps On Screen: %d ",trap_count);
			usb_serial_send(putty_buffer);
			//print the cheese active on the screen
			snprintf(putty_buffer,sizeof(putty_buffer),"Cheese On Screen: %d ",cheese_count);
			usb_serial_send(putty_buffer);
			//print the amount of cheese eaten
			snprintf(putty_buffer,sizeof(putty_buffer),"Cheese Eaten: %d ",cheese_consumed);
			usb_serial_send(putty_buffer);
			//print the remaining fireworks to use
			snprintf(putty_buffer,sizeof(putty_buffer),"Fireworks Used: %d ",firework_count);
			usb_serial_send(putty_buffer);
			//create char strings for the bools(because bools can't be printed this way)
			char *super_jerry_active;
			char *game_paused;
			if(superjerryMode==true){
				super_jerry_active="true";
			}else{
				super_jerry_active="false";
			}
			if(isPaused==true){
				game_paused="true";
			}else{
				game_paused="false";
			}
			//print the superjerry mode state
			snprintf(putty_buffer,sizeof(putty_buffer),"Super Jerry Mode: %s ",super_jerry_active);
			usb_serial_send(putty_buffer);
			//print the paused state
			snprintf(putty_buffer,sizeof(putty_buffer),"Is Paused: %s ",game_paused);
			usb_serial_send(putty_buffer);
		}
	}
}
//create a wall collision function where positions are inputted into and values are returned from it
void wall_collision(double character_x_pos,double character_y_pos,char character){
	if(round(character_x_pos)<=wall_1[0] && round(character_x_pos)>=wall_1[2] && round(character_y_pos)==round((wall_1_m * character_x_pos) + wall_1_c)){
		if(character=='J' && !superjerryMode){
			jerry_hit_wall=true;
		}else if(character=='T'){
			tom_hit_wall=true;
		}else if(character=='F'){
			firework_wall_collided=true;
		}else if(character=='C'){
			cheese_hit_wall=true;
		}
	}
	//collision for wall 2
	else if(round(character_x_pos) >= wall_2[0] && round(character_x_pos) <= wall_2[2] && round(character_y_pos) >= wall_2[1] && round(character_y_pos) <= wall_2[3]){
		if(character=='J' && !superjerryMode){
			jerry_hit_wall=true;
		}else if(character=='T'){
			tom_hit_wall=true;
		}else if(character=='F'){
			firework_wall_collided=true;
		}else if(character=='C'){
			cheese_hit_wall=true;
		}
	}
	//collision for wall 3
	else if(round(character_x_pos) >= wall_3[0] && round(character_x_pos) <= wall_3[2] && round(character_y_pos) >= wall_3[1] && round(character_y_pos) <= wall_3[3]){
		if(character=='J' && !superjerryMode){
			jerry_hit_wall=true;
		}else if(character=='T'){
			tom_hit_wall=true;
		}else if(character=='F'){
			firework_wall_collided=true;
		}else if(character=='C'){
			cheese_hit_wall=true;
		}
	}
	//collision detection for wall 4
	else if(round(character_x_pos) >= wall_4[0] && round(character_x_pos) <= wall_4[2] && round(character_y_pos)==round((wall_4_m * character_x_pos) + wall_4_c)){
		if(character=='J' && !superjerryMode){
			jerry_hit_wall=true;
		}else if(character=='T'){
			tom_hit_wall=true;
		}else if(character=='F'){
			firework_wall_collided=true;
		}else if(character=='C'){
			cheese_hit_wall=true;
		}
	}
	else{
		if(character=='J'){
			jerry_hit_wall=false;
		}else if(character=='T'){
			tom_hit_wall=false;
		}else if(character=='F'){
			firework_wall_collided=false;
		}else if(character=='C'){
			cheese_hit_wall=true;
		}
	}
}
//checks if cheese is inside the walls
void cheese_trap_wall_collision_checker(){
	//cheese collision checks
	for(uint8_t i = 0; i < 5; i++){
		if(cheese_x[cheese_count]<=mousetrap_x[i]+2 && cheese_x[cheese_count] >= mousetrap_x[i] && cheese_y[cheese_count]<=mousetrap_y[i]+1 && cheese_y[cheese_count]>=mousetrap_y[i]){
			mousetrap_x[i]=mousetrap_x[i]+1;
			mousetrap_y[i]=mousetrap_y[i]+1;
		}
	//ensures cheese spawns in playspace
	for(int i=0;i<5;i++){
		if(cheese_y[i]<=11){
			cheese_y[i]=cheese_y[i]+15;
		}
	}
	wall_collision(cheese_x[cheese_count],cheese_y[cheese_count],'C');
	wall_collision(cheese_x[cheese_count]+1,cheese_y[cheese_count]-1,'C');
	wall_collision(cheese_x[cheese_count]+1,cheese_y[cheese_count],'C');
	wall_collision(cheese_x[cheese_count]+1,cheese_y[cheese_count]+1,'C');
	if(cheese_hit_wall==true){
		cheese_x[cheese_count]=rand()%(LCD_X-3);
		cheese_y[cheese_count]=rand()%(LCD_Y-3);
	}
}
}
//setup cheese to spawn in random locations 2 seconds after each other
void setup_cheese(){
	if(get_elapsed_time()-lastCheeseDrawn>=2 && cheese_count < 5){
		lastCheeseDrawn=get_elapsed_time();
		cheese_x[cheese_count]=rand()%(LCD_X-3);
		cheese_y[cheese_count]=rand()%(LCD_Y-3);
		cheese_trap_wall_collision_checker();
		if(cheese_count==5){
			lastCheeseDrawn=get_elapsed_time();
		}
		cheese_count++;
	}
}
//draw the cheese
void draw_cheese(){
	//draw_int(25,25,cheese_time_goal,FG_COLOUR);
	for(uint8_t i = 0; i < cheese_count;i++){
		draw_pixel(cheese_x[i],cheese_y[i],FG_COLOUR);
		draw_pixel(cheese_x[i]+1,cheese_y[i]-1,FG_COLOUR);
		draw_pixel(cheese_x[i]+1,cheese_y[i],FG_COLOUR);
		draw_pixel(cheese_x[i]+1,cheese_y[i]+1,FG_COLOUR);
	}
}
//change to level 2
void change_level(){
	if(levelnumber==2){
		game_over=true;
	}
	setup_usb_connection();
	lastBowlDrawn=get_elapsed_time();
	levelnumber=2;
	setup_door();
	setup_cheese();
	cheese_count=0;
	trap_count=0;
	cheese_consumed=0;
	firework_count=0;
	for(uint8_t i = 0; i < 13;i++){
		jerry_x_coords[i]=original_jerry_x_coords[i];
		jerry_y_coords[i]=original_jerry_y_coords[i];
	}
	for(uint8_t i = 0; i < 6; i++){
		tom_x_coords[i]=original_tom_x_coords[i];
		tom_y_coords[i]=original_tom_y_coords[i];
	}
}
//move jerry with the joysticks
void move_jerry(){
	if(levelnumber==1){
		jerry_speed=1;
	}else{
		jerry_speed = adc_read(0)/1023.00;
	}
	int16_t char_code=usb_serial_getchar();
	
	uint8_t next_jerry_x[13];
	uint8_t next_jerry_y[13];
	static uint8_t prevState = 0;
	//setup character movement using the debounced switches
		//move jerry left
		if ( switch_closed[1] != prevState||char_code=='a') {
			prevState = switch_closed[1];
			for(uint8_t i = 0; i < 13; i++){
				if(round(jerry_x_coords[12]) != 4){
					jerry_x_coords[i] = jerry_x_coords[i] - jerry_speed;
					next_jerry_x[i] = jerry_x_coords[i];
					if(!superjerryMode){
						wall_collision(next_jerry_x[i],jerry_y_coords[i],'J');
						if(jerry_hit_wall==true){
							for(int x = 0; x < 13; x++){
								jerry_x_coords[x] = jerry_x_coords[x] + jerry_speed;
							}
						}
					}
				}
				
			}
		}
		prevState = 0;//reset the prevState value so other switches can be read accurately
		//move jerry right
		if ( switch_closed[2] != prevState||char_code=='d') {
			prevState = switch_closed[2];
			for(uint8_t i = 0; i < 13; i++){
				if(round(jerry_x_coords[12]) != LCD_X-1){
					next_jerry_x[i] = jerry_x_coords[i] + jerry_speed;
					jerry_x_coords[i] = jerry_x_coords[i] + jerry_speed;
					if(!superjerryMode){
						wall_collision(next_jerry_x[i],jerry_y_coords[i],'J');
						if(jerry_hit_wall==true){
							for(int x = 0; x < 13; x++){
								jerry_x_coords[x] = jerry_x_coords[x] - jerry_speed;
							}
						}
					}
				}
			}
		}
		prevState = 0;//reset the prevState value so other switches can be read accurately
		//move jerry up
		if ( switch_closed[3] != prevState||char_code=='w') {
			prevState = switch_closed[3];
			for(uint8_t i = 0; i < 13; i++){
				if(round(jerry_y_coords[12]) != 10){
					next_jerry_y[i] = jerry_y_coords[i] - jerry_speed;
					jerry_y_coords[i] = jerry_y_coords[i] - jerry_speed;
					if(!superjerryMode){
						wall_collision(jerry_x_coords[i],next_jerry_y[i],'J');
						if(jerry_hit_wall==true){
							for(int x = 0; x < 13; x++){
								jerry_y_coords[x] = jerry_y_coords[x] + jerry_speed;
							}
						}
					}
				}
			}
		}
		prevState = 0;//reset the prevState value so other switches can be read accurately
		//move jerry down
		if ( switch_closed[4] != prevState||char_code=='s') {
			prevState = switch_closed[4];
			for(uint8_t i = 0; i < 13; i++){
				if(round(jerry_y_coords[12]) != LCD_Y-3){
					next_jerry_y[i] = jerry_y_coords[i] + jerry_speed;
					jerry_y_coords[i] = jerry_y_coords[i] + jerry_speed;
					if(!superjerryMode){
						wall_collision(jerry_x_coords[i],next_jerry_y[i],'J');
						if(jerry_hit_wall==true){
							for(int x = 0; x < 13; x++){
								jerry_y_coords[x] = jerry_y_coords[x] - jerry_speed;
							}
						}
					}
				}
			}
		}
		prevState = 0;//reset the prevState value so other switches can be read accurately			
}
//move tom in the direction and speed set
void move_tom(){
	//wrap toms movement in the isPaused bool so his movement can be paused
	if(isPaused==false){
		if(levelnumber==2){
			tom_speed=adc_read(0)/1023.00*0.2;
		}
		//convert the angle into values that can be added to toms x and y coordinates
		tom_dx = tom_speed * cos(angle*180/M_PI);
		tom_dy = tom_speed* sin(angle*180/M_PI);
		//create double arrays for predicting tom's next positions
		double next_tom_x[6];
		double next_tom_y[6];
		for(uint8_t i = i; i < 6; i++){
			next_tom_x[i] = tom_x_coords[i] + tom_dx;//predict the next x position
			next_tom_y[i] = tom_y_coords[i] + tom_dy;//predict the next y position
			wall_collision(next_tom_x[i],next_tom_y[i],'T');
			if(tom_hit_wall==true){
				setup_tom();
			}
			//this section checks if tom collides with the outer walls
			//bounce tom off the right hand wall and reset his direction
			if(next_tom_x[i]>=(LCD_X-1)/1){
				setup_tom();
			}
			//bounce tom off the top wall and reset his direction
			if(next_tom_y[i]<=9.00){
				setup_tom();
			}
			//bounce tom off the left hand wall and reset his direction
			if(next_tom_x[i]<=4.00){
				setup_tom();
			}
			//bounce tom off the bottom wall and reset his direction
			if(next_tom_y[i]>=(LCD_Y-3)/1){
				setup_tom();
			}
			//handles when tom gets run over by the walls
			if(levelnumber==2){
				wall_collision(tom_x_coords[i],tom_y_coords[i],'T');
				if(tom_hit_wall==true){
					for(int i = 0; i < 6; i++){
						tom_x_coords[i]=original_tom_x_coords[i];
						tom_y_coords[i]=original_tom_y_coords[i];
					}
				}
			}
		}
		//move tom by his dx and dy values
		for(uint8_t i = 0; i < 6; i++){
			tom_x_coords[i] = tom_x_coords[i] + tom_dx;
			tom_y_coords[i] = tom_y_coords[i] + tom_dy;
		}
	}	
}
//draw the status bar
void draw_game_stats(){
	clear_screen();//clear the previous contents
	draw_string(0,0,"L:",FG_COLOUR);//draw the current level number
	draw_int(7,0,levelnumber,FG_COLOUR);//draw the current level number
	draw_string(17,0,"H:",FG_COLOUR);//draw the remaining lives
	draw_int(27,0,lives,FG_COLOUR);//draw the remaining lives
	draw_string(37,0,"S:",FG_COLOUR);//draw the current score
	draw_int(47,0,score,FG_COLOUR);//draw the current score
	draw_line(0,8,LCD_X,8,FG_COLOUR);//status bar line
}
//format the game timer
void setup_game_timer(){
	//rounds the elapsed time to the nearest second
	int seconds = (int)get_elapsed_time();
	int tens_place_seconds = seconds / 10;
	//resets the seconds timer so it does overflow past 60
	if(seconds > 59){
		if(seconds % 60 == 0){
			minutes_ones = seconds / 60;
		}
	}//configures the clock to overflow to 10 seconds in the seconds 10s place instead of the seconds ones place
	if(seconds % 10 == 0){
		seconds_ones = seconds - tens_place_seconds * 10;
		seconds_tens = tens_place_seconds;
	}else{
		seconds_ones = seconds - tens_place_seconds * 10;
	}//sets the clock up to add minutes to the timer
	if(seconds_tens > 5){
		seconds_tens = seconds_tens - 6;
	}//sets the clock up for minutes greater than 9
	if(minutes_ones > 9){
		minutes_ones = minutes_ones - 10;
		minutes_tens++;
	}
}
//draw the timer in the status bar
void draw_game_timer(){
	draw_int(55,0,minutes_tens,FG_COLOUR);
	draw_int(61,0,minutes_ones,FG_COLOUR);
	draw_string(66,0,":",FG_COLOUR);
	draw_int(70,0,seconds_tens,FG_COLOUR);
	draw_int(76,0,seconds_ones,FG_COLOUR);
}
//restarts the game
void gameOver(){
	if(game_over==true){
		clear_screen();
		draw_string(25,15,"GAME OVER!",FG_COLOUR);
		draw_string(25,35,"Replay?",FG_COLOUR);
		gamestarted = false;
		jerry_x_coords[0]=0;
		jerry_y_coords[0]=9;
		tom_x_coords[0]=LCD_X-5;
		tom_y_coords[0]=LCD_Y-9;
		lives = 5;//set the number of remaining lives
		cycle_count=0;
		setup_game_timer();
		cheese_count=0;
		trap_count=0;
		levelnumber=1;
		superjerryMode=false;
		wall_1[0]=18;
		wall_1[1]=15;
		wall_1[2]=13;
		wall_1[3]=25;
		wall_2[0]=25;
		wall_2[1]=35;
		wall_2[2]=25;
		wall_2[3]=45;
		wall_3[0]=45;
		wall_3[1]=10;
		wall_3[2]=60;
		wall_3[3]=10;
		wall_4[0]=58;
		wall_4[1]=25;
		wall_4[2]=72;
		wall_4[3]=30;
	}
}
//function for collisions between jerry and other game elements
void jerry_collision(){
	//for tom collision
	bool hit = false;
	bool hit_wall = false;
	for(uint8_t i = 0; i < 13; i++){
		for(int x = 0; x < 6; x++){
			if(round(jerry_x_coords[i])==round(tom_x_coords[x]) && round(jerry_y_coords[i])==round(tom_y_coords[x])){
				hit=true;
			}
		}
		//bowl collision
		if(levelnumber==2){
			if((round(jerry_x_coords[i])==round(bowl_x) && round(jerry_y_coords[i])==round(bowl_y))||(round(jerry_x_coords[i])==round(bowl_x+1) && round(jerry_y_coords[i])==round(bowl_y+1))||(round(jerry_x_coords[i])==round(bowl_x+1) && round(jerry_y_coords[i])==round(bowl_y))){
				if(!superjerryMode){
					superjerryMode=true;
					superMode_count=0;
					bowl_Count=0;
				}
				bowl_Count=0;
			}
		}
		//door collision
		if(cheese_consumed>=5){
			if((round(jerry_x_coords[i])==round(door_x) && round(jerry_y_coords[i])==round(door_y))||(round(jerry_x_coords[i])==round(door_x+1) && round(jerry_y_coords[i])==round(door_y))||(round(jerry_x_coords[i])==round(door_x) && round(jerry_y_coords[i]+1)==round(door_y))||(round(jerry_x_coords[i])==round(door_x+1) && round(jerry_y_coords[i])==round(door_y+1))||(round(jerry_x_coords[i])==round(door_x) && round(jerry_y_coords[i])==round(door_y+2))||(round(jerry_x_coords[i])==round(door_x+1) && round(jerry_y_coords[i])==round(door_y+2))){
				if(levelnumber==1){
					change_level();
				}
				if(levelnumber==2){
					game_over=true;
				}
			}
		}
		//cheese collision
		for(int x = 0; x < 5; x++){
			if((round(jerry_x_coords[i])==(cheese_x[x]) && round(jerry_y_coords[i])==(cheese_y[x])) || (round(jerry_x_coords[i])==(cheese_x[x]+1) && round(jerry_y_coords[i])==(cheese_y[x]-1))||(round(jerry_x_coords[i])==(cheese_x[x]+1) && round(jerry_y_coords[i])==(cheese_y[x]))||(round(jerry_x_coords[i])==(cheese_x[x]+1) && round(jerry_y_coords[i])==(cheese_y[x]+1))){
				for(int y=0,z=0; y < cheese_count;y++){
					if(y!=x){
						cheese_x[z]=cheese_x[y];
						cheese_y[z]=cheese_y[y];
						z++;
					}
				} 
				if(cheese_count==5){
					lastCheeseDrawn=get_elapsed_time();
				}
				cheese_count--;
				score++;
				cheese_consumed++;
			}
		}
		//trap collision
		for(int x = 0; x < 5;x++){
			if(!superjerryMode){
				if((round(jerry_x_coords[i])==(mousetrap_x[x]) && round(jerry_y_coords[i])==(mousetrap_y[x])) || (round(jerry_x_coords[i])==(mousetrap_x[x]+1) && round(jerry_y_coords[i])==(mousetrap_y[x]-1))||(round(jerry_x_coords[i])==(mousetrap_x[x]+1) && round(jerry_y_coords[i])==(mousetrap_y[x]))||(round(jerry_x_coords[i])==(mousetrap_x[x]+2) && round(jerry_y_coords[i])==(mousetrap_y[x]))){
					for(int y=0,z=0; y < trap_count;y++){
						if(y!=x){
							mousetrap_x[z]=mousetrap_x[y];
							mousetrap_y[z]=mousetrap_y[y];
							z++;
						}
					} 
					if(trap_count==5){
						lastMouseTrapDrawn=get_elapsed_time();
					}
					trap_count--;
					lives--;
				}
			}
		}
		//for when jerry gets run over by walls in level 2
		if(levelnumber==2 && !superjerryMode){
			if(round(jerry_y_coords[i]) == round(wall_1_m*jerry_x_coords[i]+wall_1_c ) && round(jerry_x_coords[i])==round((jerry_y_coords[i]-wall_1_c)/wall_1_m)){
				if(jerry_x_coords[i] <= wall_1[0] && jerry_x_coords[i] >= wall_1[2] && jerry_y_coords[i] >= wall_1[1] && jerry_y_coords[i] <= wall_1[3]){
					hit_wall=true;
				}
			}
			if(round(jerry_x_coords[i]) ==round(wall_2[0]) && round(jerry_y_coords[i]) >= round(wall_2[1]) && jerry_y_coords[i] <= round(wall_2[3])){
				hit_wall=true;
			}
			if(round(jerry_y_coords[i])==round(wall_3[1]) && round(jerry_x_coords[i]) >= round(wall_3[0]) && round(jerry_x_coords[i]) <= round(wall_3[2])){
				hit_wall=true;	
			}
			if(round(jerry_y_coords[i]) == round(wall_4_m*jerry_x_coords[i]+wall_4_c )&& round(jerry_x_coords[i])==round((jerry_y_coords[i]-wall_4_c)/wall_4_m)){
				if(round(jerry_x_coords[i]) >= round(wall_4[0]) && round(jerry_x_coords[i]) <= round(wall_4[2]) && round(jerry_y_coords[i]) >= round(wall_4[1]) && round(jerry_y_coords[i]) <= round(wall_4[3])){
					hit_wall=true;
				}
			}
		}
	}
	if(hit_wall==true){
		for(uint8_t j = 0; j < 13; j++){
			jerry_x_coords[j]=original_jerry_x_coords[j];
			jerry_y_coords[j]=original_jerry_y_coords[j];
		}
	}
	//jerry collisions in superjerry mode
	if(hit==true){
		if(!superjerryMode){
			for(uint8_t j = 0; j < 13; j++){
				jerry_x_coords[j]=original_jerry_x_coords[j];
				jerry_y_coords[j]=original_jerry_y_coords[j];
			}
			lives--;
		}else{
			score++;
		}
		for(uint8_t i = 0; i < 6;i++){
			tom_x_coords[i] = original_tom_x_coords[i];
			tom_y_coords[i] = original_tom_y_coords[i];
		}
		setup_tom();
		setup_door();
	}
	hit_wall = false;
	hit = false;
	if(lives==0){
		game_over=true;
	}
}
//return jerry to normal size after the superjerry mode has ended
void reset_jerry_size(){
	jerry_x_coords[1]=jerry_x_coords[0];
	jerry_x_coords[2]=jerry_x_coords[0]+1;
	jerry_x_coords[3]=jerry_x_coords[0]+1;
	jerry_x_coords[4]=jerry_x_coords[0]+1;
	jerry_x_coords[5]=jerry_x_coords[0]+2;
	jerry_x_coords[6]=jerry_x_coords[0]+2;
	jerry_x_coords[7]=jerry_x_coords[0]+2;
	jerry_x_coords[8]=jerry_x_coords[0]+3;
	jerry_x_coords[9]=jerry_x_coords[0]+3;
	jerry_x_coords[10]=jerry_x_coords[0]+3;
	jerry_x_coords[11]=jerry_x_coords[0]+4;
	jerry_x_coords[12]=jerry_x_coords[0]+4;
	//setup jerry's pixel y positions
	jerry_y_coords[1]=jerry_y_coords[0]+1;
	jerry_y_coords[2]=jerry_y_coords[0]+1;
	jerry_y_coords[3]=jerry_y_coords[0]+2;
	jerry_y_coords[4]=jerry_y_coords[0]+3;
	jerry_y_coords[5]=jerry_y_coords[0]+1;
	jerry_y_coords[6]=jerry_y_coords[0]+2;
	jerry_y_coords[7]=jerry_y_coords[0]+3;
	jerry_y_coords[8]=jerry_y_coords[0]+1;
	jerry_y_coords[9]=jerry_y_coords[0]+2;
	jerry_y_coords[10]=jerry_y_coords[0]+3;
	jerry_y_coords[11]=jerry_y_coords[0];
	jerry_y_coords[12]=jerry_y_coords[0]+1;
	CLEAR_BIT(PORTB, 2);
	CLEAR_BIT(PORTB, 3);
}
//sets up superjerry mode
void superJerry(){
	if(superjerryMode==true){
		jerry_x_coords[0]=jerry_x_coords[0];
		jerry_y_coords[0]=jerry_y_coords[0];
		jerry_x_coords[1]=jerry_x_coords[0];
		jerry_y_coords[1]=jerry_y_coords[0]+1;
		jerry_x_coords[2]=jerry_x_coords[0]+1;
		jerry_y_coords[2]=jerry_y_coords[0]+1;
		jerry_x_coords[3]=jerry_x_coords[0]+2;
		jerry_y_coords[3]=jerry_y_coords[0]+1;
		jerry_x_coords[4]=jerry_x_coords[0]+3;
		jerry_y_coords[4]=jerry_y_coords[0]+1;
		jerry_x_coords[5]=jerry_x_coords[0]+4;
		jerry_y_coords[5]=jerry_y_coords[0]+1;
		jerry_x_coords[6]=jerry_x_coords[0]+5;
		jerry_y_coords[6]=jerry_y_coords[0]+1;
		jerry_x_coords[7]=jerry_x_coords[0]+6;
		jerry_y_coords[7]=jerry_y_coords[0];
		jerry_x_coords[8]=jerry_x_coords[0]+1;
		jerry_y_coords[8]=jerry_y_coords[0]+2;
		jerry_x_coords[9]=jerry_x_coords[0]+2;
		jerry_y_coords[9]=jerry_y_coords[0]+3;
		jerry_x_coords[10]=jerry_x_coords[0]+3;
		jerry_y_coords[10]=jerry_y_coords[0]+3;
		jerry_x_coords[11]=jerry_x_coords[0]+4;
		jerry_y_coords[11]=jerry_y_coords[0]+2;
		jerry_x_coords[12]=jerry_x_coords[0]+2;
		jerry_y_coords[12]=jerry_y_coords[0]+4;
		SET_BIT(PORTB, 2);
        SET_BIT(PORTB, 3);
		if(get_elapsed_supermode_time()-disabling_time>=10){
			disabling_time=get_elapsed_supermode_time();
			superjerryMode=false;
			reset_jerry_size();
		}
	}
}
//draw the walls in the defined positions of the assignment spec sheet
void draw_walls(){
	//draw the walls
	draw_line(wall_1[0],wall_1[1],wall_1[2],wall_1[3],FG_COLOUR);
	draw_line(wall_2[0],wall_2[1],wall_2[2],wall_2[3],FG_COLOUR);
	draw_line(wall_3[0],wall_3[1],wall_3[2],wall_3[3],FG_COLOUR);
	draw_line(wall_4[0],wall_4[1],wall_4[2],wall_4[3],FG_COLOUR);
	wall_1_m = (wall_1[3]-wall_1[1])/(wall_1[2]-wall_1[0]);
	wall_1_c = wall_1[1]-wall_1_m * wall_1[0];
	wall_4_m = (wall_4[1]-wall_4[3])/(wall_4[0]-wall_4[2]);
	wall_4_c = wall_4[1]-wall_4_m * wall_4[0];
}
//move the walls perpendicular
void move_walls(){
	if(isPaused==false){
		double wall_speed = (0.2*2/1023)*(adc_read(1))-0.2;
		//calculate the angle of movement for wall 1 and then move it by that angle
		double wall_1_movement_angle = asin((sqrt((wall_1[0]-wall_1[2])*(wall_1[0]-wall_1[2]) + (wall_1[1]-wall_1[3])))/(wall_1[3]-wall_1[1]));
		wall_1[0] = wall_1[0] + cos(wall_1_movement_angle*180/M_PI)*-wall_speed;
		wall_1[2] = wall_1[2] + cos(wall_1_movement_angle*180/M_PI)*-wall_speed;
		wall_1[1] = wall_1[1] + sin(wall_1_movement_angle*180/M_PI)*-wall_speed;
		wall_1[3] = wall_1[3] + sin(wall_1_movement_angle*180/M_PI)*-wall_speed;
		//wrap wall 1 when it moves around
		if(wall_1[3]>LCD_Y){
			wall_1[0] = 14;
			wall_1[1] = 11;
			wall_1[2] = 9;
			wall_1[3] = 21;
		}
		if(wall_1[1]<=10){
			wall_1[0]=40;
			wall_1[1]=38;
			wall_1[2]=35;
			wall_1[3]=48;
		}
		//calculate the angle of movement for wall 4 and then move it by that angle
		double wall_4_movement_angle = asin((wall_4[3]-wall_4[1])/(sqrt((wall_4[0]-wall_4[2])*(wall_4[0]-wall_4[2]) + (wall_4[1]-wall_4[3]))));
		wall_4[0] = wall_4[0] + (cos(wall_4_movement_angle*180/M_PI)*wall_speed);
		wall_4[2] = wall_4[2] + (cos(wall_4_movement_angle*180/M_PI)*wall_speed);
		wall_4[1] = wall_4[1] + (sin(wall_4_movement_angle*180/M_PI)*wall_speed);
		wall_4[3] = wall_4[3] + (sin(wall_4_movement_angle*180/M_PI)*wall_speed);
		if(wall_4[3]>=LCD_Y){
			wall_4[0]=67;
			wall_4[1]=14;
			wall_4[2]=81;
			wall_4[3]=19;	
		}
		if(wall_4[2]>=LCD_X){
			wall_4[0]=39;
			wall_4[1]=43;
			wall_4[2]=53;
			wall_4[3]=48;
		}
		//move wall 2 right
		wall_2[0] = wall_2[0] - wall_speed;
		wall_2[2] = wall_2[2] - wall_speed;
		if(wall_2[0]>LCD_X){
			wall_2[0] = 0;
			wall_2[2] = 0;
		}else if(wall_2[0]<0){
			wall_2[0]=LCD_X;
			wall_2[2]=LCD_X;
		}
		//move wall 3 down
		wall_3[1] = wall_3[1] + wall_speed;
		wall_3[3] = wall_3[3] + wall_speed;
		if(wall_3[1]>LCD_Y){
			wall_3[1] = 10;
			wall_3[3] = 10;
		}else if(wall_3[1]<9){
			wall_3[1] = LCD_Y;
			wall_3[3] = LCD_Y;
		}
	}
}
//draw tom and jerry on screen
void draw_characters(){
	//for loops are used because it is arrays of pixels that are used to draw the characters
	//jerry
	for (uint8_t i =0; i < 13; i++){
		draw_pixel(jerry_x_coords[i],jerry_y_coords[i],FG_COLOUR);
	}
	//tom
	for(uint8_t i = 0; i < 6; i++){
		draw_pixel(tom_x_coords[i],tom_y_coords[i],FG_COLOUR);
	}
}
//check if the game has been paused or not
void checkPause(){
	//this checks to see if the right button has been pressed so the gameStarted bool can be set to true
	//also sets up the pause functionality
	//uses the debounced right switch or keyboard input if level 2
	int16_t char_code=usb_serial_getchar();
	static uint8_t prevState = 0;
	if(switch_closed[6] != prevState||char_code=='p') {
		_delay_ms(100);
		prevState = switch_closed[6];
		gamestarted = true;//starts the game
		game_over=false;
		//pauses and unpauses the game
		if(isPaused == true){
			isPaused = false;
		}else{
			isPaused = true;
		}
		//setup tom for randomised movement
		setup_tom();
	}
	prevState = 0;//resets the prevState value so it can be rechecked
}
//change the level on left button press
void changeLevelButton(){
	//this checks to see if the right button has been pressed so the gameStarted bool can be set to true
	//it can also use the keyboard as well
	//also sets up the pause functionality
	//uses the debounced right switch
	int16_t char_code=usb_serial_getchar();
	static uint8_t prevState = 0;
	if ( switch_closed[5] != prevState||char_code=='l') {
		prevState = switch_closed[5];
		if(levelnumber==1){
			change_level();
		}else if(levelnumber==2){
			gameOver();
		}
	}
	prevState = 0;//resets the prevState value so it can be rechecked
}
//setup firework positions
void setup_fireworks(){
	if(firework_count < 20){
		for(int i = 0; i < 20; i++){
			x_distance[i] = tom_x_coords[3] - fireworks_x[i];
			y_distance[i] = tom_y_coords[3] - fireworks_y[i];
			line_distance[i] = sqrt(x_distance[i] * x_distance[i] + y_distance[i] * y_distance[i]);
			fireworks_dx[i] = x_distance[i] * 0.25 / line_distance[i];
			fireworks_dy[i] = y_distance[i] * 0.25 / line_distance[i];
		}
	}
}
//move the fireworks to track tom
void move_firework(){
	if(fired==true){
		setup_fireworks();
		for(int i = 0; i < firework_count;i++){
			fireworks_x[i]+=fireworks_dx[i];
			fireworks_y[i]+=fireworks_dy[i];
			draw_pixel(fireworks_x[i],fireworks_y[i],FG_COLOUR);
		}
		for(uint8_t i = 0; i < 6;i++){
			for(int x = 0; x < 20; x++){
				if((round(tom_x_coords[i])==round(fireworks_x[x]) && round(tom_y_coords[i])==round(fireworks_y[x]))){
					for(int y=0,z=0; y < firework_count;y++){
						if(y!=x){
							fireworks_x[z]=fireworks_x[y];
							fireworks_y[z]=fireworks_y[y];
							z++;
							firework_count--;
						}
					}
					score++;
					for(int a = 0; a < 6; a++){
						tom_x_coords[a]=original_tom_x_coords[a];
						tom_y_coords[a]=original_tom_y_coords[a];
					}
				}
				wall_collision(fireworks_x[x],fireworks_y[x],'F');
				if(firework_wall_collided==true){
					for(int y=0,z=0; y < firework_count;y++){
						if(y!=x){
							fireworks_x[z]=fireworks_x[y];
							fireworks_y[z]=fireworks_y[y];
							z++;
							firework_count--;
						}
					}
				}
			}
		}
	}
}
//fire the fireworks
void fire_firework(){
	int16_t char_code=usb_serial_getchar();
	static uint8_t prevState = 0;
	int hasShot = 0;
	if ( switch_closed[0] != prevState||char_code=='f') {
		if(cheese_consumed>=3){
			prevState = switch_closed[0];
			fired = true;
			setup_fireworks();
			hasShot = 1;
			if(hasShot==1){
				fireworks_x[firework_count]=jerry_x_coords[3];
				fireworks_y[firework_count]=jerry_y_coords[3];
				firework_count++;
				hasShot=0;
			}
		}
	}
	prevState = 0;
}
void start_timers(){
	//initialise Timer 3 in normal mode so it overflows with a period of approximately 0.07 seconds
	TCCR3A = 0;
	TCCR3B = 2;
	TIMSK3 = 1;
}
//these two functions call the functions in a loop
void process(void) {
	//this checks to see if the right button has been pressed so the gameStarted bool can be set to true
	//also sets up the pause functionality
	checkPause();
	//if gameStarted is true we remove the splash screen and setup the game screen
	if(gamestarted == true){
		start_timers();
		//draw the top bar of the game screen
		draw_game_stats();
		//setup the game timer
		setup_game_timer();
		//draw the formatted clock
		draw_game_timer();		
		//draw the walls
		draw_walls();
		//move the walls
		move_walls();
		//send information to putty
		send_game_state();
		//set the cheese up
		setup_cheese();
		//draw the cheese
		draw_cheese();
		//setup the traps
		setup_mousetrap();
		//draw the mousetrap
		draw_trap();
		//draw the characters
		draw_characters();
		//setup the potion bowl
		setup_bowl();
		//draw the potion bowl
		draw_bowl();
		//move tom by calling the move_tom function
		move_tom();
		//move jerry by calling the move_jerry function
		move_jerry();
		//run superjerry mode through this function(checks if active first)
		superJerry();
		//fire the firework
		fire_firework();
		//move the firework
		move_firework();
		//checks if jerry has collided with in game elements
		jerry_collision();
		//draw the door
		draw_door();
		//checks if the game is paused or not
		checkPause();
		//change levels by calling this function(it runs a button check)
		changeLevelButton();
		gameOver();
		//shows the game screen
		show_screen();
	}
}
int main(void) {
	//runs the code in a loop
	setup();
	for ( ;; ) {
		process();
	}
}