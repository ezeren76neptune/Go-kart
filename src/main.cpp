#include <LiquidCrystal.h>
#include <Arduino.h>
#include <ezButton.h>
#include <RotaryEncoder.h>


const int RS = 11, EN = 12, D4 = 2, D5 = 3, D6 = 4, D7 = 5;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);
const int Fuel_level_pin = A0;
const int Speed_pin = A1;
const int RPM_pin = A2;
const int Temp_pin = A3;
const int Ligth_switch_pin = A4;
const int Menu_switch_pin = 6;  //Button
const int encoderPin1 = A2;  //Rotary encoder
const int encoderPin2 = A3; //Rotary encoder

int menu = 1;
int prevMenu = -1;
int Fuel_level = 0;
int Speed = 0;
int RPM = 0;
int Temp = 0;
int Ligth_switch_mode = 0;
int speed_mode = 0;
int aceleration_mode = 0;
int current_selection = 0;

ezButton button(Menu_switch_pin);
RotaryEncoder encoder(encoderPin1, encoderPin2);

float speed_mode_1 = 0.33; // 33% of the max speed
float speed_mode_2 = 0.66; // 66% of the max speed
float speed_mode_3 = 1.00; // 100% of the max speed

//custom characters
byte up_arrow[8] = {
        0b00100,
        0b01110,
        0b10101,
        0b00100,
        0b00100,
        0b00100,
        0b00000,
        0b00000
};

byte return_arrow[8] = {
        0b00100,
        0b01000,
        0b11111,
        0b01001,
        0b00101,
        0b00001,
        0b00111,
        0b00000
};

byte sideways_arrow[8] = {
        0b00000,
        0b00100,
        0b01000,
        0b11111,
        0b01000,
        0b00100,
        0b00000,
        0b00000
};

void setup() {
    lcd.begin(16, 2);    // set up number of columns and rows
    lcd.setCursor(0, 0); // move cursor to   (0, 0)
    pinMode(Menu_switch_pin, INPUT);

    lcd.createChar(0, up_arrow);
    lcd.createChar(1, return_arrow);
    lcd.createChar(2, sideways_arrow);
    Serial.begin(9600);
    lcd.clear();
}

void loop() {

// Button Stuff
    button.loop();
    int buttonpressed = button.isPressed();
    button.setDebounceTime(50); // set debounce time to 50 milliseconds
// encoder set up stuff
    static int pos = 0;
    encoder.tick();
    int newPos = encoder.getPosition();
// serial print the button state
    if (buttonpressed == HIGH) {
        Serial.print("Button pressed\n");
    }
// changes the current selection based on the encoder position

    if (newPos > pos) {
        Serial.print("up");
        if (menu == 2) {
            if (current_selection < 2) {
                current_selection = current_selection + 1;
            }
        } else if (current_selection < 4 && menu != 1) {
            current_selection = current_selection + 1;
        }
    } else if (newPos < pos) {
        Serial.print("down");
        if (current_selection > 1 && menu != 1) {
            current_selection = current_selection - 1;
        }
    }

    switch (menu) {
        case 1: //Home screen screen
            current_selection = 0;
            lcd.setCursor(0, 0);
            lcd.print(String(Speed) + " Mph");
            lcd.setCursor(0, 1);
            lcd.print(String(Temp) + " C");
            lcd.setCursor(7, 0);
            lcd.print(String(RPM) + " RPM");
            lcd.setCursor(7, 1);
            lcd.print("Mode: " + String(speed_mode));

            if (buttonpressed == HIGH) { //If the button is pressed and the menu is the home screen then change to the settings menu
                menu = 2;
                lcd.clear();
                current_selection = 1;
            }
            break;
        case 2: //Settings Main menu
            lcd.setCursor(0, 0);
            lcd.print("Top Speed");
            lcd.setCursor(0, 1);
            lcd.print("Aceleration");
            if (buttonpressed == HIGH && current_selection == 1) { //If the button is pressed and the current selection is 1 then change to the speed settings menu
                menu = 3;
                lcd.clear();
                current_selection = 1;

            }
            if (buttonpressed == HIGH && current_selection == 2) { //If the button is pressed and the current selection is 2 then change to the aceleration settings menu
                menu = 4;
                lcd.clear();
                current_selection = 1;
            }
            break;
        case 3: //Mode switching menu
            lcd.setCursor(0, 0);
            lcd.print("Mode");
            lcd.setCursor(0, 1);
            lcd.print("Max");
            lcd.setCursor(5, 0);
            lcd.print('1');
            lcd.setCursor(8, 0);
            lcd.print('2');
            lcd.setCursor(11, 0);
            lcd.print('3');
            lcd.setCursor(14, 0);
            lcd.write(byte(1));
            break;

        case 4: //Aceleration settings menu
            lcd.setCursor(0, 0);
            lcd.print("Mode ");
            lcd.setCursor(0, 1);
            lcd.print("m/h^2");
            lcd.setCursor(5, 0);
            lcd.print('1');
            lcd.setCursor(8, 0);
            lcd.print('2');
            lcd.setCursor(11, 0);
            lcd.print('3');
            lcd.setCursor(14, 0);
            lcd.write(byte(1));
            break;
    }

    //current selection stuff
    if (menu == 2 && pos != newPos) { //If the menu is the settings menu
        lcd.setCursor(12, 0);
        lcd.print(" ");
        lcd.setCursor(12, 1);
        lcd.print(" ");
        switch (current_selection) { //Arrow position
            case 1:
                lcd.setCursor(12, 0);
                lcd.write(byte(2));
                break;
            case 2:
                lcd.setCursor(12, 1);
                lcd.write(byte(2));
                break;

        }
    } else if((menu != 2 || menu != 1) && pos != newPos) { // If the menu is not the settings menu or the home screen
        lcd.setCursor(5, 1);
        lcd.print(" ");
        lcd.setCursor(8, 1);
        lcd.print(" ");
        lcd.setCursor(11, 1);
        lcd.print(" ");
        lcd.setCursor(14, 1);
        lcd.print(" ");
        switch (current_selection) { //Arrow position
            case 1:
                lcd.setCursor(5, 1);
                lcd.write(byte(0));
                break;

            case 2:
                lcd.setCursor(8, 1);
                lcd.write(byte(0));

                break;
            case 3:
                lcd.setCursor(11, 1);
                lcd.write(byte(0));
                break;
            case 4: //return arrow for the mode and aceleration settings menu
                lcd.setCursor(14, 1);
                lcd.write(byte(0));
                break;
        }
    }
    if (menu != 2 && menu != 1 && current_selection == 4 && buttonpressed == HIGH) {
        menu = 2;
        lcd.clear();
        current_selection = 1;
    }
    //serial print the encoder position
    if (pos != newPos) {
        //Serial.print(newPos);
        pos = newPos;
    }
    Serial.print(current_selection);

    /*Top Speed and acceleration code below this*/


}

