#ifndef PASAK_H
#define PASAK_H

#include "main.h"
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include "Roboto_10.h"

extern GFXcanvas16 canvas;

extern uint16_t espnow_cnt_tx_ok;
extern uint16_t espnow_cnt_tx_err;
extern uint16_t espnow_cnt_del_ok;
extern uint16_t espnow_cnt_del_err;

extern aAxis axis_list[4];

void screenPasak();
void loopPasak();

#endif