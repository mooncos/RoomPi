/*
 * systemlib.h
 *
 *  Created on: 14 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#ifndef SYSTEMLIB_H_
#define SYSTEMLIB_H_

#include <time.h>

#include "../sensors/dht11.h"
#include "../sensors/bh1750.h"
#include "../sensors/ccs811.h"

#include "../actuators/lcd1602.h"
#include "../actuators/buzzer.h"
#include "../actuators/statusLed.h"

#include "../libs/timerlib.h"
#include "../libs/circularbuffer.h"

// Mutexes
#define MEASUREMENT_LOCK 0
#define OUTPUT_LOCK 1
#define STORAGE_LOCK 2

typedef struct {
	enum {
		is_int, is_float, is_error
	} type;
	union {
		int ival;
		float fval;
	} val;
} SensorValueType; // This is a new type defined because we have sensors that give float value and int values depending on the sensor

typedef struct {
	int id_classroom; // id/number of the classroom the system is in (corridor, building, location...)

	// Sensors attached
	DHT11Sensor *sensor_temp_humid;
	BH1750Sensor *sensor_light;
	CCS811Sensor *sensor_co2;

	// Actuators attached
	LCD1602Display *actuator_display;
	BuzzerOutput *actuator_buzzer;
	StatusLEDOutput *actuator_leds;

	// Sensor values storage
	CircularBuffer sensor_storage[4]; // At the moment, four circular/ring buffers representing Temp, Humid, Light, CO2
	SensorValueType sensor_values[4]; // Final processed values representing Temp, Humid, Light, CO2
} SystemContext;

SystemContext* SystemContext__create(int id_classroom, DHT11Sensor *sensor_temp_humid, BH1750Sensor *sensor_light, CCS811Sensor *sensor_co2, LCD1602Display *actuator_display,
		BuzzerOutput *actuator_buzzer, StatusLEDOutput *actuator_leds);

void SystemContext__destroy(SystemContext *this);

#endif /* SYSTEMLIB_H_ */
