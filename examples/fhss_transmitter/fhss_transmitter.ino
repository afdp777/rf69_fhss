/*
  Example code for Arduino Uno (or similar boards using Atmega328)
*/

#include "rf69_fhss.h"

#define READY_LED     LED_BUILTIN
#define LED_ON_VALUE  HIGH
#define LED_OFF_VALUE LOW

bool toggle = false;
unsigned long previousMillis = 0;
  uint8_t packet_data[PACKET_LENGTH];

void setup() {  
  Serial.begin(115200);
  // LED on the board
  pinMode(READY_LED, OUTPUT);
  digitalWrite(READY_LED, LED_ON_VALUE);
  // use rfm69 as transmitter only
  fhss_set_radio_tx_mode(true); 
  // selects hop sequence 0 and default encryption key
  fhss_select_hop_sequence(0); 
  // configure the rf69 hardware and the fhss driver
  fhss_setup_rf_comm();
}

void loop() {
  // prepare packet data
  packet_data[1] = previousMillis & 0xff;
  packet_data[0] = (previousMillis >> 8) & 0xff;
  // compute crc and add it to packet 
  uint16_t crc = fhss_crc16_ccitt_reversed_0x8408(packet_data, sizeof(packet_data)-2);
  packet_data[sizeof(packet_data)-2] = (crc >> 8) & 0xff; // high byte of crc
  packet_data[sizeof(packet_data)-1] = crc & 0xff; // low byte of crc
  if(fhss_send_packet_if_ready(packet_data))
  {
    // packet was sent
    toggle = !toggle;
    digitalWrite(READY_LED, toggle ? LED_ON_VALUE : LED_OFF_VALUE);
    previousMillis = millis();
  }
  // call fhss_task at least every 1ms or more frequent
  fhss_task();
  // toggle LED every 2 seconds if we can't transmit any packets.
  if (millis() - previousMillis > 2000) {
    previousMillis = millis();
    toggle = !toggle;
    digitalWrite(READY_LED, toggle ? LED_ON_VALUE : LED_OFF_VALUE);
  }
}
