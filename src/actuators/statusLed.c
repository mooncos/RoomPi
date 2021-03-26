/*
 * statusLed.c
 *
 *  Created on: 10 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#include <wiringPi.h>
#include <wiringShift.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

/***************/

#include "statusLed.h"
#include "../utils.h"

StatusLEDOutput* StatusLEDOutput__create(int id, int series_ic_nr,
		int clock_pin, int serial_data_pin, int latch_pin,
		int led_color_flags[3]) {
	StatusLEDOutput *result = (StatusLEDOutput*) malloc(
			sizeof(StatusLEDOutput) + series_ic_nr * sizeof(int));
	result->id = id;
	result->series_ic_nr = series_ic_nr;
	result->clock_pin = clock_pin;
	result->serial_data_pin = serial_data_pin;
	result->latch_pin = latch_pin;
	result->current_color = GREEN;
	memcpy(result->led_color_flags, led_color_flags, 3 * sizeof(int));

	pinMode(clock_pin, OUTPUT);
	pinMode(serial_data_pin, OUTPUT);
	pinMode(latch_pin, OUTPUT);

	digitalWrite(clock_pin, LOW);
	digitalWrite(serial_data_pin, LOW);
	digitalWrite(latch_pin, LOW);

	memset(result->digital_values, 0, result->series_ic_nr * sizeof(int));

	StatusLEDOutput__update_registers(result);

	return result;
}

void StatusLEDOutput__destroy(StatusLEDOutput *leds) {
	if (leds) {
		StatusLEDOutput__set_all_low(leds);
		free(leds->digital_values);
		free(leds);
	}
}

void StatusLEDOutput__set_all(StatusLEDOutput *leds, int *digital_values) {
	memcpy(leds->digital_values, digital_values, leds->series_ic_nr);
	StatusLEDOutput__update_registers(leds);
}

int* StatusLEDOutput__get_all(StatusLEDOutput *leds) {
	return leds->digital_values;
}

void StatusLEDOutput__set(StatusLEDOutput *leds, int pin, int value) {
	StatusLEDOutput__set_no_update(leds, pin, value);
	StatusLEDOutput__update_registers(leds);
}

void StatusLEDOutput__update_registers(StatusLEDOutput *leds) {
	for (int i = leds->series_ic_nr - 1; i >= 0; i--) {
		shiftOut(leds->serial_data_pin, leds->clock_pin, MSBFIRST,
				leds->digital_values[i]);
	}

	digitalWrite(leds->latch_pin, HIGH);
	digitalWrite(leds->latch_pin, LOW);
}

void StatusLEDOutput__set_no_update(StatusLEDOutput *leds, int pin, int value) {
	(value) ?
			bitSet(leds->digital_values[pin / 8], pin % 8) :
			bitClear(leds->digital_values[pin / 8], pin % 8);
}

void StatusLEDOutput__set_all_low(StatusLEDOutput *leds) {
	for (int i = 0; i < leds->series_ic_nr; i++) {
		leds->digital_values[i] = 0;
	}
	StatusLEDOutput__update_registers(leds);
}

void StatusLEDOutput__set_all_high(StatusLEDOutput *leds) {
	for (int i = 0; i < leds->series_ic_nr; i++) {
		leds->digital_values[i] = 255;
	}
	StatusLEDOutput__update_registers(leds);
}

int StatusLEDOutput__get(StatusLEDOutput *leds, int pin) {
	return (leds->digital_values[pin / 8] >> (pin % 8)) & 0x01;
}

// high level functions for the user

void StatusLEDOutput__set_color(StatusLEDOutput *leds, StatusLEDColor color) {
	StatusLEDOutput__set_all_low(leds);
	int green_arr[1], yellow_arr[1], red_arr[1];
	switch (color) {
	case GREEN:
		green_arr[0] = leds->led_color_flags[GREEN];
		StatusLEDOutput__set_all(leds, green_arr);
		break;
	case YELLOW:
		yellow_arr[0] = leds->led_color_flags[YELLOW];
		StatusLEDOutput__set_all(leds, yellow_arr);
		break;
	case RED:
		red_arr[0] = leds->led_color_flags[RED];
		StatusLEDOutput__set_all(leds, red_arr);
		break;
	default:
		StatusLEDOutput__set_color_error(leds);
		break;
	}
}

void StatusLEDOutput__set_color_error(StatusLEDOutput *leds) {
	StatusLEDOutput__set_all_low(leds);
	int arr[1] = { 0b10101010 };
	StatusLEDOutput__set_all(leds, arr);
}

