#include "espnowapp.h"

//#include <Arduino.h>
//#include <Adafruit_GFX.h>
//#include <Adafruit_SSD1351.h>

#include "main.h"

#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

extern GFXcanvas16 canvas;

/*
extern uint16_t espnow_cnt_tx_ok;
extern uint16_t espnow_cnt_tx_err;
extern uint16_t espnow_cnt_del_ok;
extern uint16_t espnow_cnt_del_err;
extern uint8_t espnow_sending;
*/

extern rxNowMsg now_rx_buff[RX_NOW_BUFF_LEN];
extern int16_t now_rx_cnt;

extern aAxis axis_list[4];
extern uint8_t redraw;
extern uint8_t joy_b1;
extern uint8_t joy_b2;

extern void readAllInputs();

// espnow zprava - struktura hlavicky
typedef struct espnow_message_sensbat_head {
  uint8_t type;
  uint8_t fw_version;
  uint8_t data_count;
} espnow_message_sensbat_head;

// espnow zprava - struktura dat
typedef struct espnow_message_sensbat_data {
  char ident[4];
  float value;
} espnow_message_sensbat_data;

espnow_message_sensbat_head sensbat_head;
espnow_message_sensbat_data sensbat_data;

/**
 * Vykresleni obrazovky
 */
void screenESPNow(){

  char str[100];
  int16_t  x1, y1;
  uint16_t w, h;
  uint16_t i;
  // prekresleni obrazovky

    canvas.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
    canvas.setFont(&FreeSans9pt7b);
    canvas.setTextColor(BLACK);

    canvas.fillRect(0, 0, SCREEN_WIDTH, 18, DBLUE);
    canvas.setCursor(4, 14);
    canvas.print("ESP-Now");

    // now_rx_cnt
    sprintf(str, "%u", now_rx_cnt);
    canvas.getTextBounds(str, 0, 60, &x1, &y1, &w, &h);
    canvas.setCursor(SCREEN_WIDTH - 4 - w, 14);
    canvas.print(str);

    canvas.setFont(&FreeSans18pt7b);
    
    // now_message.type
    sprintf(str, "%u", 44);
    canvas.setCursor(4, 55);
    canvas.setTextColor(GRAY);
    canvas.print("V");
    canvas.getTextBounds(str, 0, 60, &x1, &y1, &w, &h);
    canvas.setCursor(SCREEN_WIDTH - 4 - w, 55);
    canvas.setTextColor(WHITE);
    canvas.print(str);

    // now_message.temperature
    sprintf(str, "%.1f", 1.23);
    canvas.setCursor(4, 91);
    canvas.setTextColor(GRAY);
    canvas.print("T");
    canvas.getTextBounds(str, 0, 60, &x1, &y1, &w, &h);
    canvas.setCursor(SCREEN_WIDTH - 4 - w, 91);
    canvas.setTextColor(WHITE);
    canvas.print(str);

    // na displej
    displayCanvas();
}

/**
 * Hlavni smycka
 */
void appESPNow(){
    unsigned long millis_act;
    unsigned long millis_redraw;    
    char buff[100];
    uint16_t i;
    uint16_t j;

    redraw = 1;

    while(1){
        millis_act = millis();

        // nacteni vstupu
        readAllInputs();

        if(joy_b1 == 0){
          return;
        }

        // buffer now msg
        for(i = 0; i < RX_NOW_BUFF_LEN; i++){
          if(now_rx_buff[i].proc == 1){
            now_rx_buff[i].proc = 0;
            uartPrint("NowMSG");

            sprintf(buff, "%02X %02X %02X %02X %02X %02X / %d", now_rx_buff[i].mac[0], now_rx_buff[i].mac[1], now_rx_buff[i].mac[2], now_rx_buff[i].mac[3], now_rx_buff[i].mac[4], now_rx_buff[i].mac[5], now_rx_buff[i].len);
            uartPrint(buff);

            if(now_rx_buff[i].msg[0] = 0x03){
              memcpy(&sensbat_head, &now_rx_buff[i].msg[0], sizeof(sensbat_head));
              
              sprintf(buff, "FW: %u Data: %u", sensbat_head.fw_version, sensbat_head.data_count);
              uartPrint(buff);

              for(j = 0; j < sensbat_head.data_count; j++){
                memcpy(&sensbat_data, &now_rx_buff[i].msg[3 + j * sizeof(sensbat_data)], sizeof(sensbat_data));
                sprintf(buff, "%s = %f", sensbat_data.ident, sensbat_data.value);
                uartPrint(buff);
              }
            }

            redraw = 1;
            break;
          }
        }

        // prekreslovani co vterinu
        if(millis_act - millis_redraw > 1000){
            redraw = 1;
            millis_redraw = millis_act;
        }

        // prekreslime
        if(redraw){
            screenESPNow();
        }
    }

}