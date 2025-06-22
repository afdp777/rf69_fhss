/*
  Example code for ESP32 Dev Module
*/

#include "rf69_fhss.h"

#define READY_LED     2
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
  // use rfm69 as receiver only
  fhss_set_radio_tx_mode(false); 
  // selects hop sequence 0 and default encryption key
  fhss_select_hop_sequence(0); 
  // configure the rf69 hardware and the fhss driver
  fhss_setup_rf_comm();
  xTaskCreatePinnedToCore(fhss_task, "FHSS_Task", 4096, NULL, 10, NULL, 1);
}

void loop() {
  // check if valid data was received and put it in 'packet_data'
  if(fhss_get_received_packet_if_valid(packet_data))
  {
      // display the data
      for (int i = 0; i < 20; i++) {
        Serial.print(packet_data[i], HEX);
        Serial.print(" ");
      }
      Serial.println();
      Serial.flush();
      // toggle the LED at half the FHSS rate to indicate valid data was received.
      // the LED will toggle at this faster rate when packets
      // were continously received successfully.
      toggle = !toggle;
      digitalWrite(READY_LED, toggle ? LED_ON_VALUE : LED_OFF_VALUE);
      previousMillis = millis();
  }
  // toggle LED every 2 seconds if we're not receiving any packets.
  if (millis() - previousMillis > 2000) {
    previousMillis = millis();
    toggle = !toggle;
    digitalWrite(READY_LED, toggle ? LED_ON_VALUE : LED_OFF_VALUE);
  }
}
