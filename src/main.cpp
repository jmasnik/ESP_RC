#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <SPI.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <FastLED.h>

#include "images.h"
#include "Roboto_10.h"
#include "main.h"
#include "pasak.h"

#define NUM_LEDS 1
#define LED_PIN 48

#define PIN_JOY1 4
#define PIN_JOY2 5
#define PIN_JOY3 6
#define PIN_JOY4 7
#define PIN_JOY_B1 15
#define PIN_JOY_B2 16
#define ADC_READ_CNT 3

// The SSD1351 is connected like this (plus VCC plus GND)
#define SCLK_PIN 12
#define MOSI_PIN 11
#define DC_PIN   17
#define CS_PIN   10
#define RST_PIN  18

#define LOCAL_UDP_PORT 4210

CRGB leds[NUM_LEDS];

Adafruit_SSD1351 tft = Adafruit_SSD1351(SCREEN_WIDTH, SCREEN_HEIGHT, CS_PIN, DC_PIN, MOSI_PIN, SCLK_PIN, RST_PIN); 

// UDP
WiFiUDP Udp;

uint8_t redraw;
unsigned long millis_redraw;

extern uint8_t mac_pasak[6];

typedef struct espnow_message_a {
  uint8_t type;
  float temperature;
} espnow_message_a;

espnow_message_a now_message;

uint16_t aval_joy1;
uint16_t aval_joy2;
uint16_t aval_joy3;
uint16_t aval_joy4;
uint8_t joy_b1;
uint8_t joy_b2;

typedef enum {
  SCREEN_JOYSTICK,
  SCREEN_ESPNOW,
  SCREEN_PASAK,
  SCREEN_HOME,
  SCREEN_INFO,
  SCREEN_IMAGE,
  SCREEN_MIKUL
} Screen;

Screen screen;

struct mMenuItem {
  char name[20];
  uint8_t ident;
};

struct mMenu {
  mMenuItem item_list[10];
  uint8_t item_count;
  uint8_t sel_item;
  uint8_t shift;
  uint8_t per_page;
};

mMenu menu_home;

void addMenuItemStr(mMenu *menu, const char *str, uint8_t ident);

void menuDraw(mMenu *menu);
void menuUp(mMenu *menu);
void menuDown(mMenu *menu);
void menuInit(mMenu *menu);

uint16_t now_rx_cnt;

typedef struct struct_message {
    char a[32];
    int b;
    float c;
    bool d;
} struct_message;

// Create a struct_message called myData
struct_message myData;

char mac_address[100];

// osy ktery mam
aAxis axis_list[4];

// promenny k posilani pres espnow
uint8_t espnow_sending;
uint16_t espnow_cnt_tx_ok;
uint16_t espnow_cnt_tx_err;
uint16_t espnow_cnt_del_ok;
uint16_t espnow_cnt_del_err;

GFXcanvas16 canvas(SCREEN_WIDTH, SCREEN_HEIGHT);

void screenInfo();
void screenValue();
void screenHome();

void screenImage();
void screenJoy();

void readAxis(aAxis *axis);
void readAllInputs();

void drawAxis(uint16_t center_x, uint16_t center_y, aAxis *x, aAxis *y, uint16_t color);

void initWifi();
void initESPNow();
void initUDP();

void readMacAddress();

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

/**
 * Callback po prijmu espnow
 */
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  
  now_rx_cnt++;

  if(len == sizeof(now_message) && incomingData[0] == 0x01){
    memcpy(&now_message, incomingData, sizeof(now_message));
    redraw = 1;
  }

  // bliknuti LED
  leds[0] = CRGB::Purple;
  FastLED.show();
  delay(100);
  leds[0] = CRGB::Black;
  FastLED.show();
}

/**
 * Callback po poslani espnow
 */
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // podarilo se nebo ne?
  if(status == ESP_NOW_SEND_SUCCESS){
    espnow_cnt_del_ok++;
  } else {
    espnow_cnt_del_err++;
  }

  // uz neposilam a muzu posilat znova
  espnow_sending = 0;
}

/**
 * Setup
 */
void setup() {
  now_rx_cnt = 0;
  redraw = 1;
  millis_redraw = 0;
  joy_b1 = 1;
  joy_b2 = 1;

  espnow_sending = 0;
  espnow_cnt_tx_ok = 0;
  espnow_cnt_tx_err = 0;
  espnow_cnt_del_ok = 0;
  espnow_cnt_del_err = 0;

  screen = SCREEN_HOME;

  // levej X
  axis_list[0].pin = PIN_JOY3;
  axis_list[0].val = 0;
  axis_list[0].val_prev = 0;
  axis_list[0].changed = 0;
  axis_list[0].zero_from = 2124;
  axis_list[0].zero_to = 2134;

  // levej Y
  axis_list[1].pin = PIN_JOY4;
  axis_list[1].val = 0;
  axis_list[1].val_prev = 0;
  axis_list[1].changed = 0;
  axis_list[1].zero_from = 2070;
  axis_list[1].zero_to = 2090;

  // pravej X
  axis_list[2].pin = PIN_JOY1;
  axis_list[2].val = 0;
  axis_list[2].val_prev = 0;
  axis_list[2].changed = 0;
  axis_list[2].zero_from = 2124;
  axis_list[2].zero_to = 2134;

  // pravej Y
  axis_list[3].pin = PIN_JOY2;
  axis_list[3].val = 0;
  axis_list[3].val_prev = 0;
  axis_list[3].changed = 0;  
  axis_list[3].zero_from = 2070;
  axis_list[3].zero_to = 2080;

  pinMode(PIN_JOY_B1, INPUT_PULLUP);
  pinMode(PIN_JOY_B2, INPUT_PULLUP);

  // seriak
  Serial.begin(115200);  

  initWifi();
  initESPNow();
  initUDP();

  readMacAddress();

  FastLED.addLeds<NEOPIXEL, LED_PIN>(leds, NUM_LEDS);

  //ESP32 Board MAC Address: cc:8d:a2:ed:b0:84

  log_d("Total heap: %d", ESP.getHeapSize());
  log_d("Free heap: %d", ESP.getFreeHeap());
  log_d("Total PSRAM: %d", ESP.getPsramSize());
  log_d("Free PSRAM: %d", ESP.getFreePsram());

  tft.begin();

  tft.fillRect(0, 0, 128, 96, BLACK);

  menuInit(&menu_home);
  addMenuItemStr(&menu_home, "Pasak", SCREEN_PASAK);
  addMenuItemStr(&menu_home, "Mikul", SCREEN_MIKUL);
  addMenuItemStr(&menu_home, "ESPNow", SCREEN_ESPNOW);
  addMenuItemStr(&menu_home, "Joystick", SCREEN_JOYSTICK);
  addMenuItemStr(&menu_home, "Info", SCREEN_INFO);
  addMenuItemStr(&menu_home, "Image", SCREEN_IMAGE);

}

/**
 * Inicializace wifi
 */
void initWifi(){
  // TODO: udelat scan a hodit se na channel ktery neni obsazeny
  //WiFi.mode(WIFI_STA);
  WiFi.mode(WIFI_AP_STA);

  WiFi.softAPsetHostname("mcontroller");
  WiFi.softAP("mController", "heslo123", 10);
  
  WiFi.begin();
}

/**
 * Inicializace ESP Now
 */
void initESPNow(){
  esp_now_peer_info_t peerInfo;

  // inicializace
  if (esp_now_init() != ESP_OK) {
    log_d("Error initializing ESP-NOW");
    return;
  }
  
  // registrace prijimaciho callbacku
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));

  // registrace vysilaciho callbacku
  esp_now_register_send_cb(OnDataSent);

  // registrace peer
  memcpy(peerInfo.peer_addr, mac_pasak, 6);
  peerInfo.channel = 10;
  peerInfo.encrypt = false;
     
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
  }  
}

/**
 * Hlavni smycka
 */
void loop() {
  screenHome();
}

/**
 * Nacteni analogove osy
 */
void readAxis(aAxis *axis){
  uint32_t aval;
  uint8_t i;

  aval = 0;
  for(i = 0; i < ADC_READ_CNT; i++){
    aval += 4095 - analogRead(axis->pin);
  }

  axis->val_a = aval / ADC_READ_CNT;

  axis->val_prev = axis->val;
  axis->val = 0;

  if(axis->val_a <= axis->zero_from){
    axis->val = map(axis->val_a, axis->zero_from, 0, 0, -100);
  }

  if(axis->val_a >= axis->zero_to){
    axis->val = map(axis->val_a, axis->zero_to, 4095, 0, 100);
  }  

  if(axis->val != axis->val_prev){
    axis->changed = 1;
  } else {
    axis->changed = 0;
  }
}

/**
 * Nacteni MAC adresy
 */
void readMacAddress(){
  strcpy(mac_address, "");
  uint8_t baseMac[6];
  esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
  if (ret == ESP_OK) {
    sprintf(mac_address, "%02x%02x%02x%02x%02x%02x", baseMac[0], baseMac[1], baseMac[2],baseMac[3], baseMac[4], baseMac[5]);
    log_d("%s", mac_address);
  } else {
    log_d("Failed to read MAC address");
  }
}

/**
 * Testovani joysticku
 */
void screenJoy(){
  uint8_t w;
  uint16_t i;

  while(1){
    readAllInputs();
    
    if(joy_b1 == 0){
      return;
    }

    // zmenila se hodnota osy?
    for(i = 0; i < 4; i++){
      if(axis_list[i].changed){
        // pokud ano tak prekresleni
        redraw = 1;
      }
    }

    if(redraw == 1){
      canvas.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
      canvas.setFont(&FreeSans9pt7b);
      canvas.setTextColor(WHITE);

      canvas.fillRect(0, 0, SCREEN_WIDTH, 18, DBLUE);
      canvas.setCursor(4, 14);
      canvas.print("Joystick");


      canvas.setFont(&Roboto_10);

      /*
      canvas.setCursor(4, 36);
      canvas.print(axis_list[0].val);

      canvas.setCursor(4, 56);
      canvas.print(axis_list[1].val);

      */

      drawAxis(32, 49, &axis_list[0], &axis_list[1], DBLUE);
      drawAxis(96, 49, &axis_list[2], &axis_list[3], DBLUE);

      /*
      canvas.setCursor(64, 36);
      canvas.print(axis_list[2].val);

      canvas.setCursor(64, 56);
      canvas.print(axis_list[3].val);

      canvas.setCursor(4, 76);
      canvas.print(joy_b1);

      canvas.setCursor(64, 76);
      canvas.print(axis_list[2].val_a);
      canvas.setCursor(96, 76);
      canvas.print(axis_list[3].val_a);
      */
      
      displayCanvas();

      /*
      leds[0].setHSV(
        map(axis_list[0].val, -100, 100, 0, 255),
        255,
        80
      );
      FastLED.show();
      */
    }
  }

}

/**
 * Vykresleni krizoveho ovladace na joy obrazovce
 */
void drawAxis(uint16_t center_x, uint16_t center_y, aAxis *x, aAxis *y, uint16_t color){
  uint8_t w;

  canvas.drawCircle(center_x, center_y, 26, GRAY);
  canvas.fillCircle(center_x, center_y, 2, color);

  if(x->val > 0){
    w = map(x->val, 0, 100, 0, 25);
    canvas.fillRect(center_x, center_y - 2, w, 5, color);
  }
  if(x->val < 0){
    w = map(x->val, 0, -100, 0, 25);
    canvas.fillRect(center_x - w, center_y - 2, w, 5, color);
  }

  if(y->val > 0){
    w = map(y->val, 0, 100, 0, 25);
    canvas.fillRect(center_x - 2, center_y - w, 5, w, color);
  }
  if(y->val < 0){
    w = map(y->val, 0, -100, 0, 25);
    canvas.fillRect(center_x - 2, center_y, 5, w, color);
  }  
}

/**
 * Prenese canvas na displej
 */
void displayCanvas(){
  // zobrazime na displeji
  tft.drawRGBBitmap(0, 0, canvas.getBuffer(), canvas.width(), canvas.height());

  // oznacime jako prekreslene
  redraw = 0;
}

/**
 * Nejaka info obrazovka
 */
void screenInfo(){
  canvas.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
  canvas.setFont(&FreeSans9pt7b);
  canvas.setTextColor(WHITE);

  canvas.fillRect(0, 0, SCREEN_WIDTH, 18, DBLUE);
  canvas.setCursor(4, 14);
  canvas.print("Info");
  
  canvas.setCursor(4, 36);
  canvas.print(mac_address);

  displayCanvas();

  while(1){
    readAllInputs();
    if(joy_b1 == 0){
      return;
    }
  }  
}

/**
 * Obrazovka s obrazkem
 */
void screenImage(){
  // vykreslime obrazek
  tft.drawRGBBitmap(0, 0, img_motorka, 128, 96);

  while(1){
    readAllInputs();
    if(joy_b1 == 0){
      return;
    }
  }
}

/**
 * Obrazovka s hodnotou
 */
void screenValue(){
  char str[10];
  int16_t  x1, y1;
  uint16_t w, h;

  redraw = 1;

  while(1){
    // vstupy
    readAllInputs();

    // zpet na uvod
    if(joy_b1 == 0){
      return;
    }

    // prekresleni obrazovky
    if(redraw == 1){
      canvas.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
      canvas.setFont(&FreeSans9pt7b);
      canvas.setTextColor(BLACK);

      canvas.fillRect(0, 0, SCREEN_WIDTH, 18, DBLUE);
      canvas.setCursor(4, 14);
      canvas.print("ESP-Now");

      sprintf(str, "%u", now_rx_cnt);
      canvas.getTextBounds(str, 0, 60, &x1, &y1, &w, &h);
      canvas.setCursor(SCREEN_WIDTH - 4 - w, 14);
      canvas.print(str);

      canvas.setFont(&FreeSans18pt7b);
      
      sprintf(str, "%u", now_message.type);
      canvas.setCursor(4, 55);
      canvas.setTextColor(GRAY);
      canvas.print("V");
      canvas.getTextBounds(str, 0, 60, &x1, &y1, &w, &h);
      canvas.setCursor(SCREEN_WIDTH - 4 - w, 55);
      canvas.setTextColor(WHITE);
      canvas.print(str);

      sprintf(str, "%.1f", now_message.temperature);
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
  }
}

/**
 * Pridani polozky do menu
 */
void addMenuItemStr(mMenu *menu, const char *str, uint8_t ident){
  menu->item_list[menu->item_count].ident = ident;
  strcpy(menu->item_list[menu->item_count].name, str);
  menu->item_count++;
}

/**
 * Vykresleni menu
 */
void menuDraw(mMenu *menu){
  uint8_t i;
  uint8_t see_from;
  uint8_t see_to;

  // font a barva pisma
  canvas.setFont(&FreeSans9pt7b);
  canvas.setTextColor(WHITE);

  // co ted vidim
  see_from = menu->shift;
  see_to = menu->shift + menu->per_page - 1;

  // nemusim to soupnout?
  if(menu->sel_item > see_to){
    menu->shift += menu->sel_item - see_to;
  }
  if(menu->sel_item < see_from){
    menu->shift -= see_from - menu->sel_item;
  }

  // podbarveni zvoleneho
  canvas.fillRect(0, (menu->sel_item - menu->shift) * 24, SCREEN_WIDTH, 24, COLOR_DARK_GRAY);

  // textiky
  for(i = 0; i < menu->per_page; i++){
    canvas.setCursor(4, 17 + (i * 24));
    canvas.print(menu->item_list[i + menu->shift].name);
  }
}

/**
 * Uvodni obrazovka
 */
void screenHome(){
  unsigned long millis_act;
  unsigned long millis_last_input;
  char incomingPacket[255];

  millis_last_input = 0;

  while(1){

    int packetSize = Udp.parsePacket();

    if (packetSize){
      // receive incoming UDP packets
      log_d("Received %d bytes from %s, port %d", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());

      int len = Udp.read(incomingPacket, 255);
      if (len > 0){
        incomingPacket[len] = '\0';

        log_d("UDP packet contents: %s", incomingPacket);

          Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
          Udp.print("zpet");
          Udp.endPacket();
      }      
    }

    millis_act = millis();
    readAllInputs();

    // nebudem to ovladat az tak rychle
    if(millis_act - millis_last_input > 250){
      if(axis_list[3].val < -30){
        menuDown(&menu_home);
        redraw = 1;
        millis_last_input = millis_act;
      }
      if(axis_list[3].val > 30){
        menuUp(&menu_home);
        redraw = 1;
        millis_last_input = millis_act;
      }
    }

    if(joy_b2 == 0){
      if((Screen)menu_home.item_list[menu_home.sel_item].ident == SCREEN_IMAGE) screenImage();
      if((Screen)menu_home.item_list[menu_home.sel_item].ident == SCREEN_INFO) screenInfo();
      if((Screen)menu_home.item_list[menu_home.sel_item].ident == SCREEN_ESPNOW) screenValue();
      if((Screen)menu_home.item_list[menu_home.sel_item].ident == SCREEN_JOYSTICK) screenJoy();
      if((Screen)menu_home.item_list[menu_home.sel_item].ident == SCREEN_PASAK) appPasak();
      redraw = 1;
    }

    // prekresleni obrazovky
    if(redraw == 1){
      canvas.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
      menuDraw(&menu_home);
      displayCanvas();
    }
  }
}

/**
 * Nacte vsechny vstupy
 */
void readAllInputs(){
  uint16_t i;

  // nacitani analogovych os
  for(i = 0; i < 4; i++){
    readAxis(&axis_list[i]);
    //if(axis_list[i].changed){
    //  redraw = 1;
    //}
  }

  // tlacitka
  joy_b1 = digitalRead(PIN_JOY_B1);
  joy_b2 = digitalRead(PIN_JOY_B2);
}

/**
 * Posun v menu nahoru
 */
void menuUp(mMenu *menu){
  if(menu->sel_item > 0){
    menu->sel_item--;
  }
}

/**
 * Posun v menu dolu
 */
void menuDown(mMenu *menu){
  if(menu->sel_item < menu->item_count - 1){
    menu->sel_item++;
  }
}

/**
 * Inicializace menu
 */
void menuInit(mMenu *menu){
  menu->item_count = 0;
  menu->per_page = 4;
  menu->shift = 0;
  menu->sel_item = 0;
}

/**
 * Unicializace prijmu UDP
 */
void initUDP(){
  // UDP
  Udp.begin(LOCAL_UDP_PORT);
  //WiFi.localIP()
  log_d("Now listening at IP %s, UDP port %d\n", WiFi.softAPIP().toString().c_str(), LOCAL_UDP_PORT);  
}