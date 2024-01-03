/*
*********************
Split Flap ESP Master
*********************
*/
#define BAUDRATE 115200     //Serial debugging BAUD rate
#define ANSWERSIZE 1        //Size of unit's request answer
#define UNITSAMOUNT 10      //Amount of connected units !IMPORTANT!
#define FLAPAMOUNT 45       //Amount of Flaps in each unit
#define MINSPEED 1          //Min Speed
#define MAXSPEED 12         //Max Speed
#define OTA_ENABLE          //Comment out to disable OTA functionality
//#define SERIAL_ENABLE       //Uncomment for serial debug messages, no serial messages if this whole line is a comment!
//#define UNIT_CALLS_DISABLE  //Disable the call to the units so can just debug the ESP

#include <Arduino.h>
#include <Arduino_JSON.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ezTime.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <LinkedList.h>

#include "Classes.h"
#include "LittleFS.h"

//OTA Libary
#ifdef OTA_ENABLE
#include <ArduinoOTA.h>
#endif

//The current version of code to display on the UI
const char* espVersion = "1.4.0";

//REPLACE WITH YOUR NETWORK CREDENTIALS
const char* ssid = "SSID";
const char* password = "12345678901234567890";

//Change if you want to have an OTA Password
const char* otaPassword = "";

//Change this to your timezone, use the TZ database name
//https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
const char* timezoneString = "Europe/London";

//If you want to have a different date or clock format change these two
//Complete table with every char: https://github.com/ropg/ezTime#getting-date-and-time
const char* dateFormat = "d.m.Y"; //Examples: d.m.Y -> 11.09.2021, D M y -> SAT SEP 21
const char* clockFormat = "H:i"; //Examples: H:i -> 21:19, h:ia -> 09:19PM

const char letters[] = {' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '$', '&', '#', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', '.', '-', '?', '!'};
int displayState[UNITSAMOUNT];
unsigned long previousMillis = 0;

//Variables for storing text for the display to use
String lastWrittenText = "";
LinkedList<ScheduledMessage> scheduledMessages;

//Create AsyncWebServer object on port 80
AsyncWebServer server(80);

//Search for parameter in HTTP POST request
const char* PARAM_ALIGNMENT = "alignment";
const char* PARAM_FLAP_SPEED = "flapSpeed";
const char* PARAM_DEVICEMODE = "deviceMode";
const char* PARAM_INPUT_TEXT = "inputText";
const char* PARAM_SCHEDULE_ENABLED = "scheduleEnabled";
const char* PARAM_SCHEDULE_DATE_TIME = "scheduledDateTimeUnix";
const char* PARAM_COUNTDOWN_DATE = "countdownDateTimeUnix";
const char* PARAM_ID = "id";

//Device Modes
const char* DEVICE_MODE_TEXT = "text";
const char* DEVICE_MODE_CLOCK = "clock";
const char* DEVICE_MODE_DATE = "date";
const char* DEVICE_MODE_COUNTDOWN = "countdown";

//Alignment options
const char* ALIGNMENT_MODE_LEFT = "left";
const char* ALIGNMENT_MODE_CENTER = "center";
const char* ALIGNMENT_MODE_RIGHT = "right";

//Variables to save values from HTML form
String alignment = "";
String flapSpeed = "";
String inputText = "";
String previousDeviceMode = "";
String currentDeviceMode = "";
String countdownToDateUnix = "";
long newMessageScheduledDateTimeUnix = 0;
bool newMessageScheduleEnabled = false;

//File paths to save input values permanently
const char* alignmentPath = "/alignment.txt";
const char* flapSpeedPath = "/flapspeed.txt";
const char* deviceModePath = "/devicemode.txt";
const char* countdownPath = "/countdown.txt";

//Create ezTime timezone object
Timezone timezone; 

//Denotes the system is pending a restart
bool isPendingReboot = 0;

//Denotes the system needs calibrating
bool isPendingUnitsReset = 0;

//Used to denote that the system has gone into OTA mode
#ifdef OTA_ENABLE
bool isInOtaMode = 0;
#endif

//Used for display on the UI
String lastReceivedMessageDateTime;

void setup() {
  //Serial port for debugging purposes
#ifdef SERIAL_ENABLE
  Serial.begin(BAUDRATE);
#endif

  SerialPrintln("#######################################################");
  SerialPrintln("Master module starting...");

#ifdef OTA_ENABLE
  SerialPrintln("OTA Enabled...");
#endif

  //deactivate I2C if debugging the ESP, otherwise serial does not work
#ifndef SERIAL_ENABLE
  Wire.begin(1, 3); //For ESP01 only
#endif
  //Wire.begin(D1, D2); //For NodeMCU testing only SDA=D1 and SCL=D2

  initWiFi();
  initialiseFileSystem();
  loadValuesFromFileSystem();

  //ezTime initialization
  waitForSync();
  timezone.setLocation(timezoneString);

  //Web Server Root URL
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    SerialPrintln("Request Home Page Received");

    request->send(LittleFS, "/index.html", "text/html");
  });

  server.serveStatic("/", LittleFS, "/");

  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest * request) {
    SerialPrintln("Request for Settings Received");
    
    String json = getCurrentSettingValues();
    request->send(200, "application/json", json);
    json = String();
  });
  
  server.on("/health", HTTP_GET, [](AsyncWebServerRequest * request) {
    SerialPrintln("Request for Health Check Received");
    
    String json = getCurrentSettingValues();
    request->send(200, "text/plain", "Healthy");
  });

#ifdef OTA_ENABLE
  server.on("/ota", HTTP_GET, [](AsyncWebServerRequest * request) {
    SerialPrintln("Request to start OTA mode received");
    
    //Create HTML page to explain OTA
    IPAddress ip = WiFi.localIP();
    
    String html = "<div style='text-align:center'>";
    html += "<font face='arial'><h1>Split Flap - OTA Update Mode</h1>";
    html += "<p>OTA mode has been started. You can now update your module via WiFI. Open your Arduino IDE and select the new port in \"Tools\" menu and upload the your sketch as normal!<p>";
    html += "<p>Open your Arduino IDE and select the new port in \"Tools\" menu and upload the your sketch as normal!</p>";
    html += "<p>After you have carried out your update, the system will automatically be rebooted. You can go to the main home page after this time by clicking the button below or going to '/'.</p>";
    html += "<p>You can take the system out of this mode by clicking the button to reboot below or going to '/reboot'.</p>";
    html += "<p><a href=\"http://" + ip.toString() + "\")\">Home</a></p>";
    html += "<p><a href=\"http://" + ip.toString() + "/reboot\")\">Reboot</a></p>";
    html += "</font>";
    html += "</div>";

    request->send(200, "text/html", html);
 
    if (isInOtaMode == 0) {
      SerialPrintln("Setting OTA Hostname");
      ArduinoOTA.setHostname("split-flap-ota");

      //If there is a password set, disabled by default for ease
      if (otaPassword != "") {
        ArduinoOTA.setPassword(otaPassword);
      }
      
      SerialPrintln("Starting OTA Mode");
      ArduinoOTA.begin();
      delay(100);
    
      ArduinoOTA.onStart([]() {
        LittleFS.end();
        if (ArduinoOTA.getCommand() == U_FLASH) {
          SerialPrintln("Start updating sketch");
        } 
        else {
          SerialPrintln("Start updating filesystem");
        }  
      });
      
      ArduinoOTA.onEnd([]() {
        SerialPrintln("Finished OTA Update - Rebooting");
        isPendingReboot = true;
      });
      
      ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
        SerialPrintf("OTA Progress: %u%%\r", (progress / (total / 100)));
      });
      
      ArduinoOTA.onError([](ota_error_t error) {
        SerialPrintf("Error[%u]: ", error);

        if (error == OTA_AUTH_ERROR) {
          SerialPrintln("Finished OTA Update - Rebooting");
        }
        else if (error == OTA_BEGIN_ERROR) {
          SerialPrintln("OTA Begin Failed");
        }
        else if (error == OTA_CONNECT_ERROR) {
          SerialPrintln("OTA Connect Failed");
        }
        else if (error == OTA_RECEIVE_ERROR) {
          SerialPrintln("OTA Receive Failed");
        }
        else if (error == OTA_END_ERROR) {
          SerialPrintln("OTA End Failed");
        }
      });
      
      //Put in OTA Mode
      isInOtaMode = 1;
    }
    else {
      SerialPrintln("Already in OTA Mode");
    }
  });
#endif
  
  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest * request) {
    SerialPrintln("Request to Reboot Received");
    
    //Create HTML page to explain the system is rebooting
    IPAddress ip = WiFi.localIP();
    
    String html = "<div style='text-align:center'>";
    html += "<font face='arial'><h1>Split Flap - Rebooting</h1>";
    html += "<p>Reboot is pending now...<p>";
    html += "<p>This can take anywhere between 10-20 seconds<p>";
    html += "<p>You can go to the main home page after this time by clicking the button below or going to '/'.</p>";
    html += "<p><a href=\"http://" + ip.toString() + "\">Home</a></p>";
    html += "</font>";
    html += "</div>";
    
    request->send(200, "text/html", html);
    isPendingReboot = 1;
  });
  
  server.on("/reset-units", HTTP_GET, [](AsyncWebServerRequest * request) {
    SerialPrintln("Request to Reset Units Received");
    
    //This will be picked up in the loop
    isPendingUnitsReset = 1;
    
    request->redirect("/?is-resetting-units=true");
  });

  server.on("/scheduled-message/remove", HTTP_DELETE, [](AsyncWebServerRequest * request) {
    SerialPrintln("Request to Remove Scheduled Message Received");
    
    if (request->hasParam(PARAM_ID)) {
      bool removedScheduledMessage = false;
      String idValue = request->getParam(PARAM_ID)->value();

      //Find the existing scheduled message and delete it
      for(int scheduledMessageIndex = 0; scheduledMessageIndex < scheduledMessages.size(); scheduledMessageIndex++) {
        ScheduledMessage scheduledMessage = scheduledMessages[scheduledMessageIndex];
    
        if (idValue.toInt() == scheduledMessage.ScheduledDateTimeUnix) {
          SerialPrintln("Deleting Scheduled Message due to be shown: " + scheduledMessage.Message);
          scheduledMessages.remove(scheduledMessageIndex);
          
          removedScheduledMessage = true;
          request->send(201, "text/plain", "Created");
          break;
        }
      }

      if (!removedScheduledMessage) {
        SerialPrintln("Unable to find Scheduled Message to value. Id: " + idValue);
        request->send(400, "text/plain", "Unable to find message with ID specified. Id: " + idValue);
      }
    } 
    else {
        SerialPrintln("Delete Scheduled Message Received with no ID");
        request->send(400, "text/plain", "No ID specified");
    }
  });

  server.on("/", HTTP_POST, [](AsyncWebServerRequest * request) {
    bool submissionError = false;

    SerialPrintln("Request Post of Form Received");    
    lastReceivedMessageDateTime = timezone.dateTime("d M y H:i:s");
    
    int params = request->params();
    for (int i = 0; i < params; i++) {
      AsyncWebParameter* p = request->getParam(i);
      if (p->isPost()) {

        //HTTP POST alignment value
        if (p->name() == PARAM_ALIGNMENT) {
          String receivedValue = p->value();
          if (receivedValue == ALIGNMENT_MODE_LEFT || receivedValue == ALIGNMENT_MODE_CENTER || receivedValue == ALIGNMENT_MODE_RIGHT) {
            alignment = receivedValue;
  
            writeFile(LittleFS, alignmentPath, alignment.c_str());
            
            SerialPrintln("Alignment set to: " + alignment);
          }
          else {
            SerialPrintln("Alignment provided was not valid. Value: " + receivedValue); 
            submissionError = true;
          }
        }

        //HTTP POST device mode value
        if (p->name() == PARAM_DEVICEMODE) {
          String receivedValue = p->value();
          if (receivedValue == DEVICE_MODE_TEXT || receivedValue == DEVICE_MODE_CLOCK || receivedValue == DEVICE_MODE_DATE || receivedValue == DEVICE_MODE_COUNTDOWN) {
            //This is a defacto mode from the user being set so they should be the same
            currentDeviceMode = previousDeviceMode = receivedValue;
        
            writeFile(LittleFS, deviceModePath, currentDeviceMode.c_str());
            
            SerialPrintln("Device Mode set to: " + receivedValue);
          }
          else {
            SerialPrintln("Device Mode provided was not valid. Invalid Value: " + receivedValue); 
            submissionError = true;
          }
        }

        //HTTP POST Flap Speed Slider value
        if (p->name() == PARAM_FLAP_SPEED) {
          flapSpeed = p->value().c_str();

          writeFile(LittleFS, flapSpeedPath, flapSpeed.c_str());

          SerialPrintln("Flap Speed set to: " + flapSpeed);
        }

        //HTTP POST inputText value
        if (p->name() == PARAM_INPUT_TEXT) {
          inputText = p->value().c_str();
          
          if (inputText != "") {
            SerialPrintln("Input Text set to: " + inputText);
          }
          else {
            SerialPrintln("Input Text set to: <Blank>");
          }
        }

        //HTTP POST Schedule Enabled
        if (p->name() == PARAM_SCHEDULE_ENABLED) {
          String newMessageScheduleEnabledString = p->value().c_str();
          newMessageScheduleEnabled = newMessageScheduleEnabledString == "on" ?
            true : 
            false;
            
          SerialPrintln("Schedule Enable set to: " + newMessageScheduleEnabled);
        }

        //HTTP POST Schedule Seconds
        if (p->name() == PARAM_SCHEDULE_DATE_TIME) {
          String scheduledDateTimeUnix = p->value().c_str();
          newMessageScheduledDateTimeUnix = atol(scheduledDateTimeUnix.c_str());

          SerialPrintln("Schedule Date Time set to: " + scheduledDateTimeUnix);
        }

        //HTTP POST Countdown Seconds
        if (p->name() == PARAM_COUNTDOWN_DATE) {
          String countdownUnix = p->value().c_str();
          countdownToDateUnix = atol(countdownUnix.c_str());

          writeFile(LittleFS, countdownPath, countdownUnix.c_str());

          SerialPrintln("Countdown Date set to: " + countdownToDateUnix);
        }
      }
    }    

    //If there was an error, report back to check what has been input
    if (submissionError) {
      SerialPrintln("Finished Processing Request with Error");
      request->redirect("/?invalid-submission=" + true);
    }
    else {
      SerialPrintln("Finished Processing Request Successfully");

      //Delay to give time to process the scheduled message so can be returned on "redirect"
      delay(1024);

      //Redirect so that we don't have the "re-submit form" problem in browser for refresh
      request->redirect("/");
    }
  });
  
  SerialPrintln("Starting server...");
  server.begin();
  
  SerialPrintln("Master module ready!");
  SerialPrintln("#######################################################");
}

void loop() {
  //Reboot in here as if we restart within a request handler, no response is returned
  if (isPendingReboot) {
    SerialPrintln("Rebooting Now...");
    delay(200);
    ESP.restart();
  }

  if (isPendingUnitsReset) {
    SerialPrintln("Reseting Units now...");

    //Set the device mode to "Text" so can do a reset
    previousDeviceMode = currentDeviceMode;
    currentDeviceMode = DEVICE_MODE_TEXT;

    //Blank out the message
    String blankOutText1 = createRepeatingString('-');
    showText(blankOutText1);
    delay(2000);

    //Do just enough to do a full iteration which triggers the re-calibration
    String blankOutText2 = createRepeatingString('.');
    showText(blankOutText2);

    //Put back to the mode we was just in
    currentDeviceMode = previousDeviceMode;

    //Try re-display the last message now we've reset if we was in text mode
    if (currentDeviceMode == DEVICE_MODE_TEXT) {
      showText(lastWrittenText);
    }

    //We did a reset!
    isPendingUnitsReset = 0;

    SerialPrintln("Done Units Reset!");
  }
  
#ifdef OTA_ENABLE
  //If System is in OTA, try handle!
  if(isInOtaMode == 1) {
    ArduinoOTA.handle();
    delay(1);
  }
#endif

  //ezTime library function
  events(); 

  //Reset loop delay
  unsigned long currentMillis = millis();
  
  //Delay to not spam web requests
  if (currentMillis - previousMillis >= 1024) {
    previousMillis = currentMillis;

    checkScheduledMessages();
    checkCountdown();

    //Mode Selection
    if (currentDeviceMode == DEVICE_MODE_TEXT || currentDeviceMode == DEVICE_MODE_COUNTDOWN) { 
      showText(inputText);
    } 
    else if (currentDeviceMode == DEVICE_MODE_DATE) {
      showDate();
    } 
    else if (currentDeviceMode == DEVICE_MODE_CLOCK) {
      showClock();
    } 
  }
}

void checkCountdown() {
  //This will check if a day has passed since the last time the countdown was updated
  if (currentDeviceMode == DEVICE_MODE_COUNTDOWN) {
    long countdownInSeconds = atol(countdownToDateUnix.c_str());

    //Work out how long left
    long currentTimeSeconds = timezone.now();
    long differenceSeconds = countdownInSeconds - currentTimeSeconds;
    long hours = differenceSeconds / 60 / 60;

    //If there is any remainder, we want to actually still display the full day remaining 
    long days = ceil(hours / 24.0);

    //Make sure we don't go negative
    days = days > 0 ? days : 0;
    
    String daysText = String(days) + (days == 1 ? " day" : " days");
    if (daysText.length() > UNITSAMOUNT) {
      SerialPrintln("Days Text was too long, cutting down to just the number...");
      daysText = "" + days;
    }

    if (inputText != daysText) {
      SerialPrintln("Setting Countdown Text to: " + daysText);
      inputText = daysText;
    }
  }
}

void checkScheduledMessages() {   
  //Add the message to the scheduled list and mark as no longer scheduled
  //set the last written message to the scheduled one so don't repeat
  if (newMessageScheduleEnabled) {
    SerialPrintln("Processing New Scheduled Message");

    //Find the existing scheduled message and update it if one exists
    for(int scheduledMessageIndex = 0; scheduledMessageIndex < scheduledMessages.size(); scheduledMessageIndex++) {
      ScheduledMessage scheduledMessage = scheduledMessages[scheduledMessageIndex];

      if (newMessageScheduledDateTimeUnix == scheduledMessage.ScheduledDateTimeUnix) {
        SerialPrintln("Removing Existing Scheduled Message due to be shown, it will be replaced");
        scheduledMessages.remove(scheduledMessageIndex);
        break;
      }
    }

    //If we haven't just updated one, then we need to add it
    if (newMessageScheduledDateTimeUnix > timezone.now()) {
      SerialPrintln("Adding new Scheduled Message");
      scheduledMessages.add({inputText, newMessageScheduledDateTimeUnix});
    }

    //No longer got a scheduled message to process
    newMessageScheduleEnabled = false;

    //Only if we are in text mode, do we change the current input text to the last written message 
    //so it doesn't write the scheduled message, otherwise, it will affect other modes
    if (currentDeviceMode == DEVICE_MODE_TEXT) {
      inputText = lastWrittenText;
    }
  }

  //Iterate over the current bunch of scheduled messages. If we find one where the current time exceeds when we should show
  //the message, then we need to show that message immediately
  unsigned long currentTimeUnix = timezone.now();
  for(int scheduledMessageIndex = 0; scheduledMessageIndex < scheduledMessages.size(); scheduledMessageIndex++) {
    ScheduledMessage scheduledMessage = scheduledMessages[scheduledMessageIndex];

    if (currentTimeUnix > scheduledMessage.ScheduledDateTimeUnix) {
      SerialPrintln("Scheduled Message due to be shown: " + scheduledMessage.Message);

      //Set the next message to 
      inputText = scheduledMessage.Message;

      //Set the device mode to "Text" so can show a scheduled message
      previousDeviceMode = currentDeviceMode;
      currentDeviceMode = DEVICE_MODE_TEXT;

      scheduledMessages.remove(scheduledMessageIndex);
      break;
    }
  }
}
