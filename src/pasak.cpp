#include "pasak.h"

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

}