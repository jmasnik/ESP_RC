#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 96

// Color definitions
#define	BLACK           0x0000
#define	BLUE            0x001F

#define CYAN            0x07FF
#define MAGENTA         0xF81F
#define YELLOW          0xFFE0  
#define WHITE           0xFFFF
#define	DBLUE           0x0A58
#define	GRAY            0x9CD3

#define	COLOR_DARK_GRAY 0x2104
#define	COLOR_RED       0xF800
#define	COLOR_GREEN     0x07E0
#define	COLOR_BLUE      0x0A58
#define COLOR_WHITE     0xFFFF
#define	COLOR_GRAY      0x9CD3

#define RX_NOW_BUFF_LEN 10

#define ALIGN_LEFT 0
#define ALIGN_RIGHT 1

// analogova osa
typedef struct aAxis {
  uint8_t pin;            // pin
  int8_t val;             // hodnota -100 az 100
  int8_t val_prev;        // predchozi hodnota
  uint16_t zero_from;     // nula od
  uint16_t zero_to;       // nula do
  uint8_t changed;        // doslo od minula ka zmene
  uint16_t val_a;         // neprepocitavana hodnota
} aAxis;

typedef struct rxNowMsg {
  uint8_t mac[6];
  int len;
  uint8_t msg[512];
  unsigned long tm;
  uint8_t proc;
} rxNowMsg;

void canvasTextLine(const char *str, uint8_t line, uint8_t align);
void displayCanvas();
void uartPrint(const char *str);
void readAllInputs();

#endif