#ifndef RF69_FHSS_H
#define RF69_FHSS_H

#include "stdint.h"
#include "stdbool.h"
#include <RH_RF69.h>

#define TOTAL_HOP_PATTERNS  (8)
#define TOTAL_HOP_CHANNELS  (50)
#define FHSS_HOP_RATE       (20) // number of frequency hops per second
#define BLANKING_TIMEOUT    (5000) // stop tx after 5secs.  call "fhss_clear_timeout_timer()" to resume.
#define PACKET_LENGTH       (20)

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328PB__)
// GPIO assignments for RFM69 for Atmega
#define RFM69_RESET 2
#define RFM69_MOSI 11
#define RFM69_MISO 12
#define RFM69_SCK 13
#define RFM69_NSS 4
#define RFM69_DIO0 3
// for 8MHz clock, timer0 interrupt is every 2ms
#define TIMER_MILLISEC_PER_COUNT 2 
#elif defined(ESP32)
// GPIO assignments for RFM69 for ESP32
#define RFM69_RESET 17
#define RFM69_MOSI 23
#define RFM69_MISO 19
#define RFM69_SCK 18
#define RFM69_NSS 5
#define RFM69_DIO0 16
// interrupt is every 1ms
#define TIMER_MILLISEC_PER_COUNT 1 
#else
#error "Unsupported hardware"
#endif

struct fhss_state_s
{
    volatile int timeout_timer;
    volatile uint8_t rx_timeout;
    volatile uint8_t tx_timeout;
    volatile uint8_t rx_millis_counter;
    volatile uint8_t tx_millis_counter;
    volatile uint8_t missed_counter;
    volatile bool packet_received;
    volatile bool packet_transmitted;
    volatile bool radio_tx_mode;
};
typedef struct fhss_state_s fhss_state_t;

// RFM69 parameters
struct radio_param_s
{
    uint8_t encryption_key[16];
    uint8_t old_hop_key;
    uint32_t base_frequency;
    uint32_t channel_bandwidth;
    uint32_t channels[TOTAL_HOP_CHANNELS];
    uint8_t channel_selector;
    uint8_t hop_selector;
    uint8_t hop_sequence[TOTAL_HOP_PATTERNS][TOTAL_HOP_CHANNELS];
};
typedef struct radio_param_s radio_param_t;


// function declarations
void fhss_setup_rf_comm(void);
void fhss_clear_timeout_timer(void);
void fhss_set_encryption_key(uint8_t);
bool fhss_send_packet_if_ready(uint8_t *);
bool fhss_get_received_packet_if_valid(uint8_t *);
uint16_t fhss_crc16_ccitt_reversed_0x8408(uint8_t *, uint8_t);
#if defined(ESP32)
void FHSS_Task(void *);
#else
void fhss_Task(void);
#endif



#endif // RF69_FHSS_H