#include <LiquidCrystal.h>
#include <Arduino.h>
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
const int Menu_switch_pin = 19; // Button
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
unsigned long currentMillis = 0;  // will store current time
unsigned long previousMillis = 0; // will store last time servo was updated
int currentPosition = 0;          // current position of the servo, set to 0 for initialization
unsigned int interval = 15;       // interval at which to move servo (milliseconds)
int targetPosition = 0;           // target position for the servo, Set at 0 for initialization
int buttonpressed_encoder = 0;    // button state of the encoder button
int pos = 0;                      // encoder position
int encoder_direction = 0;        // encoder direction
const unsigned long debounceTime = 200;

int menu = 1;
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


// RPM sensor stuff engine and at wheel
volatile byte revolutions_engine;
unsigned long timeold_engine;
int rpm_engine = 0;

volatile byte revolutions_wheel;
unsigned long timeold_wheel;
int rpm_wheel = 0;

int Timed_out_delay = 5;
int current_menu_value = 1; // Home screen = 1
int top_menu = 0;
int bottom_menu = 0;

struct MenuMap
{
    String MenuName;
    int menu_value;
    int menu_before;
    int menu_order;
    String menu_type;
    int Adjustable_Variable;
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

        if (currentpinval1 == 0 && currentpinval2 == 1 && PrevEnPin1 == 0 && PrevEnPin2 == 0)
        {
            pos++;
            // Serial.println(pos);
        }
        else if (currentpinval1 == 1 && currentpinval2 == 0 && PrevEnPin1 == 1 && PrevEnPin2 == 0)
        {
            pos--;
            // Serial.println(pos);
        }
        Serial.print(PrevEnPin1);
        Serial.print(PrevEnPin2);
        Serial.print(currentpinval1);
        Serial.print(currentpinval2);
        PrevEnPin1 = currentpinval1;
        PrevEnPin2 = currentpinval2;
    }
}

//  Menu Name, Menu Value, Menu Before, Menu Order, Menu Type , Adjustable Variable(if needed)
MenuMap Generated_menu[] = {
    {"Home Screen", 1, 0, 1, "Home Screen", NULL},
    {"Selection Screen", 2, 1, 1, "Selection", NULL},
    {"Acceleration screen", 3, 2, 1, "Adjustable Variable", aceleration_mode},
    {"Speed screen", 4, 2, 1, "Adjustable Variable", speed_mode}
    // Add more menus as needed
};

void displaymenu(int current_position,int menu_size)
{
if (Generated_menu[current_position].menu_type == "Home Screen")
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(String(int(floor(Speed))) + " Mph");
    lcd.setCursor(0, 1);
    lcd.print(String(Temp) + " C");
    lcd.setCursor(8, 0);
    lcd.print(String(rpm_engine) + " RPM");
    lcd.setCursor(8, 1);
    lcd.print("Mode: " + String(speed_mode));
    current_menu_value = Generated_menu[current_position].menu_value;
}
else if (Generated_menu[current_position].menu_type == "Selection")
{
    for (int j = 0; j < menu_size; j++)
    {
        if (Generated_menu[current_position].menu_value == Generated_menu[j].menu_before && Generated_menu[j].menu_order == top_menu)
        {
            top_menu = j;
        }
        if (Generated_menu[j].menu_order == top_menu + 1) {
            bottom_menu = j;
        }
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(Generated_menu[top_menu].MenuName);
    lcd.setCursor(0, 1);
    lcd.print(Generated_menu[bottom_menu].MenuName);
    current_menu_value = Generated_menu[current_position].menu_value;
}
else if (Generated_menu[current_position].menu_type == "Adjustable Variable")
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(Generated_menu[current_position].MenuName + " " + Generated_menu[current_position].Adjustable_Variable);
    current_menu_value = Generated_menu[current_position].menu_before; //not sure about this line
}

}

//Checks to make sure that the Name of the selected menu is in the menu list.
//Then the function will make sure that the menu can can be moved to from the 
//current menu by checking its menu_before value.
void SelectMenu(String Selected_Menu) { 
    int menu_size = sizeof(Generated_menu) / sizeof(Generated_menu[0]);
    for (int i = 0; i < menu_size; i++)
    {
        //Serial.print("Searching for menu\n");

        if (Generated_menu[i].MenuName == Selected_Menu)
        {
            if (Generated_menu[i].menu_before == current_menu_value)
            {
                displaymenu(i,menu_size);
                //sends the menu value and type to the display menu function
                //if the selected menu is in the menu and the current menu is before it
            }
            
            
            //break; //needs this break to stop the loop from continuing
        }
    } 
}

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

void encoderbuttoncheck() {
    static unsigned long lastInterruptTime = 0;
    unsigned long interruptTime = millis();
    if (interruptTime - lastInterruptTime > 50 && buttonpressed_encoder == 1) {
            buttonpressed_encoder = 0;
    }else if (interruptTime - lastInterruptTime > debounceTime && buttonpressed_encoder == 0) {
            buttonpressed_encoder = 1;
    }
    //Serial.print(buttonpressed_encoder);
    lastInterruptTime = interruptTime;
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

    attachInterrupt(digitalPinToInterrupt(Menu_switch_pin), encoderbuttoncheck, CHANGE);


    //Encoder interrupts
    attachInterrupt(digitalPinToInterrupt(encoderPin1), checkencoderpos, FALLING);
    attachInterrupt(digitalPinToInterrupt(encoderPin2), checkencoderpos, RISING);

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
    //button.setDebounceTime(50);
    pinMode(encoderPin1, INPUT);
    pinMode(encoderPin2, INPUT);
    pinMode(Menu_switch_pin, INPUT);
}

void loop()
{
    //Serial.println(pos);

    if (buttonpressed_encoder == HIGH && Generated_menu[current_menu_value].menu_value == 1)
    {
        SelectMenu("Selection Screen");
        top_menu = 1;
    }
    else if (buttonpressed_encoder == HIGH && Generated_menu[current_menu_value].menu_type == "Selection")
    {
        SelectMenu(Generated_menu[current_selection].MenuName);
    }
    else if (buttonpressed_encoder == HIGH && Generated_menu[current_menu_value].menu_type == "Adjustable Variable")
    {
        
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
}