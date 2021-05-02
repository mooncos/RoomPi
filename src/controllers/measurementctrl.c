/*
 * measurementctrl.c
 *
 *  Created on: 14 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#include <stdlib.h>
#include <wiringPi.h>

#include "measurementctrl.h"
#include "../libs/threadlib.h"
#include "../libs/timerlib.h"
#include "../libs/systemlib.h"
#include "../libs/fsm.h"

extern int measurement_flags;

// Timer
static void _measurement_timer_isr(union sigval value);

// FSM states enum
enum _fsm_state { NO_MEASUREMENT, TEMP_HUMID_MEASUREMENT, LIGHT_MEASUREMENT, CO2_MEASUREMENT };

// FSM input check functions
static int _temp_humid_pending_measurement(fsm_t *this);
//static int _temp_humid_not_pending_measurement(fsm_t *this) {
//	return !(_temp_humid_pending_measurement(this));
//}

static int _light_pending_measurement(fsm_t *this);
static int _light_not_pending_measurement(fsm_t *this) {
	return !(_light_pending_measurement(this));
}

static int _co2_pending_measurement(fsm_t *this);
static int _co2_not_pending_measurement(fsm_t *this) {
	return !(_co2_pending_measurement(this));
}

static int _light_and_co2_not_pending_measurement(fsm_t *this){
	return (_light_not_pending_measurement(this) && _co2_not_pending_measurement(this));
}

static int _always(fsm_t* this) {
	return 1;
}

// FSM output action functions
static void _temp_humid_do_measurement(fsm_t *this);
static void _light_do_measurement(fsm_t *this);
static void _co2_do_measurement(fsm_t *this);

// { EstadoOrigen, CondicionDeDisparo, EstadoFinal, AccionesSiTransicion }
static fsm_trans_t _measurement_fsm_tt[] = {
		{ NO_MEASUREMENT, _temp_humid_pending_measurement, TEMP_HUMID_MEASUREMENT, _temp_humid_do_measurement },
		{ NO_MEASUREMENT, _light_pending_measurement, LIGHT_MEASUREMENT, _light_do_measurement },
		{ NO_MEASUREMENT, _co2_pending_measurement, CO2_MEASUREMENT, _co2_do_measurement },
		{ TEMP_HUMID_MEASUREMENT, _light_pending_measurement, LIGHT_MEASUREMENT, _light_do_measurement },
		{ TEMP_HUMID_MEASUREMENT, _co2_pending_measurement, CO2_MEASUREMENT, _co2_do_measurement },
		{ TEMP_HUMID_MEASUREMENT, _light_and_co2_not_pending_measurement, NO_MEASUREMENT, NULL },
		{ LIGHT_MEASUREMENT, _co2_pending_measurement, CO2_MEASUREMENT, _co2_do_measurement },
		{ LIGHT_MEASUREMENT, _co2_not_pending_measurement, NO_MEASUREMENT, NULL },
		{ CO2_MEASUREMENT, _always, NO_MEASUREMENT, NULL },
		{-1, NULL, -1, NULL}
};

MeasurementCtrl* MeasurementCtrl__setup(SystemContext* this_system) {
	MeasurementCtrl *result = (MeasurementCtrl*) malloc(sizeof(MeasurementCtrl));
	tmr_t *measurement_timer = tmr_new(_measurement_timer_isr); // creado pero no iniciado
	result->timer = measurement_timer;

	result->fsm = (fsm_t *) fsm_new(NO_MEASUREMENT, _measurement_fsm_tt, this_system);
	return result;
}

void MeasurementCtrl__destroy(MeasurementCtrl * this) {
	if (this)  {
		fsm_destroy(this->fsm);
		tmr_destroy(this->timer);

		free(this);
	}
}

/* Definition of the functions */

static void _measurement_timer_isr(union sigval value) {
	piLock(MEASUREMENT_LOCK);
	measurement_flags |= (FLAG_TEMP_HUMID_PENDING_MEASUREMENT | FLAG_LIGHT_PENDING_MEASUREMENT | FLAG_CO2_PENDING_MEASUREMENT);
	piUnlock(MEASUREMENT_LOCK);
}

static int _temp_humid_pending_measurement(fsm_t *this) {
	unsigned int last_measurement_time = (((SystemContext*) this->user_data)->sensor_temp_humid)->timestamp;
	unsigned int now = millis();
	char has_1s_elapsed = (now - last_measurement_time) >= 1000 ? 1 : 0; // 1 second needs to pass between resampling
	return ((has_1s_elapsed) && (measurement_flags & FLAG_TEMP_HUMID_PENDING_MEASUREMENT));
}

static int _light_pending_measurement(fsm_t *this) {
	return (measurement_flags & FLAG_LIGHT_PENDING_MEASUREMENT);
}

static int _co2_pending_measurement(fsm_t *this) {
	return (measurement_flags & FLAG_CO2_PENDING_MEASUREMENT);
}


static void _temp_humid_do_measurement(fsm_t *this) {
	DHT11Sensor* dht = ((SystemContext*) this->user_data)->sensor_temp_humid;
	int err = DHT11Sensor__perform_measurement(dht);
	if (err == 0) {
		piLock(MEASUREMENT_LOCK);
		measurement_flags &= ~(FLAG_TEMP_HUMID_PENDING_MEASUREMENT);
		measurement_flags &= ~(FLAG_TEMP_ANOMALY | FLAG_TEMP_EMERGENCY | FLAG_HUMID_ANOMALY | FLAG_HUMID_EMERGENCY); // assume there are no abnormal values
		piUnlock(MEASUREMENT_LOCK);

		// if there is an abnormal temperature value
		if (dht->t_value < 18.0 || dht->t_value > 28.0) {
			piLock(MEASUREMENT_LOCK);
			measurement_flags |= FLAG_TEMP_ANOMALY;
			piUnlock(MEASUREMENT_LOCK);
		}

		// if there is an abnormal humidity value
		if (dht->rh_value < 30.0 || dht->rh_value > 70.0) {
			piLock(MEASUREMENT_LOCK);
			measurement_flags |= FLAG_HUMID_ANOMALY;
			piUnlock(MEASUREMENT_LOCK);
		}

		// if there is a critical temperature value
		if (dht->t_value < 5.0 || dht->t_value > 33.0) {
			piLock(MEASUREMENT_LOCK);
			measurement_flags |= FLAG_TEMP_EMERGENCY;
			piUnlock(MEASUREMENT_LOCK);
		}

		// if there is an critical humidity value
		if (dht->rh_value < 10.0 || dht->rh_value > 82.0) {
			piLock(MEASUREMENT_LOCK);
			measurement_flags |= FLAG_HUMID_EMERGENCY;
			piUnlock(MEASUREMENT_LOCK);
		}

	}
}

static void _light_do_measurement(fsm_t *this) {
	BH1750Sensor* bh = ((SystemContext*) this->user_data)->sensor_light;
	int err = BH1750Sensor__perform_measurement(bh);
	if (err >= 0) {
		piLock(MEASUREMENT_LOCK);
		measurement_flags &= ~(FLAG_LIGHT_PENDING_MEASUREMENT);
		measurement_flags &= ~(FLAG_LIGHT_ANOMALY | FLAG_LIGHT_EMERGENCY); // assume there are no abnormal values
		piUnlock(MEASUREMENT_LOCK);

		// if there is an abnormal light value
		if (bh->lux < 350) {
			piLock(MEASUREMENT_LOCK);
			measurement_flags |= FLAG_LIGHT_ANOMALY;
			piUnlock(MEASUREMENT_LOCK);
		}

		// if there is a critical light value
		if (bh->lux < 150) {
			piLock(MEASUREMENT_LOCK);
			measurement_flags |= FLAG_LIGHT_EMERGENCY;
			piUnlock(MEASUREMENT_LOCK);
		}
	}

}


static void _co2_do_measurement(fsm_t *this) {
	CCS811Sensor* ccs = ((SystemContext*) this->user_data)->sensor_co2;
	DHT11Sensor* dht = ((SystemContext*) this->user_data)->sensor_temp_humid;

	// check if temp and humid values available and send them to the co2 sensor
	if (!_temp_humid_pending_measurement(this)){
		CCS811Sensor__set_environment_data(ccs, dht->t_value, dht->rh_value);

		// check if co2 measurement available
		if(CCS811Sensor__available(ccs)){
			piLock(MEASUREMENT_LOCK);
			measurement_flags &= ~(FLAG_CO2_PENDING_MEASUREMENT);
			measurement_flags &= ~(FLAG_CO2_ANOMALY | FLAG_CO2_EMERGENCY); // assume there are no abnormal values
			piUnlock(MEASUREMENT_LOCK);

			CCS811Sensor__read_register(ccs, ALG_RESULT_DATA);
			int eco2 = ccs->app_register.alg_result_data.eco2;
			if (eco2 > 1000){
				piLock(MEASUREMENT_LOCK);
				measurement_flags |= FLAG_CO2_ANOMALY;
				piUnlock(MEASUREMENT_LOCK);
			}
			if(eco2 > 4000){
				piLock(MEASUREMENT_LOCK);
				measurement_flags |= FLAG_CO2_EMERGENCY;
				piUnlock(MEASUREMENT_LOCK);
			}
		}
	}
}
