#include <LiquidCrystal.h>
#include <Arduino.h>
#include <ezButton.h>
#include <RotaryEncoder.h>
#include <pins_arduino.h>
#include <servo.h>
#include <LiquidCrystal_I2C.h>
#include <LCDMenuLib2.h>

Servo throttle_servo;

const int RS = 11, EN = 12, D4 = 2, D5 = 3, D6 = 4, D7 = 5;
LiquidCrystal lcd(RS, EN, D4, D5, D6, D7);
const int ON = 1;
const int OFF = 0;
const int Fuel_level_pin = A0;
const int Speed_pin = A1;
const int RPM_pin = A2;
const int Temp_pin = A3;
const int Ligth_switch_pin = 9;
const int Menu_switch_pin = 6; // Button
const int encoderPin1 = A2;    // Rotary encoder
const int encoderPin2 = A3;    // Rotary encoder
const int hallPin_engine = 7;  // Hall effect sensor pin for engine
const int hallPin_wheel = 8;   // Hall effect sensor pin for wheel
const int gas_pedal_pin = A5;  // Gas pedal pin
int Light_Pin_upper = 15;      // LED pin, controls a relay for the lights
int Light_Pin_lower = 16;      // LED pin, controls a relay for the lights
int Light_Pin_back = 17;       // LED pin, controls a relay for the lights
int Fuel_gauge_pin = 7;        // Fuel gauge pin -  make sure its a PWM pin

// Global variables
unsigned long currentMillis = 0;  // will store current time
unsigned long previousMillis = 0; // will store last time servo was updated
int currentPosition = 0;          // current position of the servo, set to 0 for initialization
unsigned int interval = 15;       // interval at which to move servo (milliseconds)
int targetPosition = 0;           // target position for the servo, Set at 0 for initialization

// RPM sensor stuff engine and at wheel
volatile byte revolutions_engine;
unsigned long timeold_engine;
int rpm_engine = 0;

volatile byte revolutions_wheel;
unsigned long timeold_wheel;
int rpm_wheel = 0;

// global functions
void rpm_engine_function()
{
    revolutions_engine++;
}

void rpm_wheel_function()
{
    revolutions_wheel++;
}

void moveServo(Servo &servo, int targetPosition)
{
    currentMillis = millis();

    if (currentMillis - previousMillis >= interval)
    {
        previousMillis = currentMillis;

        currentPosition = servo.read();

        if (currentPosition < targetPosition)
        {
            servo.write(currentPosition + 1);
        }
        else if (currentPosition > targetPosition)
        {
            servo.write(currentPosition - 1);
        }
    }
}

int menu = 1;
int prevMenu = -1;
float Speed = 0;          // Speed in MPH
int Temp = 0;             // Temperature of the coolent
int speed_mode = 0;       // what speed mode is being used, initilaizes it as 0
int aceleration_mode = 0; // What acceleration mode is being used
int current_selection = 0;
float throttle = 0;
int prevmenu = 0;

// throttle servo
int max_throttle_pos = 1023; // Tells the program what the max throttle value is (0-1023)
int min_throttle_pos = 0;    // tells the program what the min throttle value is (0-1023)
int max_carb_pos = 180;      // sets max carb position
int min_carb_pos = 0;        // sets min carb position
float new_throttle = 0;      // sets the throttle percent to 0
float carb_pos = 0;          // sets the carb position to 0
int u = 0;                   // time in miliseconds per step at mph = 0
float a = 0;                 // constant for acceleration equation
int Q = .1;                  // Proportional control constant for throttle

int speed_offset = 10;         // point at which the max throttle will start to decrease
int max_speed_carb_offset = 0; // offset for carb at max speed, so the throttle doesnt close all the way so you can stay at max speed

// Change these for the three different speed modes
int speedmodes[3] = {25, 35, 100};
int max_speed_carb_offset_array[3] = {20, 30, 40}; // array for the different speed modes

// Acceleration equations
// use this link for help with the equations https://www.desmos.com/calculator/m5c8bwpr22

float Fuel_level() // When called this will return the fuel level as a percentage and update the fuel gauge
{
    float fuelsensorValue = map(analogRead(Fuel_level_pin), 0, 1023, 0, 255); // reads the fuel level from the fuel level sensor
    analogWrite(Fuel_gauge_pin, fuelsensorValue);                             // writes the fuel level to the fuel gauge
    int percentage = map(fuelsensorValue, 0, 255, 0, 100);                    // Might have to change max and min values
    return percentage;
}

float accelerationMode1() // muddy
{
    u = 40;  // milisecconds
    a = .05; // constant
    return u * exp(Speed * a);
}

float accelerationMode2() // normal
{
    u = 20;  // milisecconds
    a = .05; // constant
    return u * exp(Speed * a);
}

float accelerationMode3() // there should be no acceleration equation for this mode and just return 0 miliseconds
{
    u = 0;    // milisecconds
    return u; //
}

void LightToggle(int pin, int state)
{
    pinMode(pin, OUTPUT);
    digitalWrite(pin, state);
}
ezButton button(Menu_switch_pin);
RotaryEncoder encoder(encoderPin1, encoderPin2);

/* / Experimental code for the menu

enum Menus
{
    Main_menu,
    Settings_menu,
    Speed_settings_menu,
    Aceleration_settings_menu
};
enum Direction
{
    Up,
    Down,
    Left,
    Right,
    Nothing
};
void setmenu(Menus menus)
{
    lcd.clear();
    switch (menus)
    {
    case Main_menu: // main menu
        lcd.setCursor(0, 0);
        lcd.print(String(int(floor(Speed))) + " Mph");
        lcd.setCursor(0, 1);
        lcd.print(String(Temp) + " C");
        lcd.setCursor(8, 0);
        lcd.print(String(rpm_engine) + " RPM");
        lcd.setCursor(8, 1);
        lcd.print("Mode: " + String(speed_mode));
        menu = 1;
        break;
    case Settings_menu: // settings menu
        lcd.setCursor(0, 0);
        lcd.print("Top Speed");
        lcd.setCursor(0, 1);
        lcd.print("Aceleration");
        menu = 2;
        break;
    case Speed_settings_menu: // speed settings menu
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
        int cursor_start_x = 5;
        int cursor_start_y = 1;
        menu = 3;
        break;
    case Aceleration_settings_menu: // aceleration settings menu
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
        int cursor_start_x = 5;
        int cursor_start_y = 1;
        menu = 4;
        break;
    }
    return;
}

void moveCursor(Direction direction, int start_x, int start_y, boolean clicked, Menus selected_menu)
{
    int cursor_pos_x = start_x;
    int cursor_pos_y = start_y;
    lcd.setCursor(cursor_pos_x, cursor_pos_y);
    lcd.write(" ");
    if ((cursor_pos_x == 5 && direction == Left) || (cursor_pos_x == 8 && direction == Right) || (cursor_pos_y == 1 && direction == Down) || (cursor_pos_ == 0 && direction == Up))
    {
        return;
    }
    switch (direction)
    {
    case Up:
        cursor_pos_y = cursor_pos_y + 1;
        lcd.write(byte(2));
        break;
    case Down:
        cursor_pos_y = cursor_pos_y - 1;
        lcd.write(byte(2));
        break;
    case Left:
        cursor_pos_x = cursor_pos_x - 3;
        lcd.write(byte(0));
        break;
    case Right:
        cursor_pos_x = cursor_pos_x + 3;
        lcd.write(byte(0));
        break;
    case Nothing:
        break;
    }
}

boolean Check_encoder()
{
    static int pos = 0;
    encoder.tick();
    int newPos = encoder.getPosition();
    if (newPos != pos) {
        pos = newPos;
        return 1;
    }
    return 0;
}

boolean Check_button()
{
    button.loop();
    int buttonpressed = button.isPressed();
    button.setDebounceTime(50); // set debounce time to 50 milliseconds
    if (buttonpressed == HIGH)
    {
        return 1;
    }
    return 0;
}

 USE:
setmenu(Main_menu); //starts at main menu
if ((Check_button() == 1 || Check_encoder() != 0) && menu = 1){
    moveCursor(Nothing, 5, 1, Check_button(), Settings_menu);
    moveCursor(Nothing, 12, 0, Check_button(), Settings_menu);
}

if ((Check_button() == 1 || Check_encoder() != 0) && menu = 2){
    moveCursor(Nothing, 12, 0, Check_button(), Settings_menu);
}

END OF EXPERIMENTAL CODE */


// custom characters
byte up_arrow[8] = {
    0b00100,
    0b01110,
    0b10101,
    0b00100,
    0b00100,
    0b00100,
    0b00000,
    0b00000};

byte return_arrow[8] = {
    0b00100,
    0b01000,
    0b11111,
    0b01001,
    0b00101,
    0b00001,
    0b00111,
    0b00000};

byte sideways_arrow[8] = {
    0b00000,
    0b00100,
    0b01000,
    0b11111,
    0b01000,
    0b00100,
    0b00000,
    0b00000};

void setup()
{
    pinMode(Ligth_switch_pin, INPUT);
    pinMode(Fuel_level_pin, INPUT);
    pinMode(Speed_pin, INPUT);
    pinMode(RPM_pin, INPUT);
    pinMode(Temp_pin, INPUT);
    pinMode(Menu_switch_pin, INPUT);
    pinMode(encoderPin1, INPUT);
    pinMode(encoderPin2, INPUT);
    pinMode(hallPin_engine, INPUT);
    pinMode(hallPin_wheel, INPUT);
    pinMode(gas_pedal_pin, INPUT);
    pinMode(Fuel_gauge_pin, OUTPUT);

    lcd.begin(16, 2);    // set up number of columns and rows
    lcd.setCursor(0, 0); // move cursor to   (0, 0)

    lcd.createChar(0, up_arrow);
    lcd.createChar(1, return_arrow);
    lcd.createChar(2, sideways_arrow);
    Serial.begin(9600);
    lcd.clear();

    // rpm sensor setup
    attachInterrupt(digitalPinToInterrupt(hallPin_engine), rpm_engine_function, RISING); // Interrupt on hallPin, so that's where the hall sensor needs to be
    revolutions_engine = 0;
    rpm_engine = 0;
    timeold_engine = 0;

    attachInterrupt(digitalPinToInterrupt(hallPin_wheel), rpm_wheel_function, RISING); // Interrupt on hallPin, so that's where the hall sensor needs to be
    revolutions_wheel = 0;
    rpm_wheel = 0;
    timeold_wheel = 0;

    // servo setup
    throttle_servo.attach(9);

    // Sets the base values for the gokart so you dont need to set something if you just get into drive
    //  - No acceleration curve
    //  - No Max speed - set to an unobtainable value of 100mph
    //  - No max speed carb offset
    //  - No speed offset
    speed_offset = 0;
    max_speed_carb_offset = 0;
    speed_mode = speedmodes[3];
    aceleration_mode = 3;
}

void loop()
{
    // Button Stuff
    button.loop();
    int buttonpressed = button.isPressed();
    button.setDebounceTime(50); // set debounce time to 50 milliseconds
                                // encoder set up stuff
    static int pos = 0;
    encoder.tick();
    int newPos = encoder.getPosition();
    // serial print the button state
    if (buttonpressed == HIGH)
    {
        Serial.print("Button pressed\n");
    }
    // changes the current selection based on the encoder position

    if (newPos > pos)
    {
        Serial.print("up");
        if (menu == 2)
        {
            if (current_selection < 2)
            {
                current_selection = current_selection + 1;
            }
        }
        else if (current_selection < 4 && menu != 1)
        {
            current_selection = current_selection + 1;
        }
    }
    else if (newPos < pos)
    {
        Serial.print("down");
        if (current_selection > 1 && menu != 1)
        {
            current_selection = current_selection - 1;
        }
    }

    switch (menu)
    {
    case 1: // Home screen screen
        current_selection = 0;
        lcd.setCursor(0, 0);
        lcd.print(String(int(floor(Speed))) + " Mph");
        lcd.setCursor(0, 1);
        lcd.print(String(Temp) + " C");
        lcd.setCursor(8, 0);
        lcd.print(String(rpm_engine) + " RPM");
        lcd.setCursor(8, 1);
        lcd.print("Mode: " + String(speed_mode));
        if (buttonpressed == HIGH)
        { // If the button is pressed and the menu is the home screen then change to the settings menu
            menu = 2;
            lcd.clear();
            current_selection = 1;
        }
        break;
    case 2: // Settings Main menu
        lcd.setCursor(0, 0);
        lcd.print("Top Speed");
        lcd.setCursor(0, 1);
        lcd.print("Aceleration");
        if (buttonpressed == HIGH && current_selection == 1)
        { // If the button is pressed and the current selection is 1 then change to the speed settings menu
            menu = 3;
            lcd.clear();
            current_selection = 1;
        }
        if (buttonpressed == HIGH && current_selection == 2)
        { // If the button is pressed and the current selection is 2 then change to the aceleration settings menu
            menu = 4;
            lcd.clear();
            current_selection = 1;
        }
        break;
    case 3: // Speed Mode switching menu
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
        prevmenu = menu;

        break;

    case 4: // Aceleration settings menu
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
        prevmenu = menu; // sets the previous menu to the current menu so the slecection menu knows which setting to change as the selection code is used for both menus

        break;
    }

    // current selection stuff
    if (menu == 2 && pos != newPos)
    { // If the menu is the settings menu
        lcd.setCursor(12, 0);
        lcd.print(" ");
        lcd.setCursor(12, 1);
        lcd.print(" ");
        switch (current_selection)
        { // Arrow position
        case 1:
            lcd.setCursor(12, 0);
            lcd.write(byte(2));
            break;
        case 2:
            lcd.setCursor(12, 1);
            lcd.write(byte(2));
            break;
        }
    }
    else if ((menu != 2 || menu != 1) && pos != newPos)
    { // If the menu is not the settings menu or the home screen
        lcd.setCursor(5, 1);
        lcd.print(" ");
        lcd.setCursor(8, 1);
        lcd.print(" ");
        lcd.setCursor(11, 1);
        lcd.print(" ");
        lcd.setCursor(14, 1);
        lcd.print(" ");
        switch (current_selection)
        { // Arrow position
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
        case 4: // return arrow for the mode and aceleration settings menu
            lcd.setCursor(14, 1);
            lcd.write(byte(0));
            break;
        }
    }
    if (menu != 2 && menu != 1 && buttonpressed == HIGH) // If the menu is not the home screen or the settings menu and the button is pressed
    {
        if (prevmenu == 3 && current_selection != 4) // If the previous menu was the speed settings menu
        {
            speed_mode = speedmodes[current_selection];
            max_speed_carb_offset = max_speed_carb_offset_array[current_selection];

            Serial.print(speed_mode);
        }
        else if (prevmenu == 4 && current_selection != 4)
        {
            aceleration_mode = current_selection;

            Serial.print(aceleration_mode);
        }
        menu = 1;
        lcd.clear();
        current_selection = 1;
    }

    // serial print the encoder position
    if (pos != newPos)
    {
        // Serial.print(newPos);
        pos = newPos;
    }
    Serial.print(current_selection);

    /*Top Speed and acceleration code below this*/
    // rpm sensor
    if (revolutions_wheel >= 20)
    { // Update RPM every 20 counts, increase this for better RPM resolution, decrease for faster update
        // Calculate RPM for wheel sensor
        rpm_wheel = 60L * 1000L / (millis() - timeold_wheel) * revolutions_wheel;
        timeold_wheel = millis();
        revolutions_wheel = 0;
        // Calculate speed from RPM to MPH
        Speed = (rpm_wheel * 18 * 3.14159) / 63360.0; // calculate speed in miles per hour

        // Print RPM and speed to serial
        // Serial.println(rpm_wheel,DEC);
        // Serial.println(speed,DEC);
    }
    if (revolutions_engine >= 20)
    { // Update RPM every 20 counts, increase this for better RPM resolution, decrease for faster update
        // Calculate RPM for engine sensor
        rpm_engine = 60L * 1000L / (millis() - timeold_engine) * revolutions_engine;
        timeold_engine = millis();
        revolutions_engine = 0;
        // Serial.println(rpm_engine,DEC);
    }

    // Update acceleration based on Speed and acceleration mode
    if (aceleration_mode == 1)
    {
        float result = accelerationMode1();
        interval = result;
    }
    else if (aceleration_mode == 2)
    {
        float result = accelerationMode2();
        interval = result;
    }
    else if (aceleration_mode == 3)
    {
        float result = accelerationMode3();
        interval = result;
    }

    /*
    max_throttle_pos        //Tells the program what the max throttle value is (0-1023)
    min_throttle_pos        // tells the program what the min throttle value is (0-1023)
    max_carb_pos            // sets max carb position
    min_carb_pos            // sets min carb position
    throttle_percent        // sets the throttle percent to 0
    carb_pos = 0;           // sets the carb position to 0
    u                       // time in miliseconds per step at mph = 0
    a                       // constant for acceleration equation
    Q                       // Proportional control constant for throttle
    Speed;                  // Speed in MPH
    speed_offset            //point at which the max throttle will start to decrease
    max_speed_carb_offset   // offset for the max speed
    speed_mode              //what speed mode is being used (Top speed)
    */

    throttle = analogRead(gas_pedal_pin);                                     // reads the throttle value from the gas pedal from 1 - 1023
    new_throttle = map(throttle, min_throttle_pos, max_throttle_pos, 0, 100); // maps the throttle to the servo from min to max throttle

    if (Speed <= speed_mode - 10) // if the speed is 10mph use what ever the throttle is
    {
        carb_pos = map(new_throttle, 0, 100, min_carb_pos, max_carb_pos); // maps the throttle to the servo from min to max throttle
        moveServo(throttle_servo, carb_pos);                              // sets the carb sero to the new throttle value (carb_pos) using the moveservo function
    }
    else if (Speed > speed_mode - speed_offset && Speed <= speed_mode) // if the speed is within 10mph of max speed then use proportional control
    {
        carb_pos = (max_carb_pos - min_carb_pos + max_speed_carb_offset) / (pow(speed_mode - (speed_offset), Q) * (pow(Speed - (speed_mode - speed_offset), Q)) + max_carb_pos);
        carb_pos = ((new_throttle * 0.01) * (carb_pos - min_carb_pos) + min_carb_pos); // finds the new carb position based on the throttle percent
        // carb_pos = ... // finds the carb position at the max throttle value
        // carb_pos  = carb_pos * ... // finds the new carb position based on the throttle percent
        moveServo(throttle_servo, carb_pos); // sets the carb sero to the updated carb position using the moveservo function
    }
    else if (Speed > speed_mode)
    {
        throttle_servo.write(min_carb_pos + max_speed_carb_offset);
        // skips using the moveservo function and sets the carb position to the min carb position
        // when it gets above the max speed then adds the throttle offset to the max speed so that
        // the gokart will stay at the max speed instead of just closing the throttle completely
    }
}