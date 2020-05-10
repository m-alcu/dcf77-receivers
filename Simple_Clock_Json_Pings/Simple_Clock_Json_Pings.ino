//
//  www.blinkenlight.net
//
//  Copyright 2016 Udo Klein
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more detailSerial.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see http://www.gnu.org/licenses/

#include <dcf77.h>
#include <PriUint64.h>

const uint8_t dcf77_analog_sample_pin = 3;
const uint8_t dcf77_sample_pin = 17;       // A5 == d19
const uint8_t dcf77_inverted_samples = 0;
const uint8_t dcf77_analog_samples = 0;
const uint8_t dcf77_pin_mode = INPUT;  // disable internal pull up
//const uint8_t dcf77_pin_mode = INPUT_PULLUP;  // enable internal pull up

const uint8_t dcf77_monitor_led = 13;  // A4 == d18

uint8_t ledpin(const uint8_t led) {
    return led;
}

uint16_t countBits = 0;
uint16_t ms_counter = 0;
uint64_t messageBuffer = 0;
uint64_t outputBuffer = 0;


uint8_t sample_input_pin() {
    const uint8_t sampled_data =
        #if defined(__AVR__)
        dcf77_inverted_samples ^ (dcf77_analog_samples? (analogRead(dcf77_analog_sample_pin) > 200)
                                                      : digitalRead(dcf77_sample_pin));
        #else
        dcf77_inverted_samples ^ digitalRead(dcf77_sample_pin);
        #endif

    if (sampled_data == 1) {
      countBits++;
    }

    digitalWrite(ledpin(dcf77_monitor_led), sampled_data);
    return sampled_data;
}

void output_handler(const Clock::time_t &decoded_time) {
    ms_counter = countBits;
    countBits = 0;
    messageBuffer = messageBuffer << 1;
    if (ms_counter > 150) {
        messageBuffer = messageBuffer | 1;
    }

    int sec = BCD::bcd_to_int(decoded_time.second);
    if (sec == 0) {
        outputBuffer = messageBuffer;
        Serial.print("guardo output segundo: ");
        Serial.println(sec);
    }
}

int signalQual = 0;
int clockState = 0;

void setup() {
    using namespace Clock;

    Serial.begin(9600);

    pinMode(ledpin(dcf77_monitor_led), OUTPUT);
    pinMode(dcf77_sample_pin, dcf77_pin_mode);

    DCF77_Clock::setup();
    DCF77_Clock::set_input_provider(sample_input_pin);
    DCF77_Clock::set_output_handler(output_handler);


    // Wait till clock is synced, depending on the signal quality this may take
    // rather long. About 5 minutes with a good signal, 30 minutes or longer
    // with a bad signal
    for (uint8_t state = Clock::useless;
        state == Clock::useless || state == Clock::dirty;
        state = DCF77_Clock::get_clock_state()) {

        // wait for next sec
        Clock::time_t now;
        DCF77_Clock::get_current_time(now);
        SendStatus(now);
    }
}

void loop() {
    Clock::time_t now;

    DCF77_Clock::get_current_time(now);
    if (now.month.val > 0) {
      SendStatus(now);
    }
}

void SendStatus(Clock::time_t now) {

  switch (DCF77_Clock::get_clock_state()) {
      case Clock::useless: clockState = 1; break;
      case Clock::dirty:   clockState = 2; break;
      case Clock::synced:  clockState = 3; break;
      case Clock::locked:  clockState = 4; break;
  }
  Serial.print("{\"ClockState\":");
  Serial.print(clockState);
  Serial.print(",\"SignalQual\":");
  Serial.print(DCF77_Clock::get_prediction_match());
  Serial.print(",\"Weekday\":");
  Serial.print(BCD::bcd_to_int(now.weekday));
  Serial.print(",\"Day\":");
  Serial.print(BCD::bcd_to_int(now.day));
  Serial.print(",\"Month\":");
  Serial.print(BCD::bcd_to_int(now.month));
  Serial.print(",\"Year\":");
  Serial.print(2000+BCD::bcd_to_int(now.year));
  Serial.print(",\"Summertime\":");
  Serial.print(now.uses_summertime? 2: 1);
  Serial.print(",\"ChangeScheduled\":");
  Serial.print(now.timezone_change_scheduled? 1: 0);
  Serial.print(",\"Hour\":");
  Serial.print(BCD::bcd_to_int(now.hour));
  Serial.print(",\"Minute\":");
  Serial.print(BCD::bcd_to_int(now.minute));
  Serial.print(",\"Second\":");
  Serial.print(BCD::bcd_to_int(now.second));
  Serial.print(",\"LeapSecond\":");
  Serial.print(now.leap_second_scheduled? 1: 0);
  Serial.print(",\"MSCounter\":");
  Serial.print(ms_counter);   
  Serial.print(",\"Buffer\":");
  Serial.print(Serial.print(PriUint64<BIN>(outputBuffer)));  
  Serial.print("}");
  Serial.println("");
  
}
