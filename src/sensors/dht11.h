/*
 * dht11.h
 *
 *  Created on: 27 feb. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#ifndef DHT11_H_
#define DHT11_H_

#include "../libs/fsm.h"
#include "../libs/timerlib.h"
#include "../libs/circularbuffer.h"

#define FLAG_TEMP_HUMID_PENDING_MEASUREMENT 0x01

typedef struct {
	int id; // sensor id
	float t_value; // temperature value
	float rh_value; // relative humidity value
	int data_pin; // wPi pin the sensor is connected to
	unsigned int timestamp; // last measurement timestamp

	fsm_t *fsm; // FSM that performs a measurement from the temp humid sensor
	tmr_t *timer; // timer that goberns a flag used by the temp humid sensor measurement FSM (5 s periodic)
} DHT11Sensor;

DHT11Sensor* DHT11Sensor__create(int id, int data_pin);
void DHT11Sensor__destroy(DHT11Sensor *sensor_instance);
float DHT11Sensor__t_value(DHT11Sensor *sensor_instance);
float DHT11Sensor__rh_value(DHT11Sensor *sensor_instance);
int DHT11Sensor__perform_measurement(DHT11Sensor *sensor_instance);

#endif /* DHT11_H_ */
