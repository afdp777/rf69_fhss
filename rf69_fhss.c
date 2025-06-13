#include "rf69_fhss.h"

#define HOP_KEY (0x20)
#define HOP_KEY_POSITION (3)

static volatile fhss_state_t fhss = {
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

static volatile radio_param_t radio = {
    // The encryption key has to be the same between tx and rx devices
    .encryption_key = {0x7a, 0x61, 0x70, HOP_KEY, 0x61, 0x6c, 0x65, 0x64, 0x20, 0x79, 0x6e, 0x6f, 0x68, 0x74, 0x6e, 0x61},
    // The byte at HOP_KEY allow us to define a total of 16 hop sequences, that is,
    // two groups of 8 hop sequences because of the different encryption keys.
    .old_hop_key = HOP_KEY,
    // 903.240MHz (This corresponds to hop sequence value of 0)
    // The operating band is 902MHz to 928MHz for US
    .base_frequency = 903240,
    // Separation between carrier freq is 480kHz but channel bandwidth is 300kHz
    .channel_bandwidth = 480,
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
    }
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

static void fhss_setup_rfm69()
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

static void fhss_rx_loop()
{
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
    else
    {
        fhss.rx_millis_counter = (1000 / FHSS_HOP_RATE + (1000 / FHSS_HOP_RATE) / 5) / TIMER_MILLISEC_PER_COUNT;
    }
    fhss.rx_timeout = 0;
    rf69.setFrequency((float)radio.channels[radio.hop_sequence[radio.hop_selector][radio.channel_selector]] / 1000.0f);
}

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

void fhss_set_encryption_key(uint8_t key)
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
    // for an 80MHz cpu clock: 1ms rate = 80MHz / (80 * 1000)
    esp32timer = timerBegin(0, 80, true); // prescaler of 80
    timerAttachInterrupt(esp32timer, &ESP32TimerISR, true);
    timerAlarmWrite(esp32timer, 1000, true);
    timerAlarmEnable(esp32timer);
#endif
}

void fhss_clear_timeout_timer(void)
{
    fhss.timeout_timer = 0;
}

#if defined(ESP32)
void FHSS_Task(void *)
{
#else
void fhss_Task(void)
{
#endif
#ifdef DEBUG_ON
    Serial.println("FHSS_Task started.");
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

bool fhss_send_packet_if_ready(uint8_t *packet_data)
{
    if (packet_data != NULL && fhss.packet_transmitted)
    {
        memcpy(packet, packet_data, sizeof(packet));
        fhss.packet_transmitted = false;
        return true;
    }
    return false;
}