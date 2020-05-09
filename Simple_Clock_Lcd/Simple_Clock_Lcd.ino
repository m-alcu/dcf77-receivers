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
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see http://www.gnu.org/licenses/

#include <dcf77.h>
#include <LiquidCrystal_I2C.h>


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
uint8_t level = 0;
uint16_t pulse = 0; 

uint8_t sample_input_pin() {
    const uint8_t sampled_data =
        #if defined(__AVR__)
        dcf77_inverted_samples ^ (dcf77_analog_samples? (analogRead(dcf77_analog_sample_pin) > 200)
                                                      : digitalRead(dcf77_sample_pin));
        #else
        dcf77_inverted_samples ^ digitalRead(dcf77_sample_pin);
        #endif


    if (level != sampled_data) {
      if (level == 0) countBits = 0;
      if (level == 1) pulse = countBits;
    }
    countBits++;
    level = sampled_data;

    digitalWrite(ledpin(dcf77_monitor_led), sampled_data);
    return sampled_data;
}

int signalQual = 0;

//LiquidCrystal_I2C lcd(0x27,16,2);
LiquidCrystal_I2C lcd(0x3F,16,2);

void setup() {
    using namespace Clock;

    lcd.init(); 
    lcd.backlight();

    pinMode(ledpin(dcf77_monitor_led), OUTPUT);
    pinMode(dcf77_sample_pin, dcf77_pin_mode);

    DCF77_Clock::setup();
    DCF77_Clock::set_input_provider(sample_input_pin);


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

  lcd.setCursor(0,0);
  switch (BCD::bcd_to_int(now.weekday)) {
      case 1: lcd.print("Mon"); break;
      case 2: lcd.print("Tue"); break;
      case 3: lcd.print("Wed"); break;
      case 4: lcd.print("Thu"); break;
      case 5: lcd.print("Fri"); break;
      case 6: lcd.print("Sat"); break;
      case 7: lcd.print("Sun"); break;
  }

  lcd.setCursor(4,0);
  printDigits(BCD::bcd_to_int(now.day));
  lcd.print("/");
  lcd.setCursor(7,0);
  printDigits(BCD::bcd_to_int(now.month));
  lcd.setCursor(9,0);
  lcd.print("/");
  lcd.setCursor(10,0);
  lcd.print(2000+BCD::bcd_to_int(now.year));
  lcd.setCursor(14,0);
  lcd.print("+");
  lcd.setCursor(15,0);
  lcd.print(now.uses_summertime? 2: 1);  


  lcd.setCursor(0,1);
  if (pulse < 100) {
    lcd.print("0");
  }
  lcd.print(pulse);

  lcd.setCursor(3,1);
  switch (DCF77_Clock::get_clock_state()) {
      case Clock::useless: lcd.print("U"); break;
      case Clock::dirty:   lcd.print("D"); break;
      case Clock::synced:  lcd.print("S"); break;
      case Clock::locked:  lcd.print("L"); break;
  }

  lcd.setCursor(4,1);
  signalQual = DCF77_Clock::get_prediction_match();
  if (signalQual == 255 || signalQual == 0 ) {
      lcd.print("  ");
  } else {
      if (signalQual < 10) {
        lcd.print("0");
      }
      lcd.print(signalQual);
  }


  lcd.setCursor(6,1);
  lcd.print(now.timezone_change_scheduled? "H": " ");

  lcd.setCursor(7,1);
  printDigits(BCD::bcd_to_int(now.hour));
  lcd.setCursor(9,1);
  lcd.print(":");
  lcd.setCursor(10,1);
  printDigits(BCD::bcd_to_int(now.minute));
  lcd.setCursor(12,1);
  lcd.print(":");
  lcd.setCursor(13,1);
  printDigits(BCD::bcd_to_int(now.second));

  lcd.setCursor(15,1);
  lcd.print(now.leap_second_scheduled? "S": " ");
  
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  if(digits < 10)
    lcd.print('0');
  lcd.print(digits);
}
