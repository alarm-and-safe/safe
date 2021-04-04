/*Alarm and Safe Integration
Arduino code to control the safe and the security system
Code Author: Alex Dziak (ACD53@pitt.edu), Gareth Chapman(gjc26@pitt.edu) */
//include necessary libraries 
#include <Time.h>

#include <TimeLib.h>

#include <EEPROM.h>

#include <Wire.h>

#include <LiquidCrystal_I2C.h>

#include <Keypad.h>

#define Password_Length 5
//define the arduino pwm pins 
#define motor_fwd 44
#define motor_rev 46
#define fob_off 10
#define fob_home 11
#define fob_away 12
#define fob_panic 13 #define buzzer 45
//define all the variables
char MasterSafe1[Password_Length] = "1234";
char MasterSafe2[Password_Length] = "3456";
char MasterSafe3[Password_Length] = "0000";
//master safe password. change this to change the password. First Stored char MasterSecurity[Password_Length] = "5678"; 
//master security password. change this to change password. Second Stored char 

Data[Password_Length];
char MasterSecurity[Password_Length] = "5678";
//tracks the password being entered by user 
char action = 'N';
//the action chosen by the user (i.e. the 5th key entered) 
bool set_time = false;
//boolean for if we are currently setting the time

bool set_sec_pass = false;
//boolean for if we are currently setting the security passcode 
bool set_safe_pass = false;

//boolean for if we are currently setting the safe passcode 
char MasterTime[Password_Length] = "AAAA";
//master code to manage time
byte data_count = 0, time_count = 0, incorrect_count = 0, two_count = 0, four_count = 0;
//various counts used to manipulate arrays 
byte action_in = 0; 
//used as a boolean to know if the action has been entered char customKey; 
//key pressed by user from keypad const byte ROWS = 4; //keypad has 4 rows const byte COLS = 4; //keypad has 4 columns 
char hexaKeys[ROWS][COLS] = {
  //layout of the keypad 
  {
    '1',
    '2',
    '3',
    'A'
  },
  {
    '4',
    '5',
    '6',
    'B'
  },
  {
    '7',
    '8',
    '9',
    'C'
  },
  {
    '*',
    '0',
    '#',
    'D'
  }
};
byte rowPins[ROWS] = {
  5,
  4,
  3,
  2
};
//connect to the row pinouts of the keypad 
byte colPins[COLS] = {
  9,
  8,
  7,
  6
};
//connect to the col pinouts of the keypad 
String attempts[50][2];
// track the attempts
// column 1 = type of access (incorrect, unlock, lock, off, home, away, panic)
// column 2 = time of access (hour.minute.second.day.month.year) int attempt_row = 0;
// a count for the current row to edit
char v_min[3], v_hr[3], v_sec[3], v_day[3], v_month[3], v_yr[5];
// arrays for time elements time_t backlight_timer;
//library methods that set up the keypad and the lcd screen
Keypad customKeypad = Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
LiquidCrystal_I2C lcd(0x27, 16, 2);
//setup runs once when the arduino starts. gets everything ready 
void setup() { //Passcode
  if (EEPROM.read(0) != 0) {
    EEPROM.get(0, MasterSafe1);
    EEPROM.get(sizeof(MasterSafe1), MasterSafe2);
    EEPROM.get(sizeof(MasterSafe1) + sizeof(MasterSafe2), MasterSafe3);

  }

  if (EEPROM.read(sizeof(MasterSafe1) + sizeof(MasterSafe2) + sizeof(MasterSafe3)) != 0) {

    EEPROM.get(sizeof(MasterSafe1) + sizeof(MasterSafe2) + sizeof(MasterSafe3), MasterSecurity);

  }

  if (EEPROM.read(sizeof(MasterSafe1) + sizeof(MasterSafe2) + sizeof(MasterSafe3) + sizeof(MasterSecurity)) != 0) {

    incorrect_count = int(EEPROM.read(sizeof(MasterSafe1) + sizeof(MasterSafe2) + sizeof(MasterSafe3) + sizeof(MasterSecurity)));

  }
  if (EEPROM.read(sizeof(MasterSafe1) + sizeof(MasterSafe2) + sizeof(MasterSafe3) + sizeof(MasterSecurity) + 1) != 0) {

    EEPROM.get(sizeof(MasterSafe1) + sizeof(MasterSafe2) + sizeof(MasterSafe3) + sizeof(MasterSecurity) + 1, attempts);
    
  }

  lcd.init();

  lcd.backlight();
  setTime(0, 0, 0, 1, 1, 2000);
  // setTime(hr,min,sec,day,mnth,yr);
  // default to this, but user should change to be correct
  // eventually would want to sync this to a pc or the internet in future iterations 
  //set the pins for the arduino 
  pinMode(motor_fwd, OUTPUT);
  pinMode(motor_rev, OUTPUT);
  pinMode(fob_away, OUTPUT);
  pinMode(fob_home, OUTPUT);
  pinMode(fob_off, OUTPUT);
  pinMode(fob_panic, OUTPUT);
  pinMode(buzzer, OUTPUT); //Starts the Serial moniter
  Serial.begin(9600);
}

//loop runs over and over while the arduino is powered 
void loop() {
  if (second(backlight_timer) - second(now()) > second(180)) {
    lcd.noBacklight();
  }
  // This Does nothing?/\ Removed //
  //lcd prompt 
  if (set_time == true) { //show prompt to enter the time because we are setting the time   
    lcd.setCursor(0, 0);
    lcd.print("Enter Time:");
    lcd.setCursor(0, 2);
    lcd.print("Hr:Mn:Sc:Dy:Mo:Year");
  } else if (set_safe_pass == true || set_sec_pass == true) {
    lcd.setCursor(0, 0);
    lcd.print("Enter New Password:");
  } else { //otherwise prompt to enter a password   
    lcd.setCursor(0, 0);
    lcd.print("Enter Password:");
  }
  //record what buttons have been pressed on the 
  keypad customKey = customKeypad.getKey();

  if (customKey) { //keypad button is pressed    
    lcd.backlight();
    backlight_timer = now();
    Serial.println(customKey);
    if (data_count < Password_Length - 1 && set_time == false && set_sec_pass == false && set_safe_pass == false) { //collect the 4 password characters      
      Data[data_count] = customKey;
      lcd.setCursor(data_count, 1);
      lcd.print(Data[data_count]);
      data_count++;
      action = 'N';
    } else if (set_time == false && set_sec_pass == false && set_safe_pass == false) { //collect the 'action' character on the keypad (comes after the password)      
      action = customKey;
      lcd.setCursor(data_count + 1, 1);
      lcd.print(action);
      action_in = 1;
    } else if (set_time == true && set_sec_pass == false && set_safe_pass == false) { //we are setting the time     
      lcd.setCursor(time_count, 1);
      lcd.print(customKey);
      settingTime();
    } else if (set_sec_pass == true) { //we are setting the security passcode     
      lcd.setCursor(data_count, 1);
      lcd.print(customKey);
      settingSecurity();
    } else if (set_safe_pass == true) {
      //we are setting the safe passcode     
      lcd.setCursor(data_count, 1);
      lcd.print(customKey);
      settingSafe();
    }

  }

  //if the password and an action have been selected (i.e. total of 5 characters) 
  if (data_count == Password_Length - 1 && action_in == 1) {
    lcd.clear();
    Serial.println(Data);
    //if the password matches the safe's password   

    if (strcmp(Data, MasterSafe1) == 0 || strcmp(Data, MasterSafe2) == 0 || strcmp(Data, MasterSafe3) == 0) {
      incorrect_count = 0; //reset count that is checking for 3 successive incorrect passcodes 
      EEPROM.put(sizeof(MasterSafe1) + sizeof(MasterSafe2) + sizeof(MasterSafe3) + sizeof(MasterSecurity), incorrect_count);
      Serial.println("Correct,Safe");
      if (action == '*') { //action was to lock the safe       
        recordAttempt(1); //record that action       
        lcd.clear();
        lcd.print("Locking Safe");
        Serial.println("Locking Safe");
        digitalWrite(motor_fwd, HIGH); //this might have to be motor_rev based on which way the motor is oriented
        delay(3000); //may have to adjust time to fully move mechanism //in milliseconds
        digitalWrite(motor_fwd, LOW); //stop the motor       
        delay(2000);
      }
      if (action == '#') { //action is to unlock safe       
        recordAttempt(2);
        lcd.clear();
        lcd.print("Unlocking Safe");
        Serial.println("Unlocking Safe");
        digitalWrite(motor_rev, HIGH); //this might have to be motor_fwd based on which way the motor is oriented 
        delay(3000); //may have to adjust time to fully move mechanism       
        digitalWrite(motor_rev, LOW); //stop the motor       
        delay(2000);
      }
      if (action == '0') {
        set_safe_pass = true;
      }
      if (action != '#' && action != '*' && action != '0') { //password wasn't wrong but it wasn't an action button that would do anything     
        lcd.clear();
        lcd.print("Invalid Request");
        delay(2000);
        Serial.println("Invalid Request");
      }
    }
    if (strcmp(Data, MasterSecurity) == 0) { //passcode is for security system incorrect_count = 0;
      Serial.println("Correct, Security");
      if (action == 'A') { // turn the security to off       
        recordAttempt(3);
        lcd.clear();
        lcd.print("Security Off");
        Serial.println("Security Off");
        digitalWrite(fob_off, HIGH);
        delay(4000);
        digitalWrite(fob_off, LOW);
        delay(2000);
      }
      if (action == 'B') { // turn the security to home       recordAttempt(4);       lcd.clear();
        lcd.print("Security Home");
        Serial.println("Security Home");
        digitalWrite(fob_home, HIGH);
        delay(4000);
        digitalWrite(fob_home, LOW);
        delay(2000);
      }
      if (action == 'C') { // turn the security to away
        recordAttempt(5);
        lcd.clear();
        lcd.print("Security Away");
        Serial.println("Security Away");
        digitalWrite(fob_away, HIGH);
        delay(4000);
        digitalWrite(fob_away, LOW);
        delay(2000);
      }
      if (action == 'D') { // turn the security to panic       recordAttempt(6);       lcd.clear();
        lcd.print("Security Panic");
        Serial.println("Security Panic");
        digitalWrite(fob_panic, HIGH);
        delay(4000);
        digitalWrite(fob_panic, LOW);
        delay(2000);
      }
      if (action == '0') {
        Serial.println("0 selected");
        set_sec_pass = true;
      }
      if (action == '*' || action == '#') { // password was right but action was invalid       
        lcd.clear();
        lcd.print("Invalid Request");
        delay(2000);
        Serial.println("Invalid Request");
      }
    }
    //passcode is to manage the time   
    if (strcmp(Data, MasterTime) == 0) {
      if (action == 'A') { // set the time 
        Serial.println("Set hr:min:sec day:mo:yr");
        set_time = true;
      }
      if (action == 'B') { // printing the last 50 attempts to the serial monitor and the last 4 to the LCD
        Serial.println("Attempts History: ");
        printAttemptsSerial();
        printAttemptsLCD();
      }
    }
    if (strcmp(Data, MasterSecurity) != 0 && strcmp(Data, MasterSafe1) != 0 && strcmp(Data, MasterSafe2) && strcmp(Data, MasterSafe3) && strcmp(Data, MasterTime) != 0) {

      // password was wrong      
      recordAttempt(0);
      incorrect_count++;
      EEPROM.put(sizeof(MasterSafe1) + sizeof(MasterSafe2) + sizeof(MasterSafe3) + sizeof(MasterSecurity), incorrect_count);
      lcd.clear();
      lcd.print("Incorrect");
      Serial.println("Incorrect");
      delay(2000);
    }

    lcd.clear();
    clearData();
    data_count = 0;
    action = 'N';
    action_in = 0;
    if (incorrect_count >= 3) { // disable the keypad if 3 consecutive incorrect attempts are made 
      Serial.println("Disabled");
      lcd.clear();
      lcd.print("Disabled");
      digitalWrite(buzzer, HIGH);
      //activate the buzzer to scream as well as audible alert
      //added security: could also trigger the panic feature of the security system       
      delay(10000);
      digitalWrite(buzzer, LOW);
      delay(2000);
      incorrect_count = 0;
    }
  }
}
//function to clear the data entered 
void clearData() {
  while (data_count != 0) {
    Data[data_count--] = 0;
  }

  return;
}
//function for setting a new security passcode 
void settingSecurity() {
  Serial.println("setting sec code");
  MasterSecurity[data_count] = customKey;
  data_count++;
  if (data_count == 4) {
    clearData();
    data_count = 0;
    set_sec_pass = false;
    lcd.clear();
  }
  EEPROM.put(sizeof(MasterSafe1) + sizeof(MasterSafe2) + sizeof(MasterSafe3), MasterSecurity);
  return;
}
//function for setting a new safe passcode 
void settingSafe() {
    if ((strcmp(Data, MasterSafe1) == 0) {
        MasterSafe1[data_count] = customKey;
        EEPROM.put(0, MasterSafe1); //aaa
      }

      if ((strcmp(Data, MasterSafe2) == 0) {
          MasterSafe2[data_count] = customKey;
          EEPROM.put(sizeof(MasterSafe1), MasterSafe2);
        }

        if ((strcmp(Data, MasterSafe3) == 0) {
            MasterSafe3[data_count] = customKey;
            EEPROM.put(sizeof(MasterSafe1) + sizeof(MasterSafe2), MasterSafe3);
          }
          data_count++;
          if (data_count == 4) {
            clearData();
            data_count = 0;
            set_safe_pass = false;
            lcd.clear();
          }
          return;
        }
        //function for user to set the current time 
        void settingTime() {
          if (time_count == 0 || time_count == 1) { //set the hour     v_hr[two_count] = customKey;     time_count ++;     two_count ++;
            if (two_count == 2) two_count = 0;
          } else if (time_count == 2 || time_count == 3) { //set the minute       v_min[two_count] = customKey;       time_count ++;
            two_count++;
            if (two_count == 2) two_count = 0;
          } else if (time_count == 4 || time_count == 5) { //set the second       
            v_sec[two_count] = customKey;
            time_count++;
            two_count++;
            if (two_count == 2) two_count = 0;
          } else if (time_count == 6 || time_count == 7) { //set the day 
            v_day[two_count] = customKey;
            time_count++;
            two_count++;
            if (two_count == 2) two_count = 0;
          } else if (time_count == 8 || time_count == 9) { //set the month
            v_month[two_count] = customKey;
            time_count++;
            two_count++;
            if (two_count == 2) two_count = 0;
          } else if (time_count == 10 || time_count == 11 || time_count == 12 || time_count == 13) { //set the year
            v_yr[four_count] = customKey;
            time_count++;
            four_count++;
            if (four_count == 4) { //done with setting time, so set it using time library function         
              setTime(atoi(v_hr), atoi(v_min), atoi(v_sec), atoi(v_day), atoi(v_month), atoi(v_yr));
              //setTime(hr,min,sec,day,mnth,yr); 
              //setting it will allow the arduino to start counting time from this point onward
              set_time = false;
              time_count = 0;
              four_count = 0;
              Serial.print(day());
              Serial.print("/");
              Serial.print(month());
              Serial.print("/");
              Serial.print(year());
              Serial.print(" ");
              Serial.print(hour());
              Serial.print(":");
              Serial.print(minute());
              Serial.print(":");
              Serial.print(second());
              lcd.clear();
              lcd.print("Time set");
              delay(2000);
            }
          }
          return;
        }
        //function to record an attempt (type and day/time) void recordAttempt(int type)
        {
          String curr_date = createDate();
          if (attempt_row == 50) attempt_row = 0;
          if (type == 0) {
            attempts[attempt_row][0] = "incorrect";
            attempts[attempt_row][1] = curr_date;
            EEPROM.put(sizeof(MasterSafe1) + sizeof(MasterSafe2) + sizeof(MasterSafe3) + sizeof(MasterSecurity) + 1, attempts);
          } else {
            if (type == 1) attempts[attempt_row][0] = "unlock";
            else if (type == 2) attempts[attempt_row][0] = "lock";
            else if (type == 3) attempts[attempt_row][0] = "off";
            else if (type == 4) attempts[attempt_row][0] = "home";
            else if (type == 5) attempts[attempt_row][0] = "away";
            else if (type == 6) attempts[attempt_row][0] = "panic";
            attempts[attempt_row][1] = curr_date;
            EEPROM.put(sizeof(MasterSafe1) + sizeof(MasterSafe2) + sizeof(MasterSafe3) + sizeof(MasterSecurity) + 1, attempts);
          }
          Serial.println("Writing");
          Serial.println(attempts[attempt_row][0]);
          Serial.println(attempts[attempt_row][1]);
          Serial.println(curr_date);
          attempt_row++;
        }
        //function to put the current date in string format 
        String createDate() {
          //hr:min:sec day:mo:yr
          String date_string = (String) hour();
          date_string.concat(".");
          date_string.concat((String) minute());
          date_string.concat(".");
          date_string.concat((String) second());
          date_string.concat(".");
          date_string.concat((String) day());
          date_string.concat(".");
          date_string.concat((String) month());
          date_string.concat(".");
          date_string.concat((String) year());
          return date_string;
        }
        //function to print the attempts history to the serial monitor 
        void printAttemptsSerial() {
          for (int i = 0; i < 50; i++) {
            Serial.print(attempts[i][0]);
            Serial.print(" at ");
            Serial.print(attempts[i][1]);
            Serial.println();
          }
        }
        void printAttemptsLCD() {
          // most recent = mr = attempt row -1
          // if mr >= 3 && mr <= 49 then the last 4 attempts are indices mr, mr - 1, mr - 2, and mr - 2 // if mr = 2 then the last 4 attempts are indices 2, 1, 0, and 49
          // if mr = 1 then the last 4 attempts are indices 1, 0, 49, and 48 // if mr = 0 then the last 4 attempts are indices 0, 49, 48, and 47 int mr = attempt_row - 1;
          // print the last 4 attempts on the LCD screen for 3 seconds each starting with most recent 
          for (int i = 0; i < 4; i++) {
            printOneAttempt(mr);
            if (mr == 0) mr = 50;
            mr--;
          }
        }
        void printOneAttempt(int mr) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print(attempts[mr][0]);
          lcd.setCursor(0, 1);
          lcd.print("Hr.Mn.Sc.Dy.Mo.Year");
          lcd.setCursor(0, 2);
          lcd.print(attempts[mr][1]);
          delay(3000);
        }