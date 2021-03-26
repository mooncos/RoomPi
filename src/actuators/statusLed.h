/*
 * StatusLED.h
 *
 *  Created on: 10 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#ifndef STATUSLED_H_
#define STATUSLED_H_

typedef enum {
	GREEN = 0, YELLOW = 1, RED = 2
} StatusLEDColor;

typedef struct {
	int id; // entero
	int series_ic_nr; // number of ICs registers connected in series
	int clock_pin;
	int serial_data_pin;
	int latch_pin;
	StatusLEDColor current_color;
	int led_color_flags[3]; // 0b1100000, 0b00111000, 0b00000111 (GREEN, YELLOW, RED LEDS) MSB FIRST (leftmost)
	int digital_values[]; // pin values 0b10010000 pin Q0 is 1, pin Q1 is 0.......
} StatusLEDOutput;

StatusLEDOutput* StatusLEDOutput__create(int id, int series_ic_nr, int clock_pin, int latch_pin,
		int serial_data_pin, int led_color_flags[3]);
void StatusLEDOutput__destroy(StatusLEDOutput *leds);

void StatusLEDOutput__set_all(StatusLEDOutput *leds, int *digital_values);
int* StatusLEDOutput__get_all(StatusLEDOutput *leds);
void StatusLEDOutput__set(StatusLEDOutput *leds, int pin, int value);
void StatusLEDOutput__set_no_update(StatusLEDOutput *leds, int pin, int value);
void StatusLEDOutput__update_registers(StatusLEDOutput *leds);
void StatusLEDOutput__set_all_low(StatusLEDOutput *leds);
void StatusLEDOutput__set_all_high(StatusLEDOutput *leds);
int StatusLEDOutput__get(StatusLEDOutput *leds, int pin);

// high level functions
void StatusLEDOutput__set_color(StatusLEDOutput *leds, StatusLEDColor color);
void StatusLEDOutput__set_color_error(StatusLEDOutput *leds);

#endif /* STATUSLED_H_ */
