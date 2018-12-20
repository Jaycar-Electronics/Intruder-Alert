/* ********************************************************************
 *  WiFi Intruder Alert
 *  
 *  Please goto www.jaycar.com.au for project documentation and details
 *  
 *  Code published: v1.0 5/March/2018
 *  
 * *********************************************************************
 */

// This is where all the libraries are included
#include <time.h>
#include <ESP8266WiFi.h>
#include "Gsender.h"
#include <WiFiUdp.h>


// This is where the literal declerations are made
#define to_email              "your target email account>"

#define timezone              11       // Time zone including day light savings
#define dst                    1       // There is a bug in the time synch, day light savings does not work
#define PIR_pin               D5
#define sw_pin                D4
#define green_led             D6
#define red_led               D7
#define _OFF                  1
#define _Armed                2
#define longPRESS           500         // mSec. Long press to turn ON or OFF
#define press_timeout      2000
#define arm_delay         10000         // mSec. Delay before starting to monitor PIR motion (also allows PIR sensor to settle upon power up)
#define trigger_interval      3         // Minutes. This is the delay between valid PIR trigger readings.
#define max_no_triggers       5         // number of records trigger events to record



// Here edit the ssid and password firelds to your network details
#pragma region Globals

const char* ssid = "WAVLINK_012B";                           // WIFI network name
const char* password = "lab12345";                       // WIFI network password

// const char* ssid = "OPTUSVD3E1B098";                           // WIFI network name
// const char* password = "VARNAGOWKS07976";                       // WIFI network password

// const char* ssid = "NetComm 4870";                           // WIFI network name
// const char* password = "iuozxqtihw";                       // WIFI network password

uint8_t connection_state = 0;                    // Connected to WIFI or not
uint16_t reconnect_interval = 10000;             // If not connected wait time to try again
#pragma endregion Globals


time_t time_now;
time_t start_time;
time_t trigger_time[max_no_triggers];
uint8_t state = _OFF;
uint8_t i = 0;

bool movement_detected = false;
unsigned long sw_press_t0;
unsigned long start_t0;
unsigned int  sw_press_t;
bool startup_completed;

 
/*
 * This function establish the WiFi network connection. Return True (good connection) or False (could not connect)
 */
uint8_t WiFiConnect(const char* nSSID = nullptr, const char* nPassword = nullptr)
{
    static uint16_t attempt = 0;
    Serial.print(F("Connecting to "));
    if(nSSID) {
        WiFi.begin(nSSID, nPassword);  
        Serial.println(nSSID);
    } else {
        WiFi.begin(ssid, password);
        Serial.println(ssid);
    }

    uint8_t i = 0;
    while(WiFi.status()!= WL_CONNECTED && i++ < 50)
    {
        delay(200);
        Serial.print(".");
    }
    ++attempt;
    Serial.println("");
    if(i == 51) {
        Serial.print(F("Connection: TIMEOUT on attempt: "));
        Serial.println(attempt);
        if(attempt % 2 == 0)
            Serial.println(F("Check if access point available or SSID and Password\r\n"));
        return false;
    }
    Serial.println(F("Connection: ESTABLISHED"));
    Serial.print(F("IP address: "));
    Serial.println(WiFi.localIP());
    return true;
}


void Awaits()
{
    uint32_t ts = millis();
    while(!connection_state)
    {
        delay(50);
        if(millis() > (ts + reconnect_interval) && !connection_state){
            connection_state = WiFiConnect();
            ts = millis();
        }
    }
}

bool send_gmail_notification(int state_, int i_) {

      int n;    
      Gsender *gsender = Gsender::Instance();    // Getting pointer to class instance
      time_t  report_time = time(nullptr);
      
      String subject = "Notification from WiFi Intruder Alarm";
    
      String message = "<br><br>-------------------------------------<br>"; 
      message += " WiFi Intruder Alert  - Notification";
      message += "<br>--------------------------------------<br>";
      
      message += "Monitoring Start: ";
      message += String(localtime(&start_time)->tm_mday)+"/"+String(localtime(&start_time)->tm_mon+1)+"/"+String(localtime(&start_time)->tm_year+1900);
      message += "       "+String(localtime(&start_time)->tm_hour)+":"+String(localtime(&start_time)->tm_min)+":"+String(localtime(&start_time)->tm_sec);

      message += "<br>";
      message += "Monitoring End: ";
      message += String(localtime(&report_time)->tm_mday)+"/"+String(localtime(&report_time)->tm_mon+1)+"/"+String(localtime(&report_time)->tm_year+1900);
      message += "       "+String(localtime(&report_time)->tm_hour)+":"+String(localtime(&report_time)->tm_min)+":"+String(localtime(&report_time)->tm_sec);

      message += "<br>--------------------------------------<br>";

      switch (state_) {

        case _Armed:
                    if (movement_detected == false) {
                          message += "No intrusions to report";
                          message += "<br>";
                      } else {
                          for (n=0; n <= i_; n++) {
                              message += "Intrusion detected: ";
                              message += String(localtime(&trigger_time[n])->tm_hour)+":"+String(localtime(&trigger_time[n])->tm_min)+":"+String(localtime(&trigger_time[n])->tm_sec);
                              message += "<br>";
                          }
                      }
                    break;

        case _OFF:
                    message += "<br> <br>"; 

                    if (movement_detected == false ) {
                          message += "No intrusions to report";
                          message += "<br>";
                      } else {
                        for (n=0; n <= i_; n++) {
                            message += "Intrusion detected: ";
                            message += String(localtime(&trigger_time[n])->tm_hour)+":"+String(localtime(&trigger_time[n])->tm_min)+":"+String(localtime(&trigger_time[n])->tm_sec);
                            message += "<br>";
                        }
                      }

                    message += "<br><br>";
                    message += "--------------------------------- <br>"; 
                    message += "WiFi Intruder Alert - OFF State   <br>";
                    message += "--------------------------------- <br>";
                    
                    break;
      }

                      

      if(gsender->Subject(subject)->Send(to_email, message))
          return true;
      else
          return false;
          
}

     
/*
 * this is the first function that is executed (once) upon power-up or reset. All the required 
 * program initialisation is done here.
 */
void setup() {
          
    Serial.begin(115200);

    pinMode(PIR_pin, INPUT);
    pinMode(sw_pin, INPUT);
    pinMode(red_led, OUTPUT);
    pinMode(green_led, OUTPUT);
    
    digitalWrite(red_led,LOW);
    digitalWrite(green_led,HIGH);

    connection_state = WiFiConnect();
    if(!connection_state)  // if not connected to WIFI
        Awaits();          // constantly trying to connect

    digitalWrite(red_led,LOW);
    digitalWrite(green_led,LOW);
   
    configTime(timezone * 3600, dst * 3600, "pool.ntp.org", "time.nist.gov");
    while (!time(nullptr)) {
      Serial.println(".");
      delay(1000);
    }

    Serial.println(F("WiFi Intruder Alaamr Started - in OFF state"));
    digitalWrite(red_led,HIGH);
    digitalWrite(green_led,HIGH);

    movement_detected = false;
    startup_completed == false;
    start_time = time(nullptr);
    start_t0 = millis();

}

/*
 * This is the main program loop. It continues to loop forvever (or until you unplug the power). 
 */
void loop() {

      time_now = time(nullptr);             // Update the current time 
      sw_press_t0 = millis();               // used for timing sw press duration
      sw_press_t = 0;                       // used to measure how long the sw is pressed

      if (digitalRead(sw_pin) == LOW) {
          do {
                     
                sw_press_t = millis() - sw_press_t0;
                if (sw_press_t >= longPRESS) {
                  switch (state) {
                    case _Armed:  state = _OFF;
                          send_gmail_notification(_OFF,i);
                          digitalWrite(red_led,HIGH);
                          digitalWrite(green_led,HIGH);
                          
                          Serial.println(F("Changed from ARM to OFF state"));
  
                          break;
                    
                    case _OFF:    state = _Armed;
                          digitalWrite(red_led,HIGH);
                          digitalWrite(green_led,LOW);
                          i = 0;
                          movement_detected = false;
                          startup_completed == false;
                          start_time = time(nullptr);
                          start_t0 = millis();
                          
                          Serial.println(F("Changed from OFF to ARM state - please wait for Arm delay"));
                          
                          break;
                          
                    default:      break;
                }
                  
              }
                 
          } while (digitalRead(sw_pin) == LOW && sw_press_t < longPRESS);     
      }
  

      if (startup_completed == false && state == _Armed) {

        if (millis() - start_t0 > arm_delay) {

            Serial.println(" ");
            Serial.println(F("WiFi Intruder Alarm Now Armed - "));
                          
            startup_completed = true;
            for (int x=0; x<15; x++) {
                digitalWrite(green_led,!digitalRead(green_led));
                delay(150);
            }
            
            digitalWrite(green_led,LOW);
        }    
      }
     
      if (digitalRead(PIR_pin) == HIGH && state == _Armed && startup_completed == true) {
          
              digitalWrite(red_led, LOW);
              delay(30);
              digitalWrite(red_led, HIGH);

              if ( i == 0 ){
                  Serial.print(F("Received trigger   - "));
                  Serial.println(i);
                  trigger_time[i] = time_now;    
                  movement_detected = true;
                  send_gmail_notification(_Armed, i);
                  i += 1;                
              } else if (i > 0 && i < max_no_triggers) {
                  if (localtime(&time_now)->tm_min -  localtime(&trigger_time[i-1])->tm_min > trigger_interval) {
                    trigger_time[i] = time_now;
                    movement_detected = true;
                    i += 1;  
                   }
              } else if (i == max_no_triggers) {
                  send_gmail_notification(_Armed, i);
                  movement_detected = true;
                  i = 0;
              }
      }

}      


