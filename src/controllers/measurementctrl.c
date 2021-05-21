/*
 * measurementctrl.c
 *
 *  Created on: 14 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#include <stdlib.h>
#include <wiringPi.h>
#include <curl/curl.h>
#include <string.h>

#include "measurementctrl.h"
#include "../libs/threadlib.h"
#include "../libs/timerlib.h"
#include "../libs/systemlib.h"
#include "../libs/fsm.h"

extern int measurement_flags;

// Timer
static void _measurement_timer_isr(union sigval value);

// FSM states enum
enum _fsm_state {
	MEASUREMENT_PROCESS, GENERATE_ALERTS, DB_UPDATE
};

static int _measurement_pending_processing(fsm_t *this);
static int _measurement_processing_finished(fsm_t *this);
static int _measurement_alerts_finished(fsm_t *this);

// FSM output action functions
static void _measurement_do_processing(fsm_t *this);
static void _measurement_do_alerts(fsm_t *this);
static void _measurement_do_database_update(fsm_t *this);

// { EstadoOrigen, CondicionDeDisparo, EstadoFinal, AccionesSiTransicion }
static fsm_trans_t _measurement_fsm_tt[] = { { MEASUREMENT_PROCESS, _measurement_pending_processing, GENERATE_ALERTS, _measurement_do_processing }, { GENERATE_ALERTS, _measurement_processing_finished,
		DB_UPDATE, _measurement_do_alerts }, { DB_UPDATE, _measurement_alerts_finished, MEASUREMENT_PROCESS, _measurement_do_database_update }, { -1, NULL, -1, NULL } };

MeasurementCtrl* MeasurementCtrl__setup(SystemContext *this_system) {
	MeasurementCtrl *result = (MeasurementCtrl*) malloc(sizeof(MeasurementCtrl));
	tmr_t *measurement_timer = tmr_new(_measurement_timer_isr); // creado pero no iniciado
	result->timer = measurement_timer;

	result->fsm = (fsm_t*) fsm_new(MEASUREMENT_PROCESS, _measurement_fsm_tt, this_system);
	return result;
}

void MeasurementCtrl__destroy(MeasurementCtrl *this) {
	if (this) {
		fsm_destroy(this->fsm);
		tmr_destroy(this->timer);

		free(this);
	}
}

/* Definition of the functions */

static void _measurement_timer_isr(union sigval value) {
	piLock(MEASUREMENT_LOCK);
	measurement_flags |= (FLAG_PERFORM_PROCESSING);
	piUnlock(MEASUREMENT_LOCK);
}

static int _measurement_pending_processing(fsm_t *this) {
	return (measurement_flags & FLAG_PERFORM_PROCESSING);
}

static int _measurement_processing_finished(fsm_t *this) {
	return (measurement_flags & FLAG_PROCESSING_READY);
}

static int _measurement_alerts_finished(fsm_t *this) {
	return (measurement_flags & FLAG_ALERTS_READY);
}

/* helper functions */

static void _temp_humid_do_alerts(SystemContext *this);
static void _light_do_alerts(SystemContext *this);
static void _co2_do_alerts(SystemContext *this);

static void _get_highest_lowest_index_from_array(SensorValueType *array, int val_type, int arr_len, int *h_idx, int *l_idx) {
	SensorValueType highest, lowest;

	int t_h, t_l = -1;

	highest.type = val_type;
	lowest.type = val_type;

	switch (val_type) {
	case is_int:
		highest.val.ival = -99;
		lowest.val.ival = 999;
		break;
	case is_float:
		highest.val.fval = -99.0;
		lowest.val.fval = 999.0;
		break;
	default:
		highest.val.ival = -99;
		lowest.val.ival = 999;
		break;
	}

	for (int i = 0; i < arr_len; i++) {
		switch (array[i].type) {
		case is_int:
			if (array[i].val.ival > highest.val.ival) {
				highest = array[i];
				t_h = i;
			}
			if (array[i].val.ival < lowest.val.ival) {
				lowest = array[i];
				t_l = i;
			}
			break;
		case is_float:
			if (array[i].val.fval > highest.val.fval) {
				highest = array[i];
				t_h = i;
			}
			if (array[i].val.fval < lowest.val.fval) {
				lowest = array[i];
				t_l = i;
			}
			break;
		default:
			break;
		}
	}

	*h_idx = t_h;
	*l_idx = t_l;
}

static void _measurement_do_processing(fsm_t *this) {
	piLock(MEASUREMENT_LOCK);
	measurement_flags &= ~(FLAG_PERFORM_PROCESSING);
	piUnlock(MEASUREMENT_LOCK);

	SystemContext *this_system = (SystemContext*) this->user_data;
	// iterate for each sensor
	for (int i = 0; i < sizeof(this_system->sensor_storage) / sizeof(CircularBuffer); i++) {
		SensorValueType tmp[5];

		piLock(STORAGE_LOCK);
		CircularBufferRead(this_system->sensor_storage[i], 5 * sizeof(SensorValueType), tmp); // copy circular buffer to SensorValueType array
		piUnlock(STORAGE_LOCK);

		int type = i < 2 ? is_float : is_int;

		int h_idx, l_idx;
		_get_highest_lowest_index_from_array(tmp, type, 5, &h_idx, &l_idx);

		SensorValueType avg = { .type = type, .val.ival = 0, .val.fval = 0.0 };

		int iters = 0;

		for (int j = 0; j < sizeof(tmp) / sizeof(SensorValueType); j++) {
			if (j != h_idx && j != l_idx && tmp[j].type != is_error) {
				switch (type) {
				case is_int:
					avg.val.ival += tmp[j].val.ival;
					break;
				case is_float:
					avg.val.fval += tmp[j].val.fval;
				}
				iters++;
			}
		}

		if (iters != 0) {
			switch (type) {
			case is_int:
				avg.val.ival /= iters;
				break;
			case is_float:
				avg.val.fval /= iters;
			}

			this_system->sensor_values[i] = avg;
		} else {
			SensorValueType error_val = { .type = is_error, .val.ival = 0 };
			this_system->sensor_values[i] = error_val;
		}

	}

	piLock(MEASUREMENT_LOCK);
	measurement_flags |= FLAG_PROCESSING_READY;
	piUnlock(MEASUREMENT_LOCK);
}

static void _measurement_do_alerts(fsm_t *this) {
	piLock(MEASUREMENT_LOCK);
	measurement_flags &= ~(FLAG_PROCESSING_READY);
	piUnlock(MEASUREMENT_LOCK);

	_temp_humid_do_alerts(this->user_data);
	_light_do_alerts(this->user_data);
	_co2_do_alerts(this->user_data);

	piLock(MEASUREMENT_LOCK);
	measurement_flags |= FLAG_ALERTS_READY;
	piUnlock(MEASUREMENT_LOCK);
}

static void _measurement_do_database_update(fsm_t *this) {
	piLock(MEASUREMENT_LOCK);
	measurement_flags &= ~(FLAG_ALERTS_READY);
	piUnlock(MEASUREMENT_LOCK);

	SystemContext *this_system = (SystemContext*) this->user_data;

	char sensor_meas_name[4][6] = { "temp", "rh", "lux", "eco2" };

	// iterate for each sensor
	for (int i = 0; i < sizeof(this_system->sensor_values) / sizeof(SensorValueType); i++) {
		if (this_system->sensor_values[i].type != is_error) {
			char data[100];

			switch (this_system->sensor_values[i].type) {
			case is_int:
				sprintf(data, "%s value=%d", sensor_meas_name[i], this_system->sensor_values[i].val.ival);
				break;
			case is_float:
				sprintf(data, "%s value=%f", sensor_meas_name[i], this_system->sensor_values[i].val.fval);
				break;
			default:
				break;
			}

			CURL *hnd;

			hnd = curl_easy_init();
			curl_easy_setopt(hnd, CURLOPT_BUFFERSIZE, 102400L);
			curl_easy_setopt(hnd, CURLOPT_URL, "http://localhost:8086/write?db=db0");
			curl_easy_setopt(hnd, CURLOPT_NOPROGRESS, 1L);
			curl_easy_setopt(hnd, CURLOPT_POSTFIELDS, data);
			curl_easy_setopt(hnd, CURLOPT_POSTFIELDSIZE_LARGE, (curl_off_t ) strlen(data));
			curl_easy_setopt(hnd, CURLOPT_USERAGENT, "curl/roompisys/7.77.0-DEV");
			curl_easy_setopt(hnd, CURLOPT_MAXREDIRS, 50L);
			curl_easy_setopt(hnd, CURLOPT_HTTP_VERSION, (long )CURL_HTTP_VERSION_2_0);
			curl_easy_setopt(hnd, CURLOPT_CUSTOMREQUEST, "POST");
			curl_easy_setopt(hnd, CURLOPT_FTP_SKIP_PASV_IP, 1L);
			curl_easy_setopt(hnd, CURLOPT_TCP_KEEPALIVE, 1L);

			curl_easy_perform(hnd);

			curl_easy_cleanup(hnd);
			hnd = NULL;
		}
	}
}

static void _temp_humid_do_alerts(SystemContext *this) {
	float t_val = this->sensor_values[0].val.fval;
	float rh_val = this->sensor_values[1].val.fval;

	piLock(MEASUREMENT_LOCK);
	measurement_flags &= ~(FLAG_TEMP_ANOMALY | FLAG_TEMP_EMERGENCY | FLAG_HUMID_ANOMALY | FLAG_HUMID_EMERGENCY); // assume there are no abnormal values
	piUnlock(MEASUREMENT_LOCK);

	extern float temp_warn_low, temp_warn_high, rh_warn_low, rh_warn_high, temp_crit_low, temp_crit_high, rh_crit_low, rh_crit_high;

	if (this->sensor_values[0].type != is_error && this->sensor_values[1].type != is_error) {
		// if there is an abnormal temperature value
		if (t_val < temp_warn_low || t_val > temp_warn_high) {
			piLock(MEASUREMENT_LOCK);
			measurement_flags |= FLAG_TEMP_ANOMALY;
			piUnlock(MEASUREMENT_LOCK);
		}

		// if there is an abnormal humidity value
		if (rh_val < rh_warn_low || rh_val > rh_warn_high) {
			piLock(MEASUREMENT_LOCK);
			measurement_flags |= FLAG_HUMID_ANOMALY;
			piUnlock(MEASUREMENT_LOCK);
		}

		// if there is a critical temperature value
		if (t_val < temp_crit_low || t_val > temp_crit_high) {
			piLock(MEASUREMENT_LOCK);
			measurement_flags |= FLAG_TEMP_EMERGENCY;
			piUnlock(MEASUREMENT_LOCK);
		}

		// if there is an critical humidity value
		if (rh_val < rh_crit_low || rh_val > rh_crit_high) {
			piLock(MEASUREMENT_LOCK);
			measurement_flags |= FLAG_HUMID_EMERGENCY;
			piUnlock(MEASUREMENT_LOCK);
		}
	}
}

static void _light_do_alerts(SystemContext *this) {
	int l_val = this->sensor_values[2].val.ival;

	piLock(MEASUREMENT_LOCK);
	measurement_flags &= ~(FLAG_LIGHT_ANOMALY | FLAG_LIGHT_EMERGENCY); // assume there are no abnormal values
	piUnlock(MEASUREMENT_LOCK);

	extern int lux_warn, lux_crit;

	if (this->sensor_values[2].type != is_error) {
		// if there is an abnormal light value
		if (l_val < lux_warn) {
			piLock(MEASUREMENT_LOCK);
			measurement_flags |= FLAG_LIGHT_ANOMALY;
			piUnlock(MEASUREMENT_LOCK);
		}

		// if there is a critical light value
		if (l_val < lux_crit) {
			piLock(MEASUREMENT_LOCK);
			measurement_flags |= FLAG_LIGHT_EMERGENCY;
			piUnlock(MEASUREMENT_LOCK);
		}
	}
}

static void _co2_do_alerts(SystemContext *this) {
	int eco2_val = this->sensor_values[3].val.ival;

	piLock(MEASUREMENT_LOCK);
	measurement_flags &= ~(FLAG_CO2_ANOMALY | FLAG_CO2_EMERGENCY); // assume there are no abnormal values
	piUnlock(MEASUREMENT_LOCK);

	extern int eco2_warn, eco2_crit;

	if (this->sensor_values[3].type != is_error) {
		if (eco2_val > eco2_warn) {
			piLock(MEASUREMENT_LOCK);
			measurement_flags |= FLAG_CO2_ANOMALY;
			piUnlock(MEASUREMENT_LOCK);
		}
		if (eco2_val > eco2_crit) {
			piLock(MEASUREMENT_LOCK);
			measurement_flags |= FLAG_CO2_EMERGENCY;
			piUnlock(MEASUREMENT_LOCK);
		}
	}
}
