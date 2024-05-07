#include <LiquidCrystal.h>
#include <Arduino.h>
#include <pins_arduino.h>
#include <servo.h>
#include <LiquidCrystal_I2C.h>
#include <ezButton.h>


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
const int Encoder_button = 19; // Button
const int encoderPin1 = 18;    // Rotary encoder
const int encoderPin2 = 20;    // Rotary encoder
const int hallPin_engine = 7;  // Hall effect sensor pin for engine
const int hallPin_wheel = 8;   // Hall effect sensor pin for wheel
const int gas_pedal_pin = A4;  // Gas pedal pin
int Light_Pin_upper = 15;      // LED pin, controls a relay for the lights
int Light_Pin_lower = 16;      // LED pin, controls a relay for the lights
int Light_Pin_back = 17;       // LED pin, controls a relay for the lights
int Fuel_gauge_pin = 7;        // Fuel gauge pin -  make sure its a PWM pin

// Global variables
unsigned long currentMillis = 0;        // will store current time
unsigned long previousMillis = 0;       // will store last time servo was updated
int currentPosition = 0;                // current position of the servo, set to 0 for initialization
unsigned int interval = 15;             // interval at which to move servo (milliseconds)
int targetPosition = 0;                 // target position for the servo, Set at 0 for initialization
volatile int buttonpressed_encoder = 0; // button state of the encoder button
volatile int pos = 0;
volatile int pos_old = 0;  // encoder position
int encoder_direction = 0; // encoder direction
const unsigned long debounceTime = 200;

int menu = 1;
float Speed = 0;          // Speed in MPH
int Temp = 0;             // Temperature of the coolent
int speed_mode = 0;       // what speed mode is being used, initilaizes it as 0
int aceleration_mode = 0; // What acceleration mode is being used
int current_selection = 0;
float throttle = 0;
int Encoder_Btn_already_pressed = 0;
int current_mode_selection_value = 0;

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

// RPM sensor stuff engine and at wheel
volatile byte revolutions_engine;
unsigned long timeold_engine;
int rpm_engine = 0;

volatile byte revolutions_wheel;
unsigned long timeold_wheel;
int rpm_wheel = 0;

int Timed_out_delay = 5;
int current_menu_value = 0; // Home screen = 1
int top_menu = 0;
int bottom_menu = 0;

ezButton Enc_Btn(Encoder_button);

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

struct MenuMap
{
    String MenuName;
    int menu_value;
    int menu_before;
    int menu_order;
    String menu_type;
    int* Adjustable_Variable;
    int Menus_under; // Number of menus under it, this is for the selection screens
    int max_range;
};

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

void checkencoderpos()
{
    static unsigned long lastInterruptTime = 0;
    unsigned long interruptTime = millis();
    if (interruptTime - lastInterruptTime > 5)
    {
        static int PrevEnPin1 = 0;
        static int PrevEnPin2 = 0;
        int currentpinval1 = digitalRead(encoderPin1);
        int currentpinval2 = digitalRead(encoderPin2);
        if (currentpinval2 != currentpinval1)
        {
            pos_old = pos;
            pos++;
        }
        else
        {
            pos_old = pos;
            pos--;
        }
        PrevEnPin1 = currentpinval1;
        PrevEnPin2 = currentpinval2;
    }
}

//  Menu Name, Menu Value, Menu Before, Menu Order, Menu Type , Adjustable Variable(if needed) , Number of menus under it(selection screens), Max number for the adjustable variable(set to 2 for ON and OFF)
//  Menu Values start at zero, Home screen should be 0
MenuMap Generated_menu[] = {
    {"Home Screen", 0, NULL, 1, "Home Screen", NULL, NULL, NULL},
    {"Selection Screen", 1, 0, 1, "Selection", NULL, 3, NULL},
    {"Acceleration", 2, 1, 1, "Adjustable Variable", &aceleration_mode, NULL, 3},
    {"Speed", 3, 1, 2, "Adjustable Variable", &speed_mode, NULL, 3},
    {"Light Menu", 4, 1, 3, "Selection", NULL, 3, NULL},
    {"Front lights", 5, 4, 1, "Adjustable Variable", NULL, NULL, 2},
    {"Fog Lights", 6, 4, 2, "Adjustable Variable", NULL, NULL, 2},
    {"Rear Lights", 7, 4, 3, "Adjustable Variable", NULL, NULL, 2},
    {"Back", 8, 1, 4, "Back", NULL, NULL, NULL},
    {"Back", 8, 4, 4, "Back", NULL, NULL, NULL}
    // Add more menus as needed
};

void displaymenu(int current_position, int menu_size)
{
    static int prevmenu = current_menu_value;
    int bottom_menu_j = bottom_menu;
    int top_menu_j = top_menu;

    if (Generated_menu[current_position].menu_type == "Home Screen")
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(String(int(floor(Speed))));
        lcd.setCursor(5, 0);
        lcd.print("Mph");
        lcd.setCursor(0, 1);
        lcd.print(String(rpm_engine));
        lcd.setCursor(5, 1);
        lcd.print("RPM");
        lcd.setCursor(9, 0);
        lcd.print(String(Temp));
        lcd.setCursor(13, 0);
        lcd.print("C");
        lcd.setCursor(9, 1);
        lcd.print("S");
        lcd.setCursor(10, 1);
        lcd.print(String(speed_mode));
        lcd.setCursor(13, 1);
        lcd.print("A");
        lcd.setCursor(14, 1);
        lcd.print(String(aceleration_mode));


        current_menu_value = Generated_menu[current_position].menu_value;
        // Serial.println("Menu Displayed: " + String(Generated_menu[current_position].MenuName));
        // Serial.println("Current Menu Value: " + String(current_menu_value));
    }
    else if (Generated_menu[current_position].menu_type == "Selection")
    {
        // Serial.println("top menu: " + String(top_menu_j));
        current_menu_value = Generated_menu[current_position].menu_value;
        // Serial.println("Menu Displayed: " + String(Generated_menu[current_position].MenuName));
        // Serial.println("Current Menu Value: " + String(current_menu_value));
        // Serial.println("Current position from input: " + String(current_position));
        // Serial.println("Searching for Top menu");
        for (int j = 0; j < menu_size; j++)
        {
            // Serial.println("j value: " + String(j));
            // Serial.println("Menu name from current position of J: " + Generated_menu[j].MenuName);
            // Serial.println("Menu before value of menu from J: " + String(Generated_menu[j].menu_before));
            if (Generated_menu[current_position].menu_value == Generated_menu[j].menu_before)
            {
                // Serial.println("inside first if");
                // Serial.println("menu order of current j value: " + String(Generated_menu[j].menu_order));
                if (Generated_menu[j].menu_order == top_menu_j)
                {
                    // Serial.println("Current Top menu name: " + String(Generated_menu[top_menu_j].MenuName));
                    top_menu_j = j;
                    current_selection = j;
                    // Serial.println("new top menu: " + String(top_menu_j));
                    break;
                }
            }
        }
        // Serial.println("Searching for Bottom menu");
        for (int j = 0; j < menu_size; j++)
        {
            // Serial.println("j value: " + String(j));
            // Serial.println("Menu name from current position of J: " + Generated_menu[j].MenuName);
            // Serial.println("Menu before value of menu from J: " + String(Generated_menu[j].menu_before));
            if (Generated_menu[current_position].menu_value == Generated_menu[j].menu_before)
            {
                // Serial.println("inside first if");
                // Serial.println("menu order of current j value: " + String(Generated_menu[j].menu_order));

                if (Generated_menu[j].menu_order == Generated_menu[top_menu_j].menu_order + 1)
                {
                    bottom_menu_j = j;
                    // Serial.println("new bottom menu: " + String(bottom_menu_j));
                    break;
                }
                else
                {
                    bottom_menu_j = NULL;
                }
            }
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(Generated_menu[top_menu_j].MenuName);
        lcd.setCursor(0, 1);
        if (bottom_menu_j != NULL)
        {
            lcd.print(Generated_menu[bottom_menu_j].MenuName);
        }
        else
        {
            lcd.write(" ");
        }
        lcd.setCursor(14, 0);
        lcd.write(2);
    }

    else if (Generated_menu[current_position].menu_type == "Adjustable Variable")
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(Generated_menu[current_position].MenuName + " ");
        *Generated_menu[current_position].Adjustable_Variable = current_mode_selection_value;
        current_menu_value = Generated_menu[current_position].menu_value;

      if (Generated_menu[current_menu_value].max_range == 2){
          if (current_mode_selection_value == 2) {
          lcd.print("ON");
          }else if (current_mode_selection_value == 1){
          lcd.print("OFF");
          }
      } else {
        lcd.print(*Generated_menu[current_position].Adjustable_Variable);
      }
      //Serial.println(current_mode_selection_value);
    }
    else if (Generated_menu[current_position].menu_type == "Back")
    {
        prevmenu = Generated_menu[current_menu_value].menu_before;
        top_menu = 1;
        SelectMenu(Generated_menu[prevmenu].MenuName);
        Encoder_Btn_already_pressed = 1;
        //Serial.println(Generated_menu[prevmenu].MenuName);
        //Serial.println(prevmenu);
    }
    // prevmenu = Generated_menu[current_position].menu_value;
}

// Checks to make sure that the Name of the selected menu is in the menu list.
// Then the function will make sure that the menu can can be moved to from the
// current menu by checking its menu_before value.
void SelectMenu(String Selected_Menu)
{
    int menu_size = sizeof(Generated_menu) / sizeof(Generated_menu[0]);
    for (int i = 0; i < menu_size; i++)
    {
        // Serial.print("Searching for menu\n");
        // Serial.println("Searching for menu...   Currently at: "  + String(Generated_menu[i].MenuName));

        if (Generated_menu[i].MenuName == Selected_Menu)
        {
            // Serial.println("Menu Found: " + String(Generated_menu[i].MenuName));
            // Serial.println("Selected menu " + String(Selected_Menu));
            // Serial.println("required prev menu " + String(current_menu_value));
            // Serial.println("Current prev menu " + String(Generated_menu[i].menu_before));
            // if (Generated_menu[i].menu_before == current_menu_value || Generated_menu[i].menu_before == NULL)
            //{
            // Serial.println("Attempted to display " + String(Generated_menu[i].MenuName));
            displaymenu(i, menu_size);
            return;
            // sends the menu value and type to the display menu function
            // if the selected menu is in the menu and the current menu is before it
            //}

            // Serial.println("Incorrect previous menu");
            // return;
        }
    }
    Serial.print("Could Not Find menu");
    return;
}

void updatehomescreen() {
        //print new values
        lcd.setCursor(0, 0);
        lcd.print(String(int(floor(Speed))));
        lcd.setCursor(0, 1);
        lcd.print(String(rpm_engine));
        lcd.setCursor(9, 0);
        lcd.print(String(Temp));
        lcd.setCursor(10, 1);
        lcd.print(String(speed_mode));
        lcd.setCursor(14, 1);
        lcd.print(String(aceleration_mode));
}

void setup()
{
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

    attachInterrupt(digitalPinToInterrupt(hallPin_wheel), rpm_wheel_function, CHANGE); // Interrupt on hallPin, so that's where the hall sensor needs to be
    revolutions_wheel = 0;
    rpm_wheel = 0;
    timeold_wheel = 0;

    // attachInterrupt(digitalPinToInterrupt(Menu_switch_pin), encoderbuttoncheck, CHANGE);

    // Encoder interrupts
    attachInterrupt(digitalPinToInterrupt(encoderPin1), checkencoderpos, RISING);
    // attachInterrupt(digitalPinToInterrupt(encoderPin2), checkencoderpos, CHANGE);

    // servo setup
    throttle_servo.attach(9);

    // Sets the base values for the gokart so you dont need to set something if you just get into drive
    //  - No acceleration curve
    //  - No Max speed - set to an unobtainable value of 100mph
    //  - No max speed carb offset
    //  - No speed offset
    speed_offset = 0;
    max_speed_carb_offset = 0;
    speed_mode = 3; //array starts at 0 not 1 so 2 = 3
    aceleration_mode = 3;
    // button.setDebounceTime(50);
    pinMode(encoderPin1, INPUT);
    pinMode(encoderPin2, INPUT);
    pinMode(Encoder_button, INPUT_PULLUP);
    current_menu_value = 1;
    SelectMenu("Home Screen");
    top_menu = 0;
    current_mode_selection_value = 1;

    Enc_Btn.setDebounceTime(50);
    //Serial.print(speed_mode);
}

void loop()
{
    Enc_Btn.loop();
    int Encoder_Btn_pressed = Enc_Btn.isPressed();
    Encoder_Btn_already_pressed = 0;
    //Serial.println(Enc_btn_count);
    //Serial.println("current: " + String(Enc_btn_count));
    if (Generated_menu[current_menu_value].menu_type == "Home Screen") {
      updatehomescreen();
    }


    if (Encoder_Btn_pressed == HIGH && Generated_menu[current_menu_value].menu_type == "Home Screen" && Encoder_Btn_already_pressed == 0)
    {
        // Serial.println("Button Pressed:  Attempting to go to Selection Screen");
        top_menu = 1; // needs to be before the selection screen
        SelectMenu("Selection Screen");
        Encoder_Btn_already_pressed = 1;
        // Serial.println("Top Menu: " + String(top_menu));
    }

    if (Encoder_Btn_pressed == HIGH && Generated_menu[current_menu_value].menu_type == "Selection" && Encoder_Btn_already_pressed == 0)
    {
        top_menu = 1;
        SelectMenu(Generated_menu[current_selection].MenuName);
        Encoder_Btn_already_pressed = 1;
    }


    if (Generated_menu[current_menu_value].menu_type == "Selection" && pos != pos_old)
    {
        if (pos > pos_old)
        {
            if (top_menu < Generated_menu[current_menu_value].Menus_under + 1)
            {
                top_menu++; // Tells the menu system what the top menu value is, this counts up from 1. This is not a position value overall but where in the selection menu the top menu is.
            }
        }
        else if (pos < pos_old)
        {
            if (top_menu > 1)
            {
                top_menu--;
            }
        }
        SelectMenu(Generated_menu[current_menu_value].MenuName);
        // Serial.println("Top Menu: " + String(top_menu));
        // Serial.println(Generated_menu[current_menu_value].MenuName);
        // Serial.println(pos);
    }

    if (Generated_menu[current_menu_value].menu_type == "Adjustable Variable" && pos != pos_old)
    {
      current_mode_selection_value = *Generated_menu[current_menu_value].Adjustable_Variable;
        if (pos > pos_old)
        {
            if (current_mode_selection_value < Generated_menu[current_menu_value].max_range)
            {
                current_mode_selection_value++;
            }
        }
        else if (pos < pos_old)
        {
            if (current_mode_selection_value > 1)
            {
                current_mode_selection_value--;
            }
        }
        SelectMenu(Generated_menu[current_menu_value].MenuName);
        Serial.println(aceleration_mode);
    }

    if (Generated_menu[current_menu_value].menu_type == "Adjustable Variable" && Encoder_Btn_pressed == HIGH && Encoder_Btn_already_pressed == 0) {
    
        SelectMenu(Generated_menu[Generated_menu[current_menu_value].menu_before].MenuName);
    }
    /*
    //Top Speed and acceleration code below this
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


    Serial.print(String("menu: ") + menu + String("     prevmenu: ") + prevmenu + "\n");
    */
    pos_old = pos;
}