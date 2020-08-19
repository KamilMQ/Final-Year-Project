/* 

*/

//Libraries
#include <DHT.h> //Temp & Humidity sensor
#include <LiquidCrystal.h> //Includes ability to manipulate the LCD screen
#include <EEPROM.h> //Includes ability to manipulate/read internal storage
#include <GSM.h> //Includes the ability to mkae use of and control the GSM Shield used for SMS

//Constants
#define DHTPIN 9     // The Pin DHT is connected to
#define DHTTYPE DHT22   // DHT 22  (AM2302)
#define rad A4            // The Pin Radiator would be connected to
#define lightLED A5        //Testing Pin with a green LED
#define resetButton A1     //Reset button connected to pin A1
#define MAXTEMP 50         //EEPROM location of Max Temperature
#define MINTEMP 51         //EEPROM location of Minimum Temperature
#define PINNUMBER ""       //No PIN on the simcard 
#define TIMER 52           //EEPROM location of timer length
#define FIRSTTIME 53       //EEPROM location of whether this is the first time the Arduino is set up.
                           //A value of 0 is used for true, as it shows a clear EEPROM

DHT dht(DHTPIN, DHTTYPE); //// Initialize DHT sensor
GSM gsmAccess; //Initilising the GSM library
GSM_SMS sms;   //Initialising the SMS portion of GSM library


const int rs = 12, en = 11, d4 = 8, d5 = 6, d6 = 5, d7 = 4;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

//Variables
float hum;  //Stores humidity value
float temp; //Stores temperature value
bool radOn; //Stores whether radiator is on or not
bool liOn; //Stores whether the light output is on or not
bool manual; //Stores whether manual control of radiator is enabled
char ownerNumber[20]; //Stores the paired phone number
char senderNumber[20]; //Stores the phone number of the recieved SMS message
String ownerStringNumber;
unsigned long timeNow; // Stores the accurate time that will let us control the length of certain functions without stopping the rest of the code
unsigned long timeOfCommand; //Stores the time when the command was fullfilled.
unsigned long interval; //This is interval, defaulted to 15 mins
int maxTemp, minTemp; //Store the long term temperature limits
char ch; // Stores the individual message bytes.
String message; //Stores the SMS message
// INTERVAL 15 WAS CHANGED TO 60 SECONDS FOR DEMO PURPOSES

/*IMPORTANT EEPROM VALUES:
 * EEPROM 0 - Length of paired phone number. A value of 0 signifies no paired number. Phone number stored immediately afterwards.
 * EEPROM 50 - Max Temperature as set by user. Is preloaded to be 20C on first startup.
 * EEPROM 51 - Min Temperature as set by user. Is preloaded to be 16C on first startup.
 * EEPROM 52 - Timer for manual control. Defaults to 90 seconds for the demo but can be modified to anything up to 255 mins.
 */
void setup()
{
  Serial.begin(9600); //Init the arduino at 9600 baud rate
  if(EEPROM.read(FIRSTTIME) == 0){
    EEPROM.write(MAXTEMP,30);
    EEPROM.write(MINTEMP,26);
    EEPROM.write(TIMER,15);
    EEPROM.write(FIRSTTIME,1);
  }
  dht.begin(); //Starts up the DHT sensor (temp & hum)
  initValues();
  pinMode(rad, OUTPUT); //Declares the rad pin as Output
  pinMode(lightLED, OUTPUT); //Declares the lightLED pin as Output
  pinMode(resetButton, INPUT);
  radOn = false; //Sets the default radiator output to be off
  lcd.begin(16,2);
  lcd.print("Hello World");
  GSMConnect();
  sms.flush();
}

void loop()
{
  //Using delay instead of millis for power saving. The program doesn't need to be ran every 2 seconds exactly
    delay(2000); 
    timeNow = millis();
    //Read data and store it to variables hum and temp
    hum = dht.readHumidity();
    temp = dht.readTemperature();
    //Print temp and humidity values to serial monitor
    Serial.print("Humidity: ");
    Serial.print(hum);
    Serial.print(" %, Temp: ");
    Serial.print(temp);
    Serial.println(" Celsius");
    checkTemp(temp);
    updateDisplay();
    checkSMS();
    reset();
}


void checkSMS(){
  if (sms.available()){
      sms.remoteNumber(senderNumber, 20);
      Serial.println(senderNumber);
      while (ch = sms.read()) {
      message = message + ch;
      }
       message.toUpperCase();
       Serial.println(message);
       if (EEPROM.read(0) == 0){
        pairNum();
       } else {
        Serial.print("The owner number: ");
        Serial.println(ownerNumber);
        Serial.print("The sender number: ");
        Serial.println(senderNumber);
        bool correctNum = true;
        for (int i = 0; i<= EEPROM.read(0); i++){
          if (senderNumber[i] != ownerNumber[i]){
            correctNum = false;
          }
        }
        if (correctNum){
          command(message); 
        }
       }
       

       sms.flush();
       message = "";
    }
}

void pairNum(){
  Serial.print("The Phone in Number Paring is: ");
  Serial.println(senderNumber[5]);
  if (EEPROM.read(0) == 0){
   if (message == "PAIR"){
      numberSave();
    } else if (message == "READ") {
      numberRead();
    }
  } else {
    Serial.println("Device already paired!");
  }
}

void GSMConnect(){
  Serial.println("SMS Messages Receiver");
  boolean notConnected = true;
  // Start GSM connection
  while (notConnected) {
    if (gsmAccess.begin(PINNUMBER) == GSM_READY) {
      notConnected = false;
    } else {
      Serial.println("Not connected");
      delay(1000);
    }
  }
  Serial.println("GSM initialized");
  Serial.println("Waiting for messages");
}

void initValues(){
  maxTemp = EEPROM.read(MAXTEMP);
  minTemp = EEPROM.read(MINTEMP);
  interval = 1000 *  EEPROM.read(TIMER);
  int numLength;
  numLength = EEPROM.read(0);
  if (numLength > 0) { 
   for (int i = 0; i<=numLength; i++) {
       ownerNumber[i] = EEPROM.read((i+1));
     }
  }
}

void reset(){
  if (digitalRead(resetButton) == true){
    EEPROM.write(0,0);
    Serial.println("EEPROM 0 reset");
    lcd.clear();
    lcd.setCursor(0, 0); // top left
    lcd.print("Mobile Number");
    lcd.setCursor(0, 1); // bottom left
    lcd.print("was reset");
  }
}

void updateDisplay(){
  String stats = "   ";
  String hardwareStats = "";
  stats = stats + int(temp);
  stats = stats + "C Hum:";
  stats = stats + int(hum);
  stats = stats + "% ";
  lcd.clear();
  lcd.setCursor(0, 0); // top left
  lcd.print(stats);
  if (radOn == true){
    hardwareStats = hardwareStats + "Radiator ";
  }
  if (liOn == true){
    hardwareStats = hardwareStats + "Light";
  }
  lcd.setCursor(0, 1); // bottom left
  lcd.print(hardwareStats);
  //Serial.println("Updated");
  
}

void checkCommand(){
  //IF SMS WAS RECIEVED GO TO COMMAND

  //If manual control is enabled and 15 minutes have passed the radiator will turn off
  if ((manual == true) && (timeNow >= timeOfCommand + interval)) {
    disableManual();
  }
  
}


//Poor structure but the scope of this project is small and mainly as proof of 
// concept. If the project ever grows this should be refactored.
void command(String inst){  
  bool validCommand = false;
  Serial.println("Choices");
  if (inst == "RADON") {
    if (radOn == false){
      radiatorOn();
      manual = true; 
      timeOfCommand = timeNow;
      validCommand = true;
    }
  } else if (inst == "RADOFF") {
    if (radOn == true){
      radiatorOff();  
      manual = true;
      timeOfCommand = timeNow;
      validCommand = true;
    }
  } else if (inst == "LION") {
    if (liOn == false) {
      digitalWrite(lightLED,HIGH);
      liOn = true;
      validCommand = true;
    }
  } else if (inst == "LIOFF"){
    if (liOn == true) {
      digitalWrite(lightLED,LOW);
      liOn = false;
      validCommand = true;
    }
  } else {
    String temp = inst;
    int pinUsed;
    temp.remove(3); // Allows me to check the first three characters
    Serial.println(temp);
    if(temp == "MAX"){
      inst.remove(0,3); //Removes the first 3 letters to leave 
      pinUsed = MAXTEMP;
      Serial.print("Max");
    } else if (temp == "MIN") {
      inst.remove(0,3); //Removes the first 3 letters to leave 
      pinUsed = MINTEMP;
      Serial.print("Min");
    } else if (temp == "TIM"){
      inst.remove(0,3); //Removes the first 3 letters to leave 
      pinUsed = TIMER;
      Serial.print("Tim");
    } else {
      validCommand = false;
    }
    if (validCommand = true){
      if (digitChecker(inst) == true) {
        int value = inst.toInt();
        Serial.print("pin used ");
        Serial.print(pinUsed);
        Serial.print("value");
        Serial.println(value);
        EEPROM.write(pinUsed,value);
      } else {
        validCommand = false;
      }
    }
    initValues();
  }

  if (validCommand == false){
    lcd.clear();
    lcd.print("Invalid Command");
  }
}


bool digitChecker(String checkedString){
  int len = checkedString.length();
  char checkedChar = 'a';
  for(int i = 0; i< len;i++){
    checkedChar = checkedString.charAt(i);
    if (isDigit(checkedChar) == false) {
      return false;
    }
  }
  return true;
}

void valueSaver(int pin, int value) {
  EEPROM.write(pin,value);
}

//Reverts radiator to what it was before the manual command. 
//Will only ever run if status was manually changed, therefore has no effect
// if the command to turn rad on/off was redundant. 
void disableManual(){
  Serial.println("Disable Manual");
  if (radOn == false){
      radiatorOff();  
    }
  if (radOn == true){
      radiatorOn();
    }
  //Disable manual at the end.
  manual = false;
}

void numberSave(){
  String numString = String(senderNumber); //Converting the phone number from a charlist to a string allows to check the length of the number
  int numLength; //This will let use less EEPROM in the long run as well as supporting numbers from multiple countries

  // Saves the length of the phone number to EEPROM, allows for structure in using EERPOM efficiently 
  // as well as easy reconstruction of the phone number in future use
  numLength = numString.length();
  EEPROM.write(0,numLength);

  // The first digit of the phone number will be saved in EEPROM(1). A standard UK number should end on EEPROM(11)
  for (int i = 0; i<=numLength; i++) {
    EEPROM.write((i+1),senderNumber[i]);
    Serial.print(senderNumber[i]);
    for (int i = 0; i<=numLength; i++) {
      ownerNumber[i] = EEPROM.read((i+1));
    }
  }
  Serial.println(" Complete!");
}

void numberRead(){
  int numLength = EEPROM.read(0);
  for (int i = 0; i<=numLength; i++) {
    ownerNumber[i] = EEPROM.read((i+1));
    Serial.print(ownerNumber[i]);
  }
  Serial.println(" Complete!");
}


void checkTemp(float temp){ 
  if (temp < minTemp){
    //Serial.println("Radiator On!");
    if (radOn == false){
        radiatorOn();
      } 
      else if (radOn == true) {
        Serial.println("On");
        Serial.println("Off");
        Serial.print("The temp is:");
        Serial.print(temp);
        Serial.print(" And we want it to be between: ");
        Serial.print(minTemp);
        Serial.print(" & ");
        Serial.println(maxTemp);
      }
    }
  else if (temp >= maxTemp) {
    //Serial.println("Radiator Off!");
    if (radOn == true){
      radiatorOff();
      Serial.println("Off");
      Serial.print("The temp is:");
      Serial.print(temp);
      Serial.print(" And we want it to be between: ");
      Serial.print(minTemp);
      Serial.print(" & ");
      Serial.println(maxTemp);
      } 
      else if (radOn == false){
        Serial.println("Off");
        Serial.print("The temp is:");
        Serial.print(temp);
        Serial.print(" And we want it to be between: ");
        Serial.print(minTemp);
        Serial.print(" & ");
        Serial.println(maxTemp);
      }
  } 
}

//Turns the radiator on and sends appropriate debug message
void radiatorOn(){
  digitalWrite(rad,HIGH); //Turn radiator On
  radOn = true;
  //Serial.println("RadOn");
}

//Turns the radiator off and sends appropriate debug message
void radiatorOff(){
  //Serial.println("Off1");
  digitalWrite(rad, LOW); //Turn the radiator LED off
  radOn = false;
  //Serial.println("RadOff");  
} 
  
