#include "mikul.h"
#include "main.h"

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1351.h>
#include <WiFiUdp.h>

#define TO_MIKUL_INTERVAL 40       // 25 Hz

extern GFXcanvas16 canvas;

extern WiFiUDP Udp;
extern Adafruit_SSD1351 tft;

extern uint8_t joy_b1;
extern uint8_t joy_b2;
extern uint8_t btn_b3;

extern aAxis axis_list[4];

uint8_t to_mikul_active;
uint16_t to_mikul_port;
IPAddress to_mikul_ip;
unsigned long to_mikul_last = 0;
unsigned long act_millis = 0;
uint32_t rx_img_cnt;                // kolik packetu s obrazem jsme prijali
int8_t motor_send;                  // jakou hodnotu motoru posilame
uint8_t motor_state;                // stav ovladani motoru (0 - co na joy, to posilame, 1 - zafixovane)
unsigned long motor_state_millis;   // cas kdy jsme naposled menili stav motoru

// analogova osa
typedef struct mikulControls {
    uint8_t ident;
    int8_t motor;
    int8_t servo;
    uint8_t lights;
} mikulControls;

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
    int len;
    int packetSize;
    uint8_t packet[20];
    mikulControls mikul_controls;
    char buff[100];
    int8_t rssi;
    unsigned long millis_text;

    millis_text = 0;
    rssi = 0;

    // inicializace
    initMikul();

    // hlavni smycka
    while(1){
        // prisel nejaky UDP packet?
        packetSize = Udp.parsePacket();

        if (packetSize){
            //log_d("Received %d bytes from %s, port %d", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());

            // jeste nic neposilame na lod
            if(to_mikul_active == 0){
                // ulozime si kam posilat
                to_mikul_port = Udp.remotePort();
                to_mikul_ip = Udp.remoteIP();
                to_mikul_active = 1;
            }

            len = Udp.read(incomingPacket, packetSize);
            if (len > 0){
                //incomingPacket[len] = '\0';
                //log_d("UDP packet contents: %02x %02x", incomingPacket[0], incomingPacket[1]);

                if(incomingPacket[0] == 0xF0){
                    // rssi
                    rssi = incomingPacket[2];

                    // prohozeni bytu - prijdou opacne
                    ii = 0;
                    for(i = 3; i < len; i = i + 2){
                        image[ii] = incomingPacket[i+1];
                        image[ii + 1] = incomingPacket[i];
                        ii = ii + 2;
                    }
                    
                    // ktera cast obrazku to je
                    y = incomingPacket[1] * 5;
                    
                    // vykresleni te casti
                    tft.drawRGBBitmap(0, y, (uint16_t*)image, 128, 5);

                    // pocet prijatych obrazu
                    rx_img_cnt++;
                }

            }      
        }

        // cas aktualni
        act_millis = millis();

        // nacteni ovladacu
        readAllInputs();

        // budem posilat na lod?
        if(to_mikul_active == 1 && act_millis - to_mikul_last >= TO_MIKUL_INTERVAL){

            if(motor_state == 0){
                motor_send = axis_list[1].val;
            }

            // vytvorime packet
            mikul_controls.ident = 0xF1;
            mikul_controls.motor = motor_send;
            mikul_controls.servo = axis_list[2].val;
            mikul_controls.lights = 0;

            // posleme packet
            Udp.beginPacket(to_mikul_ip, to_mikul_port);
            Udp.write((uint8_t*)&mikul_controls, sizeof(mikul_controls)); 
            Udp.endPacket();

            // kdy jsme ho naposled poslali
            to_mikul_last = act_millis;
        }

        // Zobrazovani RSSI
        if(act_millis - millis_text > 1000){
            millis_text = act_millis;
            
            tft.fillRect(0, SCREEN_HEIGHT - 15, 64, 15, BLACK);
            tft.setCursor(0, 92);
            sprintf(buff, "RSSI %d", rssi);
            tft.print(buff);
        }

        // stisknuti leveho joye
        if(joy_b1 == 0 && act_millis - motor_state_millis > 500){
            if(motor_state == 0){
                motor_send = axis_list[1].val;
                motor_state = 1;
                tft.fillRect(128 - 6, 96 - 6, 6, 6, COLOR_RED);
            } else if(motor_state == 1){
                motor_state = 0;
                tft.fillRect(128 - 6, 96 - 6, 6, 6, COLOR_GREEN);
            }
            motor_state_millis = act_millis;
        }

        // navrat zpet na intro
        if(btn_b3 == 0){
            return;
        }
    }
}

/**
 * Inicializace
 */
void initMikul(){

    rx_img_cnt = 0;
    motor_state = 0;
    motor_state_millis = 0;

    // smazeme obrazovku
    canvas.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
    canvas.drawRect(0, 0, 128, 70, COLOR_DARK_GRAY);
    displayCanvas();

    tft.setFont(&Roboto_10);
    tft.setTextColor(COLOR_WHITE);

    tft.setCursor(0, 92);
    tft.print("Mikul");

    tft.fillRect(128 - 6, 96 - 6, 6, 6, COLOR_GREEN);

    // neposilame data na lod
    to_mikul_active = 0;
}