#include "pasak.h"
#include <esp_now.h>
#include <stdlib.h>
#include <math.h>

unsigned long millis_espnow = 0;
unsigned long millis_servo = 0;
unsigned long millis_pasak_start;

typedef struct espnow_message_pasak {
  uint8_t type;
  int16_t motor_left;
  int16_t motor_right;
  uint8_t led;
  uint8_t servo;
} espnow_message_pasak;

espnow_message_pasak message_pasak;

uint8_t mac_pasak[] = { 0x3c, 0x71, 0xbf, 0xff, 0x87, 0x80 };

uint8_t led_state = 1;
uint8_t servo_pos = 50;
uint8_t servo_ppos = 1;   // 1 - nahore, 2 - dole malo, 3 - dole hodne

/**
 * Vykresleni obrazovky
 */
void screenPasak(){
  uint8_t cont_bar_width = 10;
  uint8_t px;
  unsigned long millis_act;
  uint16_t sec;
  uint8_t min;
  uint8_t perc;
  char buff[100];
  int16_t  x1, y1;
  uint16_t w, h;

  millis_act = millis();

  // smazani obrazovky
  canvas.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);

  // postranni bary - podklad
  canvas.fillRect(0, 0, cont_bar_width, SCREEN_HEIGHT, COLOR_DARK_GRAY);
  canvas.fillRect(SCREEN_WIDTH - cont_bar_width, 0, cont_bar_width, SCREEN_HEIGHT, COLOR_DARK_GRAY);

  // postranni bary
  if(axis_list[1].val > 0){
    px = map(axis_list[1].val, 0, 100, 0, 47);
    canvas.fillRect(0, 47 - px, cont_bar_width, px, COLOR_BLUE);
  }
  if(axis_list[1].val < 0){
    px = map(-axis_list[1].val, 0, 100, 0, 47);
    canvas.fillRect(0, 48, cont_bar_width, px, COLOR_BLUE);
  }
  if(axis_list[3].val > 0){
    px = map(axis_list[3].val, 0, 100, 0, 47);
    canvas.fillRect(SCREEN_WIDTH - cont_bar_width, 47 - px, cont_bar_width, px, COLOR_BLUE);
  }
  if(axis_list[3].val < 0){
    px = map(-axis_list[3].val, 0, 100, 0, 47);
    canvas.fillRect(SCREEN_WIDTH - cont_bar_width, 48, cont_bar_width, px, COLOR_BLUE);
  }

  // led
  canvas.fillRect(12, 0, 10, 10, led_state == 1 ? COLOR_GREEN : COLOR_DARK_GRAY);

  // servo
  canvas.fillRect(24, 0, 92, 10, COLOR_DARK_GRAY);
  px = map(servo_pos, 0, 100, 0, 92);
  canvas.fillRect(24, 0, px, 10, COLOR_BLUE);

  // postranni bary - stred
  canvas.fillRect(0, 47, cont_bar_width, 2, COLOR_RED);
  canvas.fillRect(SCREEN_WIDTH - cont_bar_width, 47, cont_bar_width, 2, COLOR_RED);

  // texty velke
  //canvas.setFont(&FreeSans9pt7b);
  canvas.setFont(&Roboto_Mono_Medium_15);

  // cas jak dlouho bezime
  sec = (millis_act - millis_pasak_start) / 1000;
  min = floor(sec / 60.0);
  sec = sec - (60 * min);
  sprintf(buff, "%u:%02u", min, sec);
  canvas.getTextBounds(buff, 0, 0, &x1, &y1, &w, &h);
  
  canvas.setCursor(cont_bar_width + 5, 31);
  canvas.setTextColor(COLOR_GRAY);
  canvas.print("Time");

  canvas.setTextColor(COLOR_WHITE);
  canvas.setCursor(SCREEN_WIDTH - cont_bar_width - 10 - w, 31);
  canvas.print(buff);

  // texty male
  canvas.setFont(&Roboto_10);
  canvas.setTextColor(COLOR_WHITE);

  // kolik jsme poslali zprav
  canvas.setCursor(20, 78);
  canvas.print(espnow_cnt_tx_ok);
  
  // procento uspesnych zprav
  strcpy(buff, "-");
  if(espnow_cnt_tx_ok > 0){
    perc = round(((double)espnow_cnt_del_ok / (double)espnow_cnt_tx_ok) * 100.0);
    sprintf(buff, "%u%%", perc);
  }
  
  canvas.getTextBounds(buff, 0, 0, &x1, &y1, &w, &h);
  canvas.setCursor(SCREEN_WIDTH - cont_bar_width - 10 - w, 78);
  canvas.print(buff);

  // graf uspesnych
  px = map(perc, 0, 100, 0, SCREEN_WIDTH - cont_bar_width * 2);

  canvas.fillRect(cont_bar_width, SCREEN_HEIGHT - 6, px, 6, COLOR_GREEN);
  canvas.fillRect(cont_bar_width + px, SCREEN_HEIGHT - 6, SCREEN_WIDTH - px - 2 * cont_bar_width, 6, COLOR_RED);

  /*
  canvas.setTextColor(COLOR_DARK_GRAY);
  canvas.setCursor(20, 92);
  canvas.print(espnow_cnt_del_ok);
  
  canvas.setTextColor(COLOR_DARK_GRAY);
  canvas.setCursor(70, 92);
  canvas.print(espnow_cnt_del_err);
  */

  displayCanvas();
}

/**
 * Smycka
 */
void appPasak(){
  unsigned long millis_act;
  unsigned long millis_redraw;
  uint16_t i;
  uint8_t can_change_servo = 1;

  // init promenne
  millis_pasak_start = millis();
  millis_redraw = millis();
  espnow_cnt_tx_ok = 0;
  espnow_cnt_del_ok = 0;
  espnow_cnt_del_err = 0;

  redraw = 1;

  while(1){
    millis_act = millis();

    // nacteni vstupu
    readAllInputs();

    // prekreslovani co vterinu
    if(millis_act - millis_redraw > 1000){
      redraw = 1;
      millis_redraw = millis_act;
    }

    if(joy_b1 == 0){
      return;
    }

    // zmenila se hodnota osy?
    for(i = 0; i < 4; i++){
      if(axis_list[i].changed){
        // pokud ano tak prekresleni
        //redraw = 1;
      }
    }

    // ovladani led
    if(axis_list[0].val > 50 && led_state == 0){
      led_state = 1;
    }
    if(axis_list[0].val < -50 && led_state == 1){
      led_state = 0;
    }

    // ovladani serva
    if(axis_list[2].val > -10 && axis_list[2].val < 10){
      can_change_servo = 1;
    }
    if(can_change_servo == 1){
      if(axis_list[2].val < -50){
        if(servo_ppos < 3) servo_ppos++;
        can_change_servo = 0;
      }
      if(axis_list[2].val > 50){
        if(servo_ppos > 1) servo_ppos--;
        can_change_servo = 0;
      }
    }

    /*
    if(millis_act - millis_servo > 50){
      if(axis_list[2].val > 50 && servo_pos < 100){
        servo_pos++;
      }
      if(axis_list[2].val < -50 && servo_pos > 0){
        servo_pos--;
      }
      millis_servo = millis_act;
    }
    */

    // prednstaveny pozice na rozsah 1-100
    if(servo_ppos == 1) servo_pos = 100;
    if(servo_ppos == 2) servo_pos = 15;
    if(servo_ppos == 3) servo_pos = 0;

    // posilani rizeni
    if(millis_act - millis_espnow > 100){
      if(espnow_sending == 0){
        message_pasak.type = 0x02;
        message_pasak.servo = servo_pos;
        message_pasak.led = led_state;
        message_pasak.motor_left = map(axis_list[1].val, -100, 100, -255, 255);
        message_pasak.motor_right = map(axis_list[3].val, -100, 100, -255, 255);

        espnow_sending = 1;
        esp_err_t result = esp_now_send(mac_pasak, (uint8_t *)&message_pasak, sizeof(message_pasak));
        
        if (result == ESP_OK) {
          espnow_cnt_tx_ok++;
        } else {
          espnow_cnt_tx_err++;
        }    

        millis_espnow = millis_act;
      }
    }

    // prekresleni obrazovky
    if(redraw == 1){
      screenPasak();
    }
  }
}