#ifndef RF69_FHSS_H
#define RF69_FHSS_H


#include <stdint.h>
#include <stdbool.h>
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
void fhss_select_hop_sequence(uint8_t);
void fhss_set_radio_tx_mode(bool);
bool fhss_send_packet_if_ready(uint8_t *);
bool fhss_get_received_packet_if_valid(uint8_t *);
uint16_t fhss_crc16_ccitt_reversed_0x8408(uint8_t *, uint8_t);
#if defined(ESP32)
void fhss_task(void *);
#else
void fhss_task(void);
#endif


// we use the byte value in the HOP_KEY_POSITION to determine which hop sequence to use (lower 3 bits, 0 to 7).
// the 4th LSB (bit-3) of the byte value allow use 2 sets of 8 hop sequences, 
// separated by the encryption key differences.
#define HOP_KEY (0x20)
#define HOP_KEY_POSITION (3)

static fhss_state_t fhss = {
    .timeout_timer = 0,
    .rx_timeout = 0,
    .tx_timeout = 0,
    .rx_millis_counter = 0,
    .tx_millis_counter = 0,
    .missed_counter = 0,
    .packet_received = false,
    .packet_transmitted = false,
    .radio_tx_mode = false
};

static radio_param_t radio = {
    // The encryption key has to be the same between tx and rx devices.
    .encryption_key = {0x7a, 0x61, 0x70, HOP_KEY, 0x61, 0x6c, 0x65, 0x64, 0x20, 0x79, 0x6e, 0x6f, 0x68, 0x74, 0x6e, 0x61},
    // The byte at HOP_KEY allow us to define a total of 16 hop sequences, that is,
    // two groups of 8 hop sequences because of the different encryption keys.
    .old_hop_key = HOP_KEY,
    // 903.240MHz (This corresponds to hop sequence value of 0)
    // The operating band is 902MHz to 928MHz for US
    .base_frequency = 903240,
    // Separation between carrier freq is 480kHz but channel bandwidth is 300kHz
    .channel_bandwidth = 480,
    .channels = {0},
    // selects which channel in a hop sequence (value is 0 to 49)
    .channel_selector = 0,
    // selects which hop sequence to use (value is 0 to 7)
    .hop_selector = 0,
    // The following are pseudorandom sequences generated from
    // https://www.calculatorsoup.com/calculators/statistics/random-number-generator.php
    .hop_sequence = {
        {42, 1, 2, 39, 46, 25, 34, 23, 19, 27, 10, 41, 35, 47, 48, 29, 3, 36, 32, 28, 38, 16, 9, 30, 31, 7, 14, 8, 17, 40, 22, 11, 44, 24, 33, 26, 20, 15, 4, 12, 6, 43, 45, 13, 21, 49, 5, 0, 37, 18},
        {5, 36, 4, 46, 20, 17, 7, 37, 29, 43, 32, 30, 21, 3, 0, 12, 34, 48, 26, 42, 33, 14, 19, 35, 24, 40, 28, 9, 6, 31, 44, 18, 8, 2, 25, 10, 41, 16, 1, 11, 49, 27, 13, 15, 39, 22, 47, 45, 38, 23},
        {32, 12, 7, 22, 9, 28, 20, 45, 44, 41, 10, 34, 23, 19, 17, 47, 38, 6, 43, 0, 5, 33, 4, 42, 11, 36, 13, 15, 3, 18, 27, 21, 31, 40, 49, 29, 24, 30, 14, 48, 35, 25, 39, 8, 37, 2, 46, 26, 16, 1},
        {45, 9, 3, 7, 46, 19, 29, 10, 6, 22, 12, 24, 5, 4, 34, 30, 18, 26, 32, 47, 41, 44, 8, 21, 37, 28, 14, 36, 0, 48, 16, 2, 15, 17, 1, 40, 33, 42, 25, 31, 49, 39, 23, 20, 11, 27, 43, 38, 13, 35},
        {48, 34, 7, 23, 13, 16, 17, 1, 9, 18, 2, 21, 10, 26, 33, 45, 5, 0, 31, 36, 32, 38, 46, 28, 39, 41, 3, 25, 11, 47, 37, 24, 19, 15, 35, 29, 22, 12, 43, 40, 44, 49, 8, 27, 42, 6, 30, 4, 14, 20},
        {40, 32, 18, 3, 25, 6, 29, 39, 38, 21, 7, 15, 49, 2, 36, 47, 11, 37, 26, 14, 16, 35, 19, 1, 27, 34, 46, 5, 44, 43, 33, 45, 10, 30, 8, 20, 9, 48, 24, 41, 22, 23, 0, 4, 17, 12, 28, 42, 13, 31},
        {44, 17, 40, 7, 35, 0, 5, 43, 13, 18, 6, 32, 42, 30, 39, 37, 21, 27, 10, 12, 31, 22, 11, 24, 9, 47, 38, 15, 16, 49, 1, 26, 14, 8, 33, 45, 46, 41, 23, 20, 34, 3, 48, 19, 2, 36, 28, 4, 29, 25},
        {1, 19, 34, 10, 21, 22, 42, 18, 44, 12, 29, 30, 35, 2, 11, 9, 31, 8, 16, 27, 28, 38, 39, 37, 41, 13, 0, 40, 24, 47, 26, 17, 20, 23, 4, 49, 46, 33, 15, 14, 5, 25, 32, 43, 48, 6, 36, 3, 7, 45}
    },
};

static RH_RF69 rf69(RFM69_NSS, RFM69_DIO0);
static uint8_t packet[PACKET_LENGTH]; // last two bytes are for CRC (crc_h and crc_l)
static uint8_t *rf_packet = (uint8_t *)&packet;

// Timer Interrupt
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328PB__)
// Timer 0, 1ms interrupt, used for rx mode
ISR(TIMER0_COMPA_vect)
{
    if (fhss.rx_millis_counter > 0)
    {
        --fhss.rx_millis_counter;
    }
    else
    {
        fhss.rx_timeout = 1;
    }
    if (fhss.tx_millis_counter > 0)
    {
        --fhss.tx_millis_counter;
    }
    else
    {
        fhss.tx_timeout = 1;
    }
    if (fhss.timeout_timer < BLANKING_TIMEOUT)
    {
        fhss.timeout_timer++;
    }
}
#elif defined(ESP32)
// used for esp32 1ms timer interrupt
hw_timer_t *esp32timer = NULL;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE CommMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR ESP32TimerISR()
{
    portENTER_CRITICAL_ISR(&mux);
    if (fhss.rx_millis_counter > 0)
    {
        --fhss.rx_millis_counter;
    }
    else
    {
        fhss.rx_timeout = 1;
    }
    if (fhss.tx_millis_counter > 0)
    {
        --fhss.tx_millis_counter;
    }
    else
    {
        fhss.tx_timeout = 1;
    }
    if (fhss.timeout_timer < BLANKING_TIMEOUT)
    {
        fhss.timeout_timer++;
    }
    portEXIT_CRITICAL_ISR(&mux);
}
#endif

/*
    Reset and configure the RFM69 radio module.
*/
static void fhss_setup_rfm69(void)
{
    pinMode(RFM69_RESET, OUTPUT);
    digitalWrite(RFM69_RESET, LOW);
    // manual hardware reset of rfm69
    delay(10);
    digitalWrite(RFM69_RESET, HIGH);
    delay(10);
    digitalWrite(RFM69_RESET, LOW);
    delay(10);
    if (rf69.init())
    {
        // RFM69 radio init OK!
        rf69.setModemConfig(RH_RF69::GFSK_Rb57_6Fd120); // default is GFSK_Rb250Fd250
    }
    if (rf69.setFrequency((float)radio.base_frequency / 1000.0f))
    {
        // RFM69 frequency is set.
    }
    // range from 14-20 for power, 2nd arg must be true for RFM69HCW
    rf69.setTxPower(20, true);
    rf69.setEncryptionKey(radio.encryption_key);
}

/*
    This function does the FHSS dance that allows the transmitter and receiver to synchronize.
    It is the receiver that performs the necessary logic to sync with the transmitter.
*/
static void fhss_rx_loop()
{
    // wait for one FHSS hop period (1/20 second) or until a packet is received.
    do
    {
        if (rf69.available())
        {
            uint8_t len = sizeof(packet);
            rf69.recv(rf_packet, &len);
            fhss.packet_received = true;
            fhss.missed_counter = 0;
            break;
        }
    } while (fhss.rx_timeout == 0);
    // if we didn't receive a packet, the next hop in the sequence will wait for one FHSS hop period.
    // if we missed packets for 20 consecutive period, the next hop will be on the 1st freq in the sequence.
    radio.channel_selector = (++radio.channel_selector) % TOTAL_HOP_CHANNELS;
    if (fhss.packet_received == false)
    {
        fhss.rx_millis_counter = (1000 / FHSS_HOP_RATE) / TIMER_MILLISEC_PER_COUNT;
        if (fhss.missed_counter < 20)
        {
            ++fhss.missed_counter;
        }
        else
        {
            radio.channel_selector = 0;
        }
    }
    // if we received a packet, the next hop in the sequence will wait for 1.2 times the FHSS hop period.
    // the 20% adjustment is to ensure that we can catch the next packet on the next hop.
    // we don't really wait that long when we're synchronized.
    else
    {
        fhss.rx_millis_counter = (1000 / FHSS_HOP_RATE + (1000 / FHSS_HOP_RATE) / 5) / TIMER_MILLISEC_PER_COUNT;
    }
    fhss.rx_timeout = 0;
    // hop to the next frequency in the sequence.
    rf69.setFrequency((float)radio.channels[radio.hop_sequence[radio.hop_selector][radio.channel_selector]] / 1000.0f);
}

/*
    This is the FHSS transmitter function.
    It just sends the packet every FHSS_HOP_RATE (1/20 second).
*/
static void fhss_tx_loop()
{
    if (fhss.tx_timeout == 1)
    { // transmit on mark!
        fhss.tx_millis_counter = ((1000 / FHSS_HOP_RATE) / TIMER_MILLISEC_PER_COUNT);
        fhss.tx_timeout = 0;
        rf69.send(rf_packet, sizeof(packet));
        rf69.waitPacketSent();
        radio.channel_selector = (++radio.channel_selector) % TOTAL_HOP_CHANNELS;
        rf69.setFrequency((float)radio.channels[radio.hop_sequence[radio.hop_selector][radio.channel_selector]] / 1000.0f);
        fhss.packet_transmitted = true;
    }
}

/*
    Selects the hop sequence to use.
    Bit 0 to Bit 2 determines the actual hop sequence frequencies selected.
    Bit 3 extends the total hop sequences to 2 sets of 8 hop sequences differentiated by the 
    different encryption key resulting from the modified byte in the HOP_KEY_POSITION.
*/
void fhss_select_hop_sequence(uint8_t key)
{
    key = key & 0x0f;
    radio.hop_selector = key & 0x07;
    radio.encryption_key[HOP_KEY_POSITION] = HOP_KEY + key;
    if (radio.old_hop_key != radio.encryption_key[HOP_KEY_POSITION])
    {
        radio.old_hop_key = radio.encryption_key[HOP_KEY_POSITION];
        rf69.setEncryptionKey(radio.encryption_key);
    }
}

/*
    Initialize the FHSS system
*/
void fhss_setup_rf_comm(void)
{
    // prepare the array of hop frequencies
    for (int i = 0; i < TOTAL_HOP_CHANNELS; i++)
    {
        radio.channels[i] = radio.base_frequency + i * radio.channel_bandwidth;
    }
    fhss_setup_rfm69();
    // We use a 1-ms timer interrupt for timing the FHSS
#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega328PB__)
    // Timer 0 is already used for millis().
    // We'll use millis()'s Timer0 and insert our interrupt
    // somewhere in the middle and call the "Timer0 Compare A" interrupt.
    cli();
    OCR0A = 0xAF;
    TIMSK0 |= _BV(OCIE0A);
    sei();
#else if defined(ESP32)
    esp32timer = timerBegin(1000000); // set timer freq to 1MHz
    timerAttachInterrupt(esp32timer, &ESP32TimerISR); // attach the ISR
    // set alarm to call the ISR every 1ms (in us).
    // repeat the alarm (3rd param) with unlimited count 0 (4th param)
    timerAlarm(esp32timer, 1000, true, 0); 
#endif
}

/*
    The transmitter continously sends the packet so that the receiver can remain synchronized.
    However, if no packet is sent by the main loop within BLANKING_TIMEOUT period (5 seconds)
    the transmitter will stop transmitting.
    This function resumes the transmission for another BLANKING_TIMEOUT period. 
*/
void fhss_clear_timeout_timer(void)
{
    fhss.timeout_timer = 0;
}

/*
    fhss_task() must be called at least every HOP_RATE (1/20 second).
*/
#if defined(ESP32)
void fhss_task(void *)
{
#else
void fhss_task(void)
{
#endif
#if defined(ESP32)
    while (1)
    {
#endif
        if (fhss.radio_tx_mode)
        {
            if (fhss.timeout_timer != BLANKING_TIMEOUT)
            {
                fhss_tx_loop();
            }
        }
        else
        {
            fhss_rx_loop();
        }
#if defined(ESP32)
        delay(1);
    }
#endif
}

/*
    Compute the crc
*/
uint16_t fhss_crc16_ccitt_reversed_0x8408(uint8_t *x, uint8_t len)
{
    uint8_t *data = x;
    uint16_t crc = 0;
    while (len--)
    {
        crc ^= *data++;
        for (uint8_t k = 0; k < 8; k++)
            if (crc & 1)
                crc = (crc >> 1) ^ 0x8408;
            else
                crc = crc >> 1;
    }
    return crc;
}

/*
    Copy the received packet if it's valid.
    The 'out' array should be the same size as the packet.
    Returns true for a valid received packet.
*/
bool fhss_get_received_packet_if_valid(uint8_t *out)
{
    if (out != NULL && fhss.packet_received)
    {
        uint8_t crc_h, crc_l;
        uint16_t crc;
        crc = fhss_crc16_ccitt_reversed_0x8408(packet, sizeof(packet) - 2);
        crc_h = crc >> 8;
        crc_l = crc & 0x00ff;
        if ((crc_h == packet[sizeof(packet) - 2]) && (crc_l == packet[sizeof(packet) - 1]))
        {
            memcpy(out, packet, sizeof(packet));
            return true;
        }
    }
    return false;
}

/*
    Copy the packet data to the fhss' internal packet buffer.
    Returns true if packet data was successfully copied.
    Resets to transmit timeout timer.
*/
bool fhss_send_packet_if_ready(uint8_t *packet_data)
{
    if (packet_data != NULL && fhss.packet_transmitted)
    {
        memcpy(packet, packet_data, sizeof(packet));
        fhss.packet_transmitted = false;
        fhss.timeout_timer = 0;
        return true;
    }
    return false;
}

/*
    Set transmit mode to true to act as fhss transmitter.
    False to act as fhss receiver.
*/
void fhss_set_radio_tx_mode(bool transmit_mode)
{
    fhss.radio_tx_mode = transmit_mode;
}


#endif // RF69_FHSS_H