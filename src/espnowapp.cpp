#include "espnowapp.h"
#include "main.h"

#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

extern GFXcanvas16 canvas;

extern rxNowMsg now_rx_buff[RX_NOW_BUFF_LEN];
extern int16_t now_rx_cnt;

extern aAxis axis_list[4];
extern uint8_t redraw;
extern uint8_t joy_b1;
extern uint8_t joy_b2;

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

uint8_t act_screen;

char str_last_mac[20];
char str_length[10];
char str_before[10];
unsigned long tm_last;

char str_ident[6][10];
char str_val[6][10];

void resetStrVals();

/**
 * Vykresleni obrazovky
 */
void screenESPNow(){

  char str[100];
  int16_t  x1, y1;
  uint16_t w, h;
  uint16_t i;
  uint8_t idx;

  // prekresleni obrazovky
  canvas.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
  canvas.setFont(&FreeSans9pt7b);
  canvas.setTextColor(BLACK);

  // hlavicka
  canvas.fillRect(0, 0, SCREEN_WIDTH, 18, DBLUE);
  canvas.setCursor(4, 14);
  canvas.print("ESPNow");

  sprintf(str, "%u/4", act_screen);
  canvas.getTextBounds(str, 0, 60, &x1, &y1, &w, &h);
  canvas.setCursor(SCREEN_WIDTH - 6 - w, 14);
  canvas.print(str);

  // uvodka
  if(act_screen == 1){
    sprintf(str, "%u", now_rx_cnt);
    canvasTextLine("RX count", 1, ALIGN_LEFT);
    canvasTextLine(str, 1, ALIGN_RIGHT);

    canvasTextLine(str_last_mac, 3, ALIGN_LEFT);

    canvasTextLine(str_before, 2, ALIGN_LEFT);
    canvasTextLine(str_length, 2, ALIGN_RIGHT);
  }

  // stranky s hodnotama
  if(act_screen > 1){
    idx = (act_screen - 2) * 2;

    // prvni hodnota
    canvas.setFont(&FreeSans9pt7b);
    canvas.setCursor(4, 55);
    canvas.setTextColor(GRAY);
    canvas.print(str_ident[idx]);

    canvas.setFont(&FreeSans18pt7b);
    canvas.getTextBounds(str_val[idx], 0, 60, &x1, &y1, &w, &h);
    canvas.setCursor(SCREEN_WIDTH - 4 - w, 55);
    canvas.setTextColor(WHITE);
    canvas.print(str_val[idx]);

    // druha hodnota
    canvas.setFont(&FreeSans9pt7b);
    canvas.setCursor(4, 91);
    canvas.setTextColor(GRAY);
    canvas.print(str_ident[idx + 1]);

    canvas.setFont(&FreeSans18pt7b);
    canvas.getTextBounds(str_val[idx + 1], 0, 60, &x1, &y1, &w, &h);
    canvas.setCursor(SCREEN_WIDTH - 4 - w, 91);
    canvas.setTextColor(WHITE);
    canvas.print(str_val[idx + 1]);
  }

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
    act_screen = 1;

    strcpy(str_last_mac, "-");
    strcpy(str_length, "-B");
    strcpy(str_before, "-s");
    tm_last = 0;

    while(1){
        millis_act = millis();

        // nacteni vstupu
        readAllInputs();

        if(joy_b1 == 0){
          return;
        }

        if(axis_list[2].val > 50){
          if(act_screen < 4) act_screen++;
          redraw = 1;
        }
        if(axis_list[2].val < -50){
          if(act_screen > 1) act_screen--;
          redraw = 1;
        }

        // buffer now msg
        for(i = 0; i < RX_NOW_BUFF_LEN; i++){
          if(now_rx_buff[i].proc == 1){
            now_rx_buff[i].proc = 0;
            uartPrint("NowMSG");

            sprintf(str_last_mac, "%02X%02X%02X%02X%02X%02X", now_rx_buff[i].mac[0], now_rx_buff[i].mac[1], now_rx_buff[i].mac[2], now_rx_buff[i].mac[3], now_rx_buff[i].mac[4], now_rx_buff[i].mac[5]);
            sprintf(str_length, "%uB", now_rx_buff[i].len);
            tm_last = now_rx_buff[i].tm;

            if(now_rx_buff[i].msg[0] = 0x03){
              memcpy(&sensbat_head, &now_rx_buff[i].msg[0], sizeof(sensbat_head));
              
              sprintf(buff, "FW: %u Data: %u", sensbat_head.fw_version, sensbat_head.data_count);
              uartPrint(buff);

              resetStrVals();

              for(j = 0; j < sensbat_head.data_count; j++){
                memcpy(&sensbat_data, &now_rx_buff[i].msg[3 + j * sizeof(sensbat_data)], sizeof(sensbat_data));
                sprintf(buff, "%s = %f", sensbat_data.ident, sensbat_data.value);
                uartPrint(buff);

                // na displej
                if(j < 6){
                  sprintf(str_ident[j], "%s", sensbat_data.ident);
                  if(sensbat_data.value < 10 && sensbat_data.value > -10){
                    sprintf(str_val[j], "%.2f", sensbat_data.value);
                  } else {
                    sprintf(str_val[j], "%.1f", sensbat_data.value);
                  }
                }
              }
            }

            redraw = 1;
            break;
          }
        }

        // update before stringu
        if(tm_last != 0){
          sprintf(str_before, "%.0fs", round((millis() - tm_last) / 1000.0));
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

/**
 * Vynulovani hodnot
 */
void resetStrVals(){
  uint8_t i;
  for(i = 0; i < 6; i++){
    strcpy(str_ident[i], "-");
    strcpy(str_val[i], "-");
  }
}