/*
 * main.c
 *
 *  Created on: 15 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wiringPi.h>

#define DEB

volatile int measurement_flags = 0x00;
volatile int output_flags = 0x00;

float temp_crit_low = 5.0;
float temp_crit_high = 33.0;
float temp_warn_low = 18.0;
float temp_warn_high = 28.0;
float rh_crit_low = 10.0;
float rh_crit_high = 82.0;
float rh_warn_low = 30.0;
float rh_warn_high = 70.0;
int lux_crit = 150;
int lux_warn = 350;
int eco2_crit = 4000;
int eco2_warn = 2000;
int meas_t_ms = 30000;
int dht_t_ms = 5000;
int bh1750_t_ms = 5000;
int ccs811_t_ms = 5000;
int output_t_ms = 5000;

volatile int buzzer_disabled = 0x0;

#include "libs/systemlib.h"
#include "libs/systemtype.h"
#include "controllers/measurementctrl.h"
#include "controllers/outputctrl.h"

SystemType *roompi_system;

SystemType* systemSetup(void) {

	printf("[LOG] System is being initialized and set up...\n");
	// wiringPi Setup
	wiringPiSetup();

	/* Creation of the attached sensors */
	// DHT11 Temperature and Humidity Creation and Setup
	printf("[LOG-DHT11Sensor] DHT11 Sensor is being initialized and set up...\n");
	DHT11Sensor *dht_sensor = DHT11Sensor__create(1, 29);

	// BH1750 Lux sensor Creation and Setup
	printf("[LOG-BH1750Sensor] BH1750 Sensor is being initialized and set up...\n");
	BH1750Sensor *bh_sensor = BH1750Sensor__create(4, 0x23, CONTINUOUS_H_RES);

	// CCS811 CO2 sensor Creation and Setup
	printf("[LOG-CCS811Sensor]  CCS811Sensor is being initialized and set up...\n");
	CCS811Sensor *ccs_sensor = CCS811Sensor__create(5, CCS811_ADDR_LOW, 0, 3, 2);

	CCS811Sensor__connect(ccs_sensor);

	union ApplicationRegister appreg1 = ccs_sensor->app_register;
	appreg1.meas_mode.reserved2 = 0;             //We dont know what reserved bits do, so let's put them zero
	appreg1.meas_mode.reserved1 = 0;             //We dont know what reserved bits do, so let's put them zero
	appreg1.meas_mode.driveMode = MODE1_EACH_1S; //Lets select the operation mode: MODE0_IDLE MODE1_EACH_1S MODE2_EACH_10S MODE3_EACH_60S MODE4_EACH_250MS
	appreg1.meas_mode.int_data_ready = 1;        //Lets enable the interrupt pin after we have a valid data
	appreg1.meas_mode.int_thresh = 0;            //We are going to disable the interrupts for thresholds

	CCS811Sensor__set_app_register(ccs_sensor, appreg1);
	CCS811Sensor__write_register(ccs_sensor, MEAS_MODE);

	/* Creation of the attached actuators */

	// Buzzer output creation and setup
	printf("[LOG-BuzzerOutput] BuzzerOutput Actuator is being initialized and set up...\n");
	BuzzerOutput *buzzer_actuator = BuzzerOutput__create(2, 26);

	// LED Array (status leds) creation and setup
	printf("[LOG-StatusLEDOutput] StatusLEDOutput Actuator is being initialized and set up...\n");
	int color_pins[] = { 0b00000011, 0b00011100, 0b11100000 };
	StatusLEDOutput *leds_actuator = StatusLEDOutput__create(3, 1, 25, 24, 23, color_pins);
	StatusLEDOutput__set_all_high(leds_actuator);
	//delay(5000);

	// LCD1602 Character display creation and setup
	printf("[LOG-LCD1602Display] LCD1602Display Actuator is being initialized and set up...\n");
	LCD1602Display *lcd_actuator = LCD1602Display__create(0, 15, 255, 16, 1, 10, 11, 31, 26, 1, 4, 5, 6);
	LCD1602Display__begin(lcd_actuator, 16, 2, 0);

	/* Creation of custom characters for the display */
	int clock[8] = { 0x1F, 0x11, 0x0A, 0x04, 0x0E, 0x1F, 0x1F, 0x00 }; // clock symbol for wait operations
	int arrow[8] = { 0b00000, 0b00100, 0b00010, 0b11111, 0b00010, 0b00100, 0b00000, 0b00000 }; // arrow
	int general_bad[8] = { 0x00, 0x0A, 0x0A, 0x0A, 0x00, 0x0E, 0x11, 0x00 }; // bad
	int air[8] = { 0x00, 0x0C, 0x05, 0x1B, 0x14, 0x06, 0x00, 0x00 }; // Co2
	int temp[8] = { 0x04, 0x0A, 0x0A, 0x0A, 0x0A, 0x11, 0x13, 0x0E }; // temp
	int humid[8] = { 0x00, 0x04, 0x0A, 0x11, 0x11, 0x1F, 0x0E, 0x00 }; // humid
	//int noise[8] = { 0x01, 0x03, 0x07, 0x1F, 0x1F, 0x07, 0x03, 0x01 }; // dBs
	int lux[8] = { 0x04, 0x15, 0x0E, 0x1B, 0x0E, 0x15, 0x04, 0x00 }; // lux
	int test[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }; // test
	LCD1602Display__create_char(lcd_actuator, 0, clock);
	LCD1602Display__create_char(lcd_actuator, 1, arrow);
	LCD1602Display__create_char(lcd_actuator, 2, general_bad);
	LCD1602Display__create_char(lcd_actuator, 3, air);
	LCD1602Display__create_char(lcd_actuator, 4, temp);
	LCD1602Display__create_char(lcd_actuator, 5, humid);
	LCD1602Display__create_char(lcd_actuator, 6, test);
	LCD1602Display__create_char(lcd_actuator, 7, lux);

	LCD1602Display__clear(lcd_actuator);
	LCD1602Display__set_cursor(lcd_actuator, 0, 0);
	LCD1602Display__print(lcd_actuator, "roomPi      v7.0");
	LCD1602Display__set_cursor(lcd_actuator, 0, 1);
	LCD1602Display__write(lcd_actuator, 0);
	LCD1602Display__print(lcd_actuator, " Iniciando...");

	// System Context creation and initialization
	printf("[LOG] System is starting...\n");
	SystemContext *roompi_system_ctx = SystemContext__create(001, dht_sensor, bh_sensor, ccs_sensor, lcd_actuator, buzzer_actuator, leds_actuator);

	// Measurement subsystem creation and initialization
	MeasurementCtrl *measurement_ctrl = MeasurementCtrl__setup(roompi_system_ctx);

	// Output subsystem creation and initialization
	OutputCtrl *output_ctrl = OutputCtrl__setup(roompi_system_ctx);

	// Create and Setup Root System Type
	SystemType *roompi_system = SystemType__setup(roompi_system_ctx, measurement_ctrl, output_ctrl);

	return roompi_system;
}

int main(int argc, char **argv) {
	roompi_system = systemSetup();

	int filerr = 0;

	// load system config options
	FILE *fp = fopen("/home/pi/roompi.conf", "r");
	if (fp != NULL) {
		char chunk[64];
		char parsed[8];

		for (int i = 0; i < 18; i++) {
			if (fgets(chunk, sizeof(chunk), fp) != NULL) {
				char *cp = strrchr(chunk, ' ');
				if (cp && *(cp + 1)) {
					sprintf(parsed, "%s", cp + 1);
					switch (i) {
					case 0:
						LCD1602Display__set_cursor(roompi_system->root_system->actuator_display, 0, 0);
						LCD1602Display__print(roompi_system->root_system->actuator_display, "                ");
						char aux[64];
						strcpy(aux, chunk);
						aux[strlen(aux) - 1] = '\0';
						char *po = strrchr(aux, '=');
						LCD1602Display__set_cursor(roompi_system->root_system->actuator_display, 0, 0);
						LCD1602Display__write(roompi_system->root_system->actuator_display, 1);
						LCD1602Display__print(roompi_system->root_system->actuator_display, " %s", po + 2);
						break;
					case 1:
						temp_crit_low = atof(parsed);
						break;
					case 2:
						temp_crit_high = atof(parsed);
						break;
					case 3:
						temp_warn_low = atof(parsed);
						break;
					case 4:
						temp_warn_high = atof(parsed);
						break;
					case 5:
						rh_crit_low = atof(parsed);
						break;
					case 6:
						rh_crit_high = atof(parsed);
						break;
					case 7:
						rh_warn_low = atof(parsed);
						break;
					case 8:
						rh_warn_high = atof(parsed);
						break;
					case 9:
						lux_crit = atoi(parsed);
						break;
					case 10:
						lux_warn = atoi(parsed);
						break;
					case 11:
						eco2_crit = atoi(parsed);
						break;
					case 12:
						eco2_warn = atoi(parsed);
						break;
					case 13:
						meas_t_ms = atoi(parsed);
						break;
					case 14:
						dht_t_ms = atoi(parsed);
						break;
					case 15:
						bh1750_t_ms = atoi(parsed);
						break;
					case 16:
						ccs811_t_ms = atoi(parsed);
						break;
					case 17:
						output_t_ms = atoi(parsed);
						break;
					default:
						break;
					}
				} else
					filerr = 1;
			} else
				filerr = 1;
		}
	} else
		filerr = 1;

	if (filerr) {
		LCD1602Display__set_cursor(roompi_system->root_system->actuator_display, 0, 0);
		LCD1602Display__write(roompi_system->root_system->actuator_display, 2);
		LCD1602Display__print(roompi_system->root_system->actuator_display, "I/O roompi.conf");
	}

	// si pulso boton activa measurement processing
	// si pulso este otro boton activa next display

	int button_pins[3] = { 22, 21, 30 }; // button pin nos from left to right button on the board

	// set pullup on button pins
	for (int i = 0; i < 3; i++) {
		pullUpDnControl(button_pins[i], PUD_UP);
		pinMode(button_pins[i], INPUT);
	}

	// define buttons ISRs
	void _force_meas_processing_isr() {
		measurement_flags |= FLAG_PERFORM_PROCESSING;
	}

	void _force_next_display_isr() {
		output_flags |= FLAG_NEXT_DISPLAY_INFO;
	}

	void _toggle_buzzer_isr() {
		buzzer_disabled ^= 0x1;
		if (!(measurement_flags & (FLAG_TEMP_EMERGENCY | FLAG_HUMID_EMERGENCY | FLAG_LIGHT_EMERGENCY | FLAG_CO2_EMERGENCY))) {
			BuzzerOutput__disable(roompi_system->root_system->actuator_buzzer);
		} else {
			BuzzerOutput__toggle(roompi_system->root_system->actuator_buzzer);
		}
	}

	// ISRs setup
	wiringPiISR(button_pins[0], INT_EDGE_FALLING, _force_meas_processing_isr);
	wiringPiISR(button_pins[1], INT_EDGE_FALLING, _force_next_display_isr);
	wiringPiISR(button_pins[2], INT_EDGE_FALLING, _toggle_buzzer_isr);

	tmr_startms(roompi_system->root_measurement_ctrl->timer, meas_t_ms);
	tmr_startms(roompi_system->root_system->sensor_temp_humid->timer, dht_t_ms); // fire temp humid fsm every 5 seconds
	tmr_startms(roompi_system->root_system->sensor_light->timer, bh1750_t_ms); // fire light fsm every 5 seconds
	tmr_startms(roompi_system->root_system->sensor_co2->timer, ccs811_t_ms);  // fire co2 fsm every 5 seconds

	// Output system timer
	tmr_startms(roompi_system->root_output_ctrl->timer, output_t_ms);

	measurement_flags |= FLAG_LIGHT_PENDING_MEASUREMENT;
	measurement_flags |= FLAG_TEMP_HUMID_PENDING_MEASUREMENT;
	measurement_flags |= FLAG_CO2_PENDING_MEASUREMENT;

	StatusLEDOutput__set_color(roompi_system->root_system->actuator_leds, GREEN);

	while (1) {
		fsm_fire(roompi_system->root_measurement_ctrl->fsm);
		fsm_fire(roompi_system->root_system->sensor_temp_humid->fsm);
		fsm_fire(roompi_system->root_system->sensor_light->fsm);
		fsm_fire(roompi_system->root_system->sensor_co2->fsm);
		fsm_fire(roompi_system->root_output_ctrl->fsm_buzzer);
		fsm_fire(roompi_system->root_output_ctrl->fsm_leds);
		fsm_fire(roompi_system->root_output_ctrl->fsm_info);
		fsm_fire(roompi_system->root_output_ctrl->fsm_warnings);
	}

	SystemType__destroy(roompi_system);

	return 0;
}
