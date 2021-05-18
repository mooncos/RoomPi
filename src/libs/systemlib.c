/*
 * systemlib.c
 *
 *  Created on: 14 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#include <stdlib.h>

#include "systemlib.h"

SystemContext* SystemContext__create(int id_classroom,
		DHT11Sensor *sensor_temp_humid, BH1750Sensor *sensor_light, CCS811Sensor *sensor_co2,
		LCD1602Display *actuator_display, BuzzerOutput *actuator_buzzer,
		StatusLEDOutput *actuator_leds) {
	SystemContext *result = (SystemContext*) malloc(sizeof(SystemContext));
	result->id_classroom = id_classroom;
	result->sensor_temp_humid = sensor_temp_humid;
	result->sensor_light = sensor_light;
	result->sensor_co2 = sensor_co2;
	result->actuator_display = actuator_display;
	result->actuator_buzzer = actuator_buzzer;
	result->actuator_leds = actuator_leds;

	// Create circular buffers
	for (int i = 0; i < sizeof(result->sensor_storage) / sizeof(CircularBuffer); i++) {
		result->sensor_storage[i] = CircularBufferCreate(5 * sizeof(SensorValueType));
	}

	for (int i = 0; i < sizeof(result->sensor_values) / sizeof(SensorValueType); i++) {
		SensorValueType aux = {.type = is_error };
		result->sensor_values[i] = aux;
	}

	return result;
}

void SystemContext__destroy(SystemContext *this) {
	if (this) {
		DHT11Sensor__destroy(this->sensor_temp_humid);
		BH1750Sensor__destroy(this->sensor_light);
		CCS811Sensor__destroy(this->sensor_co2);
		LCD1602Display__destroy(this->actuator_display);
		BuzzerOutput__destroy(this->actuator_buzzer);
		StatusLEDOutput__destroy(this->actuator_leds);

		free(this);
	}
}
