/*
 * main.c
 *
 *  Created on: 9 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#include <stdio.h>
#include <wiringPi.h>
#include <time.h>

#include "actuators/lcd1602.h"
#include "sensors/dht11.h"
#include "actuators/buzzer.h"
#include "actuators/statusLed.h"
#include "sensors/bh1750.h"

int debug_prueba(int argc, char **argv) {

	wiringPiSetup();
	// int id, int rs, int rw, int enable,	int fourbitmode, int d0, int d1, int d2, int d3, int d4, int d5, int d6, int d7
	LCD1602Display *midisplay = LCD1602Display__create(0, 15, 255, 16, 1, 10,
			11, 31, 26, 1, 4, 5, 6);

	DHT11Sensor *misensor = DHT11Sensor__create(1, 29);

	BuzzerOutput *mibuzzer = BuzzerOutput__create(2, 26);
	int color_pins[] = { 0b00000011, 0b00011100, 0b11100000 };
	// int id, int series_ic_nr, int clock_pin, int serial_data_pin, int latch_pin, int led_color_flags[3]
	StatusLEDOutput *misleds = StatusLEDOutput__create(3, 1, 25, 24, 23,
			color_pins);
	StatusLEDOutput__set_all_high(misleds);
	//int prueba[] = {0b11100000};
	//StatusLEDOutput__set_color(misleds, RED);

	BH1750Sensor *milux = BH1750Sensor__create(4, 0x23, CONTINUOUS_H_RES);
	LCD1602Display__begin(midisplay, 16, 2, 0);

	int clock[8] = { 0x1F, 0x11, 0x0A, 0x04, 0x0E, 0x1F, 0x1F, 0x00 };
	int good[8] = { 0x00, 0x0A, 0x0A, 0x0A, 0x00, 0x11, 0x0E, 0x00 }; // all good
	int general_bad[8] = { 0x00, 0x0A, 0x0A, 0x0A, 0x00, 0x0E, 0x11, 0x00 }; // bad
	int air[8] = { 0x00, 0x0C, 0x05, 0x1B, 0x14, 0x06, 0x00, 0x00 }; // Co2
	int temp[8] = { 0x04, 0x0A, 0x0A, 0x0A, 0x0A, 0x11, 0x13, 0x0E }; // temp
	int humid[8] = { 0x00, 0x04, 0x0A, 0x11, 0x11, 0x1F, 0x0E, 0x00 }; // humid
	int noise[8] = { 0x01, 0x03, 0x07, 0x1F, 0x1F, 0x07, 0x03, 0x01 }; // dBs
	int lux[8] = { 0x04, 0x15, 0x0E, 0x1B, 0x0E, 0x15, 0x04, 0x00 }; // cd.

	LCD1602Display__create_char(midisplay, 0, clock);
	LCD1602Display__create_char(midisplay, 1, good);
	LCD1602Display__create_char(midisplay, 2, general_bad);
	LCD1602Display__create_char(midisplay, 3, air);
	LCD1602Display__create_char(midisplay, 4, temp);
	LCD1602Display__create_char(midisplay, 5, humid);
	LCD1602Display__create_char(midisplay, 6, noise);
	LCD1602Display__create_char(midisplay, 7, lux);

	LCD1602Display__clear(midisplay);
	LCD1602Display__set_cursor(midisplay, 16, 0);
	char texto[16] = "roomPi **** v0.1";
	for (int i = 0; i < 16; ++i) {
		if (texto[i] == '\0')
			break;
		LCD1602Display__write(midisplay, texto[i]);
		delay(150);
		LCD1602Display__scroll_display_left(midisplay);
	}

	delay(1000);
	LCD1602Display__set_cursor(midisplay, 16, 1);
	LCD1602Display__write(midisplay, 0);
	LCD1602Display__print(midisplay, " Iniciando");
	delay(5000);

	LCD1602Display__set_cursor(midisplay, 0, 0);
	LCD1602Display__write(midisplay, 7);
	LCD1602Display__print(midisplay, " MUY POCA LUZ");

	for (int i = 0; i < 16; ++i) {
		LCD1602Display__scroll_display_right(midisplay);
	}

	while (1) {
		float mitemperatura, mihumedad;
		int err = DHT11Sensor__perform_measurement(misensor);

		while (err != 0) {
			err = DHT11Sensor__perform_measurement(misensor);
			switch (err) {
			case 2:
				printf("Error en el checksum, repitiendo medida...\n");
				delay(1000);
				break;
			case 0:
				break;
			default:
				return 1;
				break;
			}
		}
		printf("Lectura correcta, imprimiendo valores del sensor %d\n",
				misensor->id);
		mitemperatura = misensor->t_value;
		mihumedad = misensor->rh_value;
		// Humedad
		LCD1602Display__set_cursor(midisplay, 0, 1);
		LCD1602Display__print(midisplay, "                ");
		LCD1602Display__set_cursor(midisplay, 0, 1);
		LCD1602Display__print(midisplay, "humedad: %.1f %%", mihumedad);
		BuzzerOutput__disable(mibuzzer);
		StatusLEDOutput__set_color(misleds, GREEN);
		delay(5000);
		// Temperatura
		LCD1602Display__set_cursor(midisplay, 0, 1);
		LCD1602Display__print(midisplay, "                ");
		LCD1602Display__set_cursor(midisplay, 0, 1);
		LCD1602Display__print(midisplay, "temp: %.1f ", mitemperatura);
		int simbolito = 0b11011111;
		LCD1602Display__write(midisplay, simbolito);
		LCD1602Display__print(midisplay, "C");
		BuzzerOutput__disable(mibuzzer);
		StatusLEDOutput__set_color(misleds, YELLOW);
		delay(5000);
		// Luz
		BH1750Sensor__perform_measurement(milux);
		LCD1602Display__set_cursor(midisplay, 0, 1);
		LCD1602Display__print(midisplay, "                ");
		LCD1602Display__set_cursor(midisplay, 0, 1);
		LCD1602Display__print(midisplay, "Lux: %d", BH1750Sensor__lux_value(milux));
		BuzzerOutput__disable(mibuzzer);
		StatusLEDOutput__set_color(misleds, YELLOW);
		delay(5000);
		// Hora y fecha
		time_t rawtime;
		struct tm *timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		LCD1602Display__set_cursor(midisplay, 0, 1);
		LCD1602Display__print(midisplay, "                ");
		LCD1602Display__set_cursor(midisplay, 0, 1);
		LCD1602Display__print(midisplay, "%02d:%02d %02d-%02d-%d",
				timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_mday,
				1 + timeinfo->tm_mon, 1900 + timeinfo->tm_year);
		StatusLEDOutput__set_color(misleds, RED);
		BuzzerOutput__enable(mibuzzer);
		delay(5000);
	}

	printf("fin\n");

}
