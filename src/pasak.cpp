#include "pasak.h"
#include <esp_now.h>

unsigned long millis_espnow = 0;
unsigned long millis_servo = 0;

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

/**
 * Vykresleni obrazovky
 */
void screenPasak(){
  uint8_t cont_bar_width = 10;
  uint8_t px;

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

  // texty
  //canvas.setFont(&FreeSans9pt7b);
  canvas.setFont(&Roboto_10);
  
  canvas.setTextColor(WHITE);
  canvas.setCursor(20, 78);
  canvas.print(espnow_cnt_tx_ok);
  
  //canvas.setCursor(16, 28);
  //canvas.print(espnow_cnt_tx_err);

  canvas.setTextColor(COLOR_GREEN);
  canvas.setCursor(20, 92);
  canvas.print(espnow_cnt_del_ok);
  
  canvas.setTextColor(COLOR_RED);
  canvas.setCursor(70, 92);
  canvas.print(espnow_cnt_del_err);

  displayCanvas();
}

/**
 * Smycka
 */
void loopPasak(){
  unsigned long millis_act;

  millis_act = millis();

  // ovladani led
  if(axis_list[0].val > 50 && led_state == 0){
    led_state = 1;
  }
  if(axis_list[0].val < -50 && led_state == 1){
    led_state = 0;
  }

  // ovladani serva
  if(millis_act - millis_servo > 50){
    if(axis_list[2].val > 50 && servo_pos < 100){
      servo_pos++;
    }
    if(axis_list[2].val < -50 && servo_pos > 0){
      servo_pos--;
    }
    millis_servo = millis_act;
  }

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
}