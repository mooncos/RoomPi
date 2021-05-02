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
} SystemContext;

SystemContext* SystemContext__create(int id_classroom,
		DHT11Sensor *sensor_temp_humid, BH1750Sensor *sensor_light, CCS811Sensor *sensor_co2,
		LCD1602Display *actuator_display, BuzzerOutput *actuator_buzzer,
		StatusLEDOutput *actuator_leds);

void SystemContext__destroy(SystemContext *this);

#endif /* SYSTEMLIB_H_ */
