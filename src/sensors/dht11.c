/*
 * dht11.c
 *
 *  Created on: 27 feb. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

/************************/

#include "dht11.h"
#include "../utils.h"
#include "../libs/systemlib.h"
#include "../libs/systemtype.h"

// FSM Functions and variables

extern int measurement_flags; // grab global light flags

// Timer
static void _temp_humid_timer_isr(union sigval value);

// FSM states enum
enum _temp_humid_fsm_state { TEMP_HUMID_MEASUREMENT };

// FSM input check functions
static int _temp_humid_pending_measurement(fsm_t *this);

// FSM output action functions
static void _temp_humid_do_measurement(fsm_t *this);

// { EstadoOrigen, CondicionDeDisparo, EstadoFinal, AccionesSiTransicion }
static fsm_trans_t _temp_humid_fsm_tt[] = {
		{ TEMP_HUMID_MEASUREMENT, _temp_humid_pending_measurement, TEMP_HUMID_MEASUREMENT, _temp_humid_do_measurement },
		{-1, NULL, -1, NULL}
};

/************************/


DHT11Sensor* DHT11Sensor__create(int id, int data_pin) {
	DHT11Sensor *result = (DHT11Sensor*) malloc(sizeof(DHT11Sensor));
	result->id = id;
	result->data_pin = data_pin;
	result->t_value = 0;
	result->rh_value = 0;
	result->timestamp = 0;

	// Timer instantiation
		tmr_t *temp_humid_timer = tmr_new(_temp_humid_timer_isr); // creado pero no iniciado
		result->timer = temp_humid_timer;

		// FSM creation
		result->fsm = (fsm_t *) fsm_new(TEMP_HUMID_MEASUREMENT, _temp_humid_fsm_tt, result); //3rd param pointer available under user_data


	return result;
}

void DHT11Sensor__destroy(DHT11Sensor *sensor_instance) {
	if (sensor_instance) {
		fsm_destroy(sensor_instance->fsm);
		tmr_destroy(sensor_instance->timer);
		// reset....
		free(sensor_instance);
	};
}

float DHT11Sensor__t_value(DHT11Sensor *sensor_instance) {
	return sensor_instance->t_value;
}

float DHT11Sensor__rh_value(DHT11Sensor *sensor_instance) {
	return sensor_instance->rh_value;
}

int DHT11Sensor__perform_measurement(DHT11Sensor *sensor_instance) {
	// test data
	//sensor_instance->t_value = 25.0;
	//sensor_instance->rh_value = 17.0;

	const int DHT_PIN = sensor_instance->data_pin;
	const int MAX_TIMINGS = 85;

	int data[5] = { 0, 0, 0, 0, 0 };

	int laststate = HIGH;
	int counter = 0;
	int j = 0, i;

	data[0] = data[1] = data[2] = data[3] = data[4] = 0;

	/* pull pin down for 18 milliseconds */
	pinMode(DHT_PIN, OUTPUT);
	digitalWrite(DHT_PIN, LOW);
	delay(18);

	/* prepare to read the pin */
	pinMode(DHT_PIN, INPUT);

	/* detect change and read data */
	for (i = 0; i < MAX_TIMINGS; i++) {
		counter = 0;
		while (digitalRead(DHT_PIN) == laststate) {
			counter++;
			delayMicroseconds(1);
			if (counter == 255) {
				break;
			}
		}
		laststate = digitalRead(DHT_PIN);

		if (counter == 255) {
			break;
		}

		/* ignore first 3 transitions */
		if ((i >= 4) && (i % 2 == 0)) {
			/* shove each bit into the storage bytes */
			data[j / 8] <<= 1;
			if (counter > 50) {
				data[j / 8] |= 1;
			}
			j++;
		}
	}

	/*
	 * check we read 40 bits (8bit x 5 ) + verify checksum in the last byte
	 * print it out if data is good
	 */
	if ((j >= 40) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) {
		float h = (float) ((data[0] << 8) + data[1]) / 10;
		if (h > 100) {
			h = data[0];	// for DHT11
		}
		float c = (float) (((data[2] & 0x7F) << 8) + data[3]) / 10;
		if (c > 125) {
			c = data[2];	// for DHT11
		}
		if (data[2] & 0x80) {
			c = -c;
		}

		//printf( "Humidity = %.1f %% Temperature = %.1f *C (%.1f *F)\n", h, c, f );
		sensor_instance->rh_value = h;
		sensor_instance->t_value = c;

		sensor_instance->timestamp = millis();

		return 0;

	} else {
		//printf("%d %d %d %d %d\n", data[0], data[1], data[2], data[3], data[4]);
		sensor_instance->timestamp = millis();
		return 2;
	}

	return 1;
}

/************************/

static void _temp_humid_timer_isr(union sigval value) {
	piLock(MEASUREMENT_LOCK);
	measurement_flags |= FLAG_TEMP_HUMID_PENDING_MEASUREMENT;
	piUnlock(MEASUREMENT_LOCK);
}

static int _temp_humid_pending_measurement(fsm_t *this) {
	return (measurement_flags & FLAG_LIGHT_PENDING_MEASUREMENT);
}

static void _temp_humid_do_measurement(fsm_t *this) {
	DHT11Sensor* dht = (DHT11Sensor*) this->user_data;
	int r = DHT11Sensor__perform_measurement(dht);

	SensorValueType res_temp_val; // craft SensorValueType instance with type Integer and value measured temp or error
	SensorValueType res_humid_val; // craft SensorValueType instance with type Integer and value measured humid or error

	if (r > 0) {
		// we have an error
		res_temp_val.type = is_error;
		res_temp_val.val.ival = 0;

		res_humid_val.type = is_error;
		res_humid_val.val.ival = 0;

	} else {
		piLock(MEASUREMENT_LOCK);
		measurement_flags &= ~(FLAG_TEMP_HUMID_PENDING_MEASUREMENT);
		piUnlock(MEASUREMENT_LOCK);

		res_temp_val.type = is_float;
		res_temp_val.val.fval = DHT11Sensor__t_value(dht);

		res_humid_val.type = is_float;
		res_humid_val.val.fval = DHT11Sensor__rh_value(dht);
	}

	extern SystemType *roompi_system; // get the current system
	piLock(STORAGE_LOCK);
	CircularBufferPush(roompi_system->root_system->sensor_storage[0], (SensorValueType*) &res_temp_val, sizeof(res_temp_val));
	CircularBufferPush(roompi_system->root_system->sensor_storage[1], (SensorValueType*) &res_humid_val, sizeof(res_humid_val));
	piUnlock(STORAGE_LOCK);

}
