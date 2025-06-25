# Frequency-hopping spread-spectrum (FHSS) C library for Arduino (Atmega328 and ESP32) using RFM69HCW radio module

## Introduction

This FHSS library allows frequency-hopping operation using sequences of frequencies arranged in a pseudo-random fashion. 
The library is available for use with Atmega328 and ESP32 on Arduino development framework but can easily be modified for use with other microcontrollers.

# How It Works

The library requires a 1ms interrupt service routine to drive the timing of the synchronization mechanism. In Atmega328-based Arduino, the `ISR(TIMER0_COMPA_vect)` was used since Timer0 is already being used for the `millis()` functionality. For ESP32, this is done using `timerBegin()`, `timerAttachInterrupt()`, and `timerAlarm()` functions to setup a repeating 1ms alarm interrupt. The 1ms timer interrupt is used to generate the HOP_RATE and communicate this using a set of tx and rx timeout flags. These flags are monitored by `fhss_rx_loop()` and `fhss_tx_loop()` which are called by `fhss_task()` depending on the `radio_tx_mode` flag.

### As A Transmitter

The RFM69 module is configured for transmission when the `radio_tx_mode` flag is set to `true`.
When the library is set for transmitter operation, only the `fhss_tx_loop()` gets called by `fhss_task()`. Every time the `tx_timeout` flag is set in the timer interrupt, `fhss_tx_loop()` sends the 20-byte packet to the RFM69 module and waits for the completion of its transmission. Afterwards, it hops to the next frequency in the selected hop sequence.

### As A Receiver

The `fhss_rx_loop()` function is where the FHSS synchronization magic happens.
The `fhss_rx_loop()` gets called by `fhss_task()` when the `radio_tx_mode` flag is set to `false`.<br>
In `fhss_rx_loop()`, the library waits for one FHSS hop period (1/20 second) or until a packet is received. <br>
If no packet was received, the next hop in the sequence will wait for one FHSS hop period. The library will continue to do this waiting every hop until it received a packet or 20 waits was done. If the number of wait-hops reached 20, the library will no longer hop but will wait at the first frequency of the hop sequence.  The reason for this is: after 20 hops and having not able to receive any packet, it's better to just wait on one frequency to prevent any "hopping race" between the transmitter and receiver preventing them to catch up with each other.<br><br>
Once a valid packet is received, the wait period on the next hop is adjust to an additional 20% duration. This is to have a wider window on the next hop to catch the transmitter. We are reminded that though the wait-hop when synchronized has an added 20% duration, the receiver actually only waits for a few milliseconds only before it receives the next packet and immediately hops to the next frequency to wait for the transmitter.



## How To Use

* Install the `RadioHead by Mike McCauley` library in Arduino IDE's Library Manager.

* Copy the `rf69_fhss.h` file to your sketch directory.

* Include it on your sketch / program:

```cpp
#include "rf69_fhss.h"
```
* Read the example sketches to learn how to use each functions.


