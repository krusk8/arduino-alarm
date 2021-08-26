    /**
    * Created By Tafuri Mirko
    * Centrale antifurto costruita con Arduino uno. Materiali:
    * Arduino UNO, lettore RFID RC522, sensore Pir, 3 led colorati, LCD, un pulsante, un buzzer.
    * Importante, collegare il lettore RFID ai pin di Arduino come segue:
    * https://www.arduino.cc/en/Reference/SPI
    * 
    * MOSI (Master Out Slave In): Pin 11 / ICSP-4
    * MISO (Master In Slave Out): Pin 12 / ICSP-1
    * SCK (Serial Clock): Pin 13 / ISCP-3
    * SS (Slave Select): Pin 10
    * RST: Pin 9
    * 
    * --- LED LEGEND ---
    * RED OFF      -> ALARM OFF
    * RED FLASHING -> ALARM ENGAGED
    * RED ON       -> ALARM TRIGGERED
    * YELLOW       -> CHECK MOVMENT FOR TEST
    */
    //Serial Peripheral Interface(SPI) to RFID communications
    #include <SPI.h>
    #include <RFID.h>
    //EEprom is memory to save information
    #include <EEPROM.h>
    //LiquidCrystal manage the LCD
    #include <LiquidCrystal.h>

    //Define pins
    #define HALL_sensor__PIN 2
    #define ledRed_PIN 4
    #define ledYellow_PIN 5
    #define BTN_RST_PIN 6
    #define BTN_TEST_PIN 3
    #define buzzer_PIN 7
    #define PIR_PIN 8
    #define RST_PIN 9
    #define SS_PIN 10
    //Define Default
    #define DEF_DELAY_MSG 2000
    #define DEF_DELAY_ALARM 5000 //TOD portare a 10 secondi
    #define DEF_BUZZ_FREQ 4186
    #define DEF_BUZZ_LENGTH 100
    #define DEF_FLASH_INTERVAL 500

    //TODO Define master key number before start
    unsigned char masterKeys[] = {70,43,12,232,43};
    //Define Variables
    RFID rfid(SS_PIN, RST_PIN); 
    LiquidCrystal lcd(A0, A1, A2, A3, A4, A5);

    boolean isAlarmON = false;
    boolean isAlarmTriggered = false;
    boolean isMasterManageMode = false;
    int ledState = LOW;
    int slaveCounter;
    unsigned long previousMillis = 0;       
    

    void setup() { 
      SPI.begin(); 
      rfid.init();    
      initPins();
      resetLed();
      resetLcd();
    }
   
    void initPins() {
      lcd.begin(16, 2);
      pinMode(HALL_sensor__PIN, INPUT);
      pinMode(BTN_RST_PIN, INPUT);
      pinMode(BTN_TEST_PIN, INPUT);
      pinMode(buzzer_PIN, OUTPUT);
      pinMode(ledRed_PIN, OUTPUT);
      pinMode(ledYellow_PIN, OUTPUT);
    }
    void resetLed() {
      digitalWrite(ledRed_PIN, LOW);
      digitalWrite(ledYellow_PIN, LOW);
    }
    void printMessageOnLCD(String message){
      lcd.clear();
      lcd.print(message);
      delay(DEF_DELAY_MSG);
    }
    void resetLcd() {
      printMessageOnLCD("AllarmDuino 2.0");
    }
    void loop() {
      checkMovment();
      //TODO reinserire una volta arrivato l'Rfid
      //checkRfid();   
      
      if (digitalRead(HALL_sensor__PIN) == HIGH) {
        digitalWrite(ledYellow_PIN, HIGH);
      }else{
        digitalWrite(ledYellow_PIN, LOW);
      } 

      
      rfid.halt();
    }
    void updateSlaveCounter(){
      slaveCounter = EEPROM.read(0);
    }
    void getSlavesByCountId(unsigned char selectedSlave[], int slaveCounter){
      for(int j = 0; j<5;j++){
           selectedSlave[j] = EEPROM.read(j+1+((slaveCounter-1)*5));
        }
    }
    void saveSlaveAtEnd(){
      for(int j = 0; j<5;j++){
       EEPROM.write((j+1+(slaveCounter*5)), rfid.serNum[j]);
      }
      slaveCounter = slaveCounter++;
      EEPROM.write(0, slaveCounter);
      printMessageOnLCD("Total Slaves: "+ slaveCounter);
    }
    void checkMovment(){
      //TODO REMOVE
      digitalWrite(ledYellow_PIN, digitalRead(PIR_PIN));
      
      //Maybe can be more than just one PIR
      if(isAlarmON) {
        digitalWrite(ledRed_PIN, HIGH);
        printMessageOnLCD("ALLARME!!");
        delay(DEF_DELAY_ALARM);
        isAlarmTriggered = true;
        buzz(buzzer_PIN, DEF_BUZZ_FREQ, DEF_BUZZ_LENGTH);
     }
    }

    void resetPressed(){
      printMessageOnLCD("RESET KEY!!");
      //Clear EEPROM
      for (int i = 0 ; i < EEPROM.length() ; i++) {
        EEPROM.write(i, 0);
      }
      stopManageMode();
      resetLcd();
    }

    boolean isCardEquals(unsigned char card1[], unsigned char card2[]) {
      boolean result = true;
      for(int i=0; i<5; i++){
        boolean isEqualCurrent = card1[i] == card2[i];
        result = isEqualCurrent && result;
      }
      return result;
    }
    boolean isCardPassedMaster(){
      return isCardEquals(rfid.serNum, masterKeys);
    }
    boolean isCardPassedSlave(){
      boolean result;
      if(slaveCounter > 0){
        result = true;
        for(int i = 1; i<=slaveCounter;i++){
          unsigned char currentSlave[5];
          getSlavesByCountId(currentSlave, i);
          boolean isEqual = isCardEquals(rfid.serNum, currentSlave);
          result = isEqual && result;
        }
      }else{
        result = false;
      }
      return result;
    }
    void startManageMode(){
      isMasterManageMode = true;
      printMessageOnLCD("MANAGE MODE ON");
      printMessageOnLCD("Total Slaves: " + slaveCounter);
      printMessageOnLCD("1) Press Button");
      printMessageOnLCD("1) To reset ALL");
      printMessageOnLCD("2) Pass Slave");
      printMessageOnLCD("2) To Save it");
      printMessageOnLCD("3) Pass Master");
      printMessageOnLCD("3) To STOP MANAGE");
      printMessageOnLCD("MANAGE MODE ON");

      //TODO Stampare anche le key degli slave??
      //TODOGestirre messaggi informativi
    }

    void saveSlave(){
      printMessageOnLCD("New Slave");
      saveSlaveAtEnd();
      stopManageMode();   
    } 
    void stopManageMode(){
      isMasterManageMode = false;    
      printMessageOnLCD("MANAGE MODE OFF");
      resetLcd();
    } 
    void stopAlarm(){
      isAlarmON = false;
      isAlarmTriggered = false;
      printMessageOnLCD("VALID KEY");
      printMessageOnLCD("ALARM OFF..");
      //Shoutdown all Led
      resetLed();
      resetLcd();
    }  
    void startAlarm(){
      printMessageOnLCD("VALID KEY");
      printMessageOnLCD("ALARM ON..");
      //WAIT dealy before engaged alarm
      delay(DEF_DELAY_ALARM);
      isAlarmON = true;
      isAlarmTriggered = false;
      printMessageOnLCD("ANTIFURTO");
      printMessageOnLCD("ATTIVO..");
      flashingLed(ledRed_PIN, DEF_FLASH_INTERVAL);
      resetLcd();
    }

    void checkRfid(){
      updateSlaveCounter();
      if (rfid.isCard()) {
        if (rfid.readCardSerial()) {
          //If isMasterManageMode ON
          if(isMasterManageMode){
            //Reset button pressed
            if (digitalRead(BTN_RST_PIN) == HIGH) {
              resetPressed();
            }
            else if (isCardPassedMaster()) {
              stopManageMode();
            }             
            else if (isCardPassedSlave()) {
              //remove slave not implemented yet
              stopManageMode();
            }//Add new Slave
            else{
              if(slaveCounter <3){
                saveSlave();  
              }else{
                printMessageOnLCD("Max Slave is 3");
                printMessageOnLCD("Press Button");
                printMessageOnLCD("To reset ALL");
                stopManageMode();
              }
            }
          }
          //If isMasterManageMode OFF and isCardPassedMaster startManageMode 
          else if (isCardPassedMaster()) {
            startManageMode();
          }//If isMasterManageMode OFF and isCardPassedSlave start or stop Alarm
          else if (isCardPassedSlave()) {
            if (isAlarmON) {  
              stopAlarm();
            } else {
              startAlarm();
            } 
          }
        }
      }
    }
      
    void buzz(int targetPin, long frequency, long length) {
     long delayValue = 1000000/frequency/2;
     long numCycles = frequency * length/ 1000;
     for (long i=0; i < numCycles; i++) {
      digitalWrite(targetPin,HIGH);
      delayMicroseconds(delayValue);
      digitalWrite(targetPin,LOW);
      delayMicroseconds(delayValue);
     }
    }
    void flashingLed(int led_PIN, long interval) {
      unsigned long currentMillis = millis();
      if(currentMillis - previousMillis > interval) {
        previousMillis = currentMillis; 
        //Toggle led_PIN
        digitalWrite(led_PIN, !digitalRead(led_PIN));
      }
    }




