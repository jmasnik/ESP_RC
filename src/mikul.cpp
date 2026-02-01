#include "mikul.h"
#include "main.h"

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <WiFiUdp.h>

extern GFXcanvas16 canvas;

extern WiFiUDP Udp;
extern Adafruit_SSD1351 tft;

extern uint8_t joy_b1;
extern uint8_t joy_b2;

extern aAxis axis_list[4];

/**
 * Hlavni funkce
 */
void appMikul()
{
    char incomingPacket[1500];
    uint16_t i;
    uint8_t image[4000];
    uint8_t y;
    uint16_t ii;

    initMikul();

    while(1){
        int packetSize = Udp.parsePacket();

        if (packetSize){
        // receive incoming UDP packets
        //log_d("Received %d bytes from %s, port %d", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());

        int len = Udp.read(incomingPacket, packetSize);
        if (len > 0){
            //incomingPacket[len] = '\0';
            //log_d("UDP packet contents: %02x %02x", incomingPacket[0], incomingPacket[1]);

            if(incomingPacket[0] == 0xF0){
            ii = 0;
            for(i = 2; i < len; i = i + 2){
                image[ii] = incomingPacket[i+1];
                image[ii + 1] = incomingPacket[i];
                ii = ii + 2;
            }
            
            y = 16 + incomingPacket[1] * 7;
            
            tft.drawRGBBitmap(16, y, (uint16_t*)image, 96, 7);
            }

        }      
        }

        readAllInputs();
        
        // navrat zpet na intro
        if(joy_b1 == 0){
            return;
        }
    }
}

/**
 * Inicializace
 */
void initMikul(){
    canvas.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
    displayCanvas();
}