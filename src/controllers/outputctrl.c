/*
 * outputctrl.c
 *
 *  Created on: 15 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#include <stdlib.h>
#include <wiringPi.h>
#include <time.h>

#include "outputctrl.h"
#include "measurementctrl.h"
#include "../libs/threadlib.h"
#include "../libs/timerlib.h"
#include "../libs/systemlib.h"
#include "../libs/fsm.h"

extern int output_flags;
extern int measurement_flags;

// Timer
static void _output_timer_isr(union sigval value);

// FSM states enum
enum _fsm_buzzer_state {
	OFF, ON
};
enum _fsm_leds_state {
	NORMAL, ANOMALY, EMERGENCY
};
enum _fsm_info_state {
	HOUR_INFO, TEMPERATURE_INFO, HUMIDITY_INFO, LIGHT_INFO
};

enum _fsm_warning_state {
	NO_WARNING, TEMPERATURE_WARNING, HUMIDITY_WARNING, LIGHT_WARNING
};

// FSM input check functions
static int _next_display_info(fsm_t *this); // activated every 5 s
static int _next_display_warning(fsm_t *this); // activated every 5 s
static int _general_anomaly(fsm_t *this); // anomaly in at least 1 sensor
static int _not_general_anomaly(fsm_t *this) {
	return !(_general_anomaly(this));
}
static int _not_general_anomaly_and_next_display_warning(fsm_t *this) {
	return (_not_general_anomaly(this) && _next_display_warning(this));
}
static int _general_emergency(fsm_t *this); // emergency in at least 1 sensor
static int _not_general_emergency(fsm_t *this) {
	return !(_general_emergency(this));
}

static int _temp_anomaly(fsm_t *this); // temperature sensor anomaly
static int _humid_anomaly(fsm_t *this); // humidity sensor anomaly
static int _light_anomaly(fsm_t *this); // light sensor anomaly
//static int _not_temp_anomaly(fsm_t *this) {
//	return !(_temp_anomaly(this));
//}
//static int _not_humid_anomaly(fsm_t *this){
//	return !(_humid_anomaly(this));
//}
//static int _not_light_anomaly(fsm_t *this) {
//	return !(_light_anomaly(this));
//}

//static int _temp_emergency(fsm_t *this); // temperature sensor emergency
//static int _humid_emergency(fsm_t *this); // humidity sensor emergency
//static int _light_emergency(fsm_t *this); // light sensor emergency
//static int _not_temp_emergency(fsm_t *this) {
//	return !(_temp_emergency(this));
//}
//static int _not_humid_emergency(fsm_t *this){
//	return !(_humid_emergency(this));
//}
//static int _not_light_emergency(fsm_t *this) {
//	return !(_light_emergency(this));
//}

//static int _temp_anomaly_and_next_display(fsm_t* this) {
//	return (_temp_anomaly(this) && _next_display(this));
//}
static int _humid_anomaly_and_next_display_warning(fsm_t* this) {
	return (_humid_anomaly(this) && _next_display_warning(this));
}
static int _light_anomaly_and_next_display_warning(fsm_t* this) {
	return (_light_anomaly(this) && _next_display_warning(this));
}
//static int _not_temp_anomaly_and_next_display(fsm_t* this) {
//	return (_not_temp_anomaly(this) && _next_display(this));
//}
//static int _not_humid_anomaly_and_next_display(fsm_t* this) {
//	return (_not_humid_anomaly(this) && _next_display(this));
//}

// FSM output action functions
//FSM buzzer
static void _buzzer_on(fsm_t *this);
static void _buzzer_off(fsm_t *this);

// FSM led array
static void _set_green_leds(fsm_t *this);
static void _set_yellow_leds(fsm_t *this);
static void _set_red_leds(fsm_t *this);

// FSM display info (bottom row)
static void _show_info_hour(fsm_t* this);
static void _show_info_temp(fsm_t* this);
static void _show_info_humid(fsm_t* this);
static void _show_info_light(fsm_t* this);

// FSM display warning (top row)
static void _show_warning_none(fsm_t* this);
static void _show_warning_temp(fsm_t* this);
static void _show_warning_humid(fsm_t* this);
static void _show_warning_light(fsm_t* this);


static fsm_trans_t _buzzer_fsm_tt[] = {
		{ OFF, _general_emergency, ON, _buzzer_on},
		{ ON, _not_general_emergency, OFF, _buzzer_off},
		{-1, NULL, -1, NULL}
};

static fsm_trans_t _leds_fsm_tt[] = {
		{ NORMAL, _general_anomaly, ANOMALY, _set_yellow_leds },
		{ ANOMALY, _general_emergency, EMERGENCY, _set_red_leds },
		{ EMERGENCY, _not_general_emergency, ANOMALY, _set_yellow_leds },
		{ ANOMALY, _not_general_anomaly, NORMAL, _set_green_leds },
		{-1, NULL, -1, NULL}
};

static fsm_trans_t _info_fsm_tt[] = {
		{ HOUR_INFO, _next_display_info, TEMPERATURE_INFO, _show_info_temp },
		{ TEMPERATURE_INFO, _next_display_info, HUMIDITY_INFO, _show_info_humid },
		{ HUMIDITY_INFO, _next_display_info, LIGHT_INFO, _show_info_light },
		{ LIGHT_INFO, _next_display_info, HOUR_INFO, _show_info_hour },
		{-1, NULL, -1, NULL}
};

static fsm_trans_t _warning_fsm_tt[] = {
		{ NO_WARNING, _not_general_anomaly_and_next_display_warning, NO_WARNING, _show_warning_none },
		{ NO_WARNING, _temp_anomaly, TEMPERATURE_WARNING, _show_warning_temp },
		{ NO_WARNING, _humid_anomaly, HUMIDITY_WARNING, _show_warning_humid },
		{ NO_WARNING, _light_anomaly, LIGHT_WARNING, _show_warning_light },
		{ TEMPERATURE_WARNING, _humid_anomaly_and_next_display_warning, HUMIDITY_WARNING, _show_warning_humid },
		{ TEMPERATURE_WARNING, _light_anomaly_and_next_display_warning, LIGHT_WARNING, _show_warning_light },
		{ HUMIDITY_WARNING, _light_anomaly_and_next_display_warning, LIGHT_WARNING, _show_warning_light },
		{ TEMPERATURE_WARNING, _next_display_warning, NO_WARNING, NULL },
		{ HUMIDITY_WARNING, _next_display_warning, NO_WARNING, NULL },
		{ LIGHT_WARNING, _next_display_warning, NO_WARNING, NULL },
		{-1, NULL, -1, NULL}
};

OutputCtrl* OutputCtrl__setup(SystemContext* this_system) {
	OutputCtrl *result = (OutputCtrl*) malloc(sizeof(OutputCtrl));
	tmr_t *output_timer = tmr_new(_output_timer_isr); // creado pero no iniciado
	result->timer = output_timer;

	result->fsm_buzzer = (fsm_t *) fsm_new(OFF, _buzzer_fsm_tt, this_system);
	result->fsm_leds = (fsm_t *) fsm_new(NORMAL, _leds_fsm_tt, this_system);
	result->fsm_info = (fsm_t *) fsm_new(HOUR_INFO, _info_fsm_tt, this_system);
	result->fsm_warnings = (fsm_t *) fsm_new(NO_WARNING, _warning_fsm_tt, this_system);

	return result;
}

void OutputCtrl__destroy(OutputCtrl * this) {
	if (this)  {
		fsm_destroy(this->fsm_buzzer);
		fsm_destroy(this->fsm_leds);
		fsm_destroy(this->fsm_info);
		fsm_destroy(this->fsm_warnings);

		tmr_destroy(this->timer);

		free(this);
	}
}

/* Definition of the functions */

static void _output_timer_isr(union sigval value) {
	piLock(OUTPUT_LOCK);
	output_flags |= FLAG_NEXT_DISPLAY_INFO;
	output_flags |= FLAG_NEXT_DISPLAY_WARNING;
	piUnlock(OUTPUT_LOCK);
}

static int _next_display_info(fsm_t *this) {
	int res = output_flags & FLAG_NEXT_DISPLAY_INFO;
	return res;
}
static int _next_display_warning(fsm_t *this) {
	int res = output_flags & FLAG_NEXT_DISPLAY_WARNING;
	return res;
}

static int _general_anomaly(fsm_t *this) {
	int res = measurement_flags & (FLAG_TEMP_ANOMALY | FLAG_HUMID_ANOMALY | FLAG_LIGHT_ANOMALY);
	return res;
}

static int _general_emergency(fsm_t *this) {
	int res = measurement_flags & (FLAG_TEMP_EMERGENCY | FLAG_HUMID_EMERGENCY | FLAG_LIGHT_EMERGENCY);
	return res;
}

static int _temp_anomaly(fsm_t *this) {
	int res = measurement_flags & FLAG_TEMP_ANOMALY;
	return res;
}

static int _humid_anomaly(fsm_t *this) {
	int res = measurement_flags & FLAG_HUMID_ANOMALY;
	return res;
}

static int _light_anomaly(fsm_t *this) {
	int res = measurement_flags & FLAG_LIGHT_ANOMALY;
	return res;
}

//static int _temp_emergency(fsm_t *this) {
//	return (measurement_flags & FLAG_TEMP_EMERGENCY);
//}
//
//static int _humid_emergency(fsm_t *this) {
//	return (measurement_flags & FLAG_HUMID_EMERGENCY);
//}
//
//static int _light_emergency(fsm_t *this) {
//	return (measurement_flags & FLAG_LIGHT_EMERGENCY);
//}



static void _buzzer_on(fsm_t *this) {
	BuzzerOutput* buzzer = ((SystemContext*) this->user_data)->actuator_buzzer;
	BuzzerOutput__enable(buzzer);
}

static void _buzzer_off(fsm_t *this) {
	BuzzerOutput* buzzer = ((SystemContext*) this->user_data)->actuator_buzzer;
	BuzzerOutput__disable(buzzer);
}

static void _set_green_leds(fsm_t *this) {
	StatusLEDOutput* leds = ((SystemContext*) this->user_data)->actuator_leds;
	StatusLEDOutput__set_color(leds, GREEN);
}

static void _set_yellow_leds(fsm_t *this) {
	StatusLEDOutput* leds = ((SystemContext*) this->user_data)->actuator_leds;
	StatusLEDOutput__set_color(leds, YELLOW);
}

static void _set_red_leds(fsm_t *this) {
	StatusLEDOutput* leds = ((SystemContext*) this->user_data)->actuator_leds;
	StatusLEDOutput__set_color(leds, RED);
}

static void _show_info_hour(fsm_t* this) {
	LCD1602Display* display = ((SystemContext*) this->user_data)->actuator_display;

	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	LCD1602Display__set_cursor(display, 0, 1);
	LCD1602Display__print(display, "                ");
	LCD1602Display__set_cursor(display, 0, 1);
	LCD1602Display__print(display, "%02d:%02d %02d-%02d-%d", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_mday, 1 + timeinfo->tm_mon, 1900 + timeinfo->tm_year);

	piLock(OUTPUT_LOCK);
	output_flags &= ~(FLAG_NEXT_DISPLAY_INFO);
	piUnlock(OUTPUT_LOCK);
}

static void _show_info_temp(fsm_t* this) {
	LCD1602Display* display = ((SystemContext*) this->user_data)->actuator_display;
	DHT11Sensor* dht = ((SystemContext*) this->user_data)->sensor_temp_humid;

	LCD1602Display__set_cursor(display, 0, 1);
	LCD1602Display__print(display, "                ");
	LCD1602Display__set_cursor(display, 0, 1);
	LCD1602Display__print(display, "Temp: %.1f ", dht->t_value);

	int degrees_symbol = 0b11011111;
	LCD1602Display__write(display, degrees_symbol);
	LCD1602Display__print(display, "C");

	piLock(OUTPUT_LOCK);
	output_flags &= ~(FLAG_NEXT_DISPLAY_INFO);
	piUnlock(OUTPUT_LOCK);
}

static void _show_info_humid(fsm_t* this) {
	LCD1602Display* display = ((SystemContext*) this->user_data)->actuator_display;
	DHT11Sensor* dht = ((SystemContext*) this->user_data)->sensor_temp_humid;

	LCD1602Display__set_cursor(display, 0, 1);
	LCD1602Display__print(display, "                ");
	LCD1602Display__set_cursor(display, 0, 1);
	LCD1602Display__print(display, "Humidity: %.1f %%", dht->rh_value);

	piLock(OUTPUT_LOCK);
	output_flags &= ~(FLAG_NEXT_DISPLAY_INFO);
	piUnlock(OUTPUT_LOCK);
}

static void _show_info_light(fsm_t* this) {
	LCD1602Display* display = ((SystemContext*) this->user_data)->actuator_display;
	BH1750Sensor* bh = ((SystemContext*) this->user_data)->sensor_light;

	LCD1602Display__set_cursor(display, 0, 1);
	LCD1602Display__print(display, "                ");
	LCD1602Display__set_cursor(display, 0, 1);
	LCD1602Display__print(display, "Light: %d lx", bh->lux);

	piLock(OUTPUT_LOCK);
	output_flags &= ~(FLAG_NEXT_DISPLAY_INFO);
	piUnlock(OUTPUT_LOCK);
}

static void _show_warning_none(fsm_t* this) {
	LCD1602Display* display = ((SystemContext*) this->user_data)->actuator_display;

	LCD1602Display__set_cursor(display, 0, 0);
	LCD1602Display__print(display, "                ");
	LCD1602Display__set_cursor(display, 0, 0);
	LCD1602Display__print(display, "roomPi      v4.0");

	piLock(OUTPUT_LOCK);
	output_flags &= ~(FLAG_NEXT_DISPLAY_WARNING);
	piUnlock(OUTPUT_LOCK);
}

static void _show_warning_temp(fsm_t* this) {
	LCD1602Display* display = ((SystemContext*) this->user_data)->actuator_display;

	LCD1602Display__set_cursor(display, 0, 0);
	LCD1602Display__print(display, "                ");
	LCD1602Display__set_cursor(display, 0, 0);
	LCD1602Display__write(display, 4);
	LCD1602Display__print(display, " AVISO TEMP.");

	piLock(OUTPUT_LOCK);
	output_flags &= ~(FLAG_NEXT_DISPLAY_WARNING);
	piUnlock(OUTPUT_LOCK);
}

static void _show_warning_humid(fsm_t* this) {
	LCD1602Display* display = ((SystemContext*) this->user_data)->actuator_display;

	LCD1602Display__set_cursor(display, 0, 0);
	LCD1602Display__print(display, "                ");
	LCD1602Display__set_cursor(display, 0, 0);
	LCD1602Display__write(display, 5);
	LCD1602Display__print(display, " AVISO HUMED.");

	piLock(OUTPUT_LOCK);
	output_flags &= ~(FLAG_NEXT_DISPLAY_WARNING);
	piUnlock(OUTPUT_LOCK);
}

static void _show_warning_light(fsm_t* this) {
	LCD1602Display* display = ((SystemContext*) this->user_data)->actuator_display;

	LCD1602Display__set_cursor(display, 0, 0);
	LCD1602Display__print(display, "                ");
	LCD1602Display__set_cursor(display, 0, 0);
	LCD1602Display__write(display, 7);
	LCD1602Display__print(display, " MUY POCA LUZ");

	piLock(OUTPUT_LOCK);
	output_flags &= ~(FLAG_NEXT_DISPLAY_WARNING);
	piUnlock(OUTPUT_LOCK);
}
