/*
 * outputctrl.h
 *
 *  Created on: 15 mar. 2021
 *      Author: Victoria M. Gullon and Marcos Gomez
 */

#ifndef CONTROLLERS_OUTPUTCTRL_H_
#define CONTROLLERS_OUTPUTCTRL_H_

#include "../libs/fsm.h"
#include "../libs/timerlib.h"
#include "../libs/systemlib.h"

#define FLAG_NEXT_DISPLAY_INFO 0x01
#define FLAG_NEXT_DISPLAY_WARNING 0x02

#define OUTPUT_LOCK 0

typedef struct {
	fsm_t *fsm_buzzer; // FSM buzzer
	fsm_t *fsm_leds;
	fsm_t *fsm_info;
	fsm_t *fsm_warnings;
	tmr_t *timer; // timer that goberns a flag used by the FSMs (5 s periodic)
} OutputCtrl;

OutputCtrl* OutputCtrl__setup(SystemContext* this_system);
void OutputCtrl__destroy(OutputCtrl *this);


#endif /* CONTROLLERS_OUTPUTCTRL_H_ */
