/* ####################################################################################################################### */
/* # ____  ____  _     ___ _____   _____ _        _    ____    _____ ____  ____    __  __    _    ____ _____ _____ ____  # */
/* #/ ___||  _ \| |   |_ _|_   _| |  ___| |      / \  |  _ \  | ____/ ___||  _ \  |  \/  |  / \  / ___|_   _| ____|  _ \ # */
/* #\___ \| |_) | |    | |  | |   | |_  | |     / _ \ | |_) | |  _| \___ \| |_) | | |\/| | / _ \ \___ \ | | |  _| | |_) |# */
/* # ___) |  __/| |___ | |  | |   |  _| | |___ / ___ \|  __/  | |___ ___) |  __/  | |  | |/ ___ \ ___) || | | |___|  _ < # */
/* #|____/|_|   |_____|___| |_|   |_|   |_____/_/   \_|_|     |_____|____/|_|     |_|  |_/_/   \_|____/ |_| |_____|_| \_\# */
/* ####################################################################################################################### */
/*
  This project project is done for fun as part of: https://github.com/JonnyBooker/split-flap
  None of this would be possible without the brilliant work of David Königsmann: https://github.com/Dave19171/split-flap

  Licensed under GNU: https://github.com/JonnyBooker/split-flap/blob/master/LICENSE
*/

/* .--------------------------------------------------------------------------------. */
/* |  ___           __ _                    _    _       ___       __ _             | */
/* | / __|___ _ _  / _(_)__ _ _  _ _ _ __ _| |__| |___  |   \ ___ / _(_)_ _  ___ ___| */
/* || (__/ _ | ' \|  _| / _` | || | '_/ _` | '_ | / -_) | |) / -_|  _| | ' \/ -_(_-<| */
/* | \___\___|_||_|_| |_\__, |\_,_|_| \__,_|_.__|_\___| |___/\___|_| |_|_||_\___/__/| */
/* |                    |___/                                                       | */
/* '--------------------------------------------------------------------------------' */
/*
  These define statements can be changed as you desire for changing the functionality and
  behaviour of your device.
*/
#define SERIAL_ENABLE           //Uncomment for serial debug messages, no serial messages if this whole line is a comment!
#define UNIT_CALLS_DISABLE      //Disable the call to the units so can just debug the ESP
#define OTA_ENABLE              //Comment out to disable OTA functionality
#define UNITS_AMOUNT 10         //Amount of connected units !IMPORTANT TO BE SET CORRECTLY!
#define SERIAL_BAUDRATE 115200  //Serial debugging BAUD rate
#define WIFI_SETUP_MODE AP      //Option to either direct connect to a WiFi Network or setup a AP to configure WiFi. Options: AP or DIRECT

/* .--------------------------------------------------------. */
/* | ___         _               ___       __ _             | */
/* |/ __|_  _ __| |_ ___ _ __   |   \ ___ / _(_)_ _  ___ ___| */
/* |\__ | || (_-|  _/ -_| '  \  | |) / -_|  _| | ' \/ -_(_-<| */
/* ||___/\_, /__/\__\___|_|_|_| |___/\___|_| |_|_||_\___/__/| */
/* |     |__/                                               | */
/* '--------------------------------------------------------' */
/*
  These are important to maintain normal system behaviour. Only change if you know 
  what your doing.
*/
#define ANSWER_SIZE 1           //Size of unit's request answer
#define FLAP_AMOUNT 45          //Amount of Flaps in each unit
#define MIN_SPEED 1             //Min Speed
#define MAX_SPEED 12            //Max Speed
#define WEBSERVER_H             //Needed in order to be compatible with WiFiManager: https://github.com/me-no-dev/ESPAsyncWebServer/issues/418#issuecomment-667976368

/* .-----------------------------------. */
/* | _    _ _                 _        | */
/* || |  (_| |__ _ _ __ _ _ _(_)___ ___| */
/* || |__| | '_ | '_/ _` | '_| / -_(_-<| */
/* ||____|_|_.__|_| \__,_|_| |_\___/__/| */
/* '-----------------------------------' */
/*
  External library dependencies, not much more to say!
*/
//Specifically put here in this order to avoid conflict
#include <WiFiManager.h>
#include <ESP8266WiFi.h>

#include <Arduino.h>
#include <Arduino_JSON.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ezTime.h>
#include <LinkedList.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include "Classes.h"
#include "LittleFS.h"

//OTA Libary if we are into that kind of thing
#ifdef OTA_ENABLE
#include <ArduinoOTA.h>
#endif

/* .------------------------------------------------------------------------------------. */
/* |  ___           __ _                    _    _       ___     _   _   _              | */
/* | / __|___ _ _  / _(_)__ _ _  _ _ _ __ _| |__| |___  / __|___| |_| |_(_)_ _  __ _ ___| */
/* || (__/ _ | ' \|  _| / _` | || | '_/ _` | '_ | / -_) \__ / -_|  _|  _| | ' \/ _` (_-<| */
/* | \___\___|_||_|_| |_\__, |\_,_|_| \__,_|_.__|_\___| |___\___|\__|\__|_|_||_\__, /__/| */
/* |                    |___/                                                  |___/    | */
/* '------------------------------------------------------------------------------------' */
/*
  Settings you can feel free to change to customise how your display works.
*/
//Used if connecting via "WIFI_SETUP_MODE" of "AP"
const char* wifiDirectSsid = "";
const char* wifiDirectPassword = "";

//Change if you want to have an Over The Air (OTA) Password for updates
const char* otaPassword = "";

//Change this to your timezone, use the TZ database name
//https://en.wikipedia.org/wiki/List_of_tz_database_time_zones
const char* timezoneString = "Europe/London";

//If you want to have a different date or clock format change these two
//Complete table with every char: https://github.com/ropg/ezTime#getting-date-and-time
const char* dateFormat = "d.m.Y"; //Examples: d.m.Y -> 11.09.2021, D M y -> SAT SEP 21
const char* clockFormat = "H:i"; //Examples: H:i -> 21:19, h:ia -> 09:19PM

/* .------------------------------------------------------------. */
/* | ___         _               ___     _   _   _              | */
/* |/ __|_  _ __| |_ ___ _ __   / __|___| |_| |_(_)_ _  __ _ ___| */
/* |\__ | || (_-|  _/ -_| '  \  \__ / -_|  _|  _| | ' \/ _` (_-<| */
/* ||___/\_, /__/\__\___|_|_|_| |___\___|\__|\__|_|_||_\__, /__/| */
/* |     |__/                                          |___/    | */
/* '------------------------------------------------------------' */
/*
  Used for normal running of the system so changing things here might make things 
  behave a little strange.
*/
//The current version of code to display on the UI
const char* espVersion = "1.4.0";

//All the letters on the units that we have to be displayed. You can change these if it so pleases at your own risk
const char letters[] = {' ', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '$', '&', '#', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', ':', '.', '-', '?', '!'};
int displayState[UNITS_AMOUNT];
unsigned long previousMillis = 0;

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

//File paths to save input values permanently
const char* alignmentPath = "/alignment.txt";
const char* flapSpeedPath = "/flapspeed.txt";
const char* deviceModePath = "/devicemode.txt";
const char* countdownPath = "/countdown.txt";

//Variables for storing things for checking and use in normal running
String alignment = "";
String flapSpeed = "";
String inputText = "";
String previousDeviceMode = "";
String currentDeviceMode = "";
String countdownToDateUnix = "";
String lastWrittenText = "";
String lastReceivedMessageDateTime = "";
long newMessageScheduledDateTimeUnix = 0;
bool newMessageScheduleEnabled = false;
bool isPendingReboot = false;
bool isPendingUnitsReset = false;
LinkedList<ScheduledMessage> scheduledMessages;
Timezone timezone; 

//Create AsyncWebServer object on port 80
AsyncWebServer webServer(80);

//Used for creating a Access Point to allow WiFi setup
WiFiManager wifiManager;
bool wifiConfigured = false;

//Used to denote that the system has gone into OTA mode
#ifdef OTA_ENABLE
bool isInOtaMode = 0;
#endif

/* .-----------------------------------------------. */
/* | ___          _          ___     _             | */
/* ||   \ _____ _(_)__ ___  / __|___| |_ _  _ _ __ | */
/* || |) / -_\ V | / _/ -_) \__ / -_|  _| || | '_ \| */
/* ||___/\___|\_/|_\__\___| |___\___|\__|\_,_| .__/| */
/* |                                         |_|   | */
/* '-----------------------------------------------' */
void setup() {
#ifdef SERIAL_ENABLE
  Serial.begin(SERIAL_BAUDRATE);
#endif

  SerialPrintln("#######################################################");
  SerialPrintln("Master module starting...");

  //De-activate I2C if debugging the ESP, otherwise serial does not work
#ifndef SERIAL_ENABLE
  //For ESP01 only
  Wire.begin(1, 3); 
#endif
  //Wire.begin(D1, D2); //For NodeMCU testing only SDA=D1 and SCL=D2

  //Load and read all the things
  initialiseFileSystem();
  loadValuesFromFileSystem();
  //wifiManager.resetSettings();
  initWiFi();

  if (wifiConfigured && !isPendingReboot) {
    //ezTime initialization
    waitForSync();
    timezone.setLocation(timezoneString);

#ifdef OTA_ENABLE
    SerialPrintln("OTA is enabled! Yay!");
#endif

    //Web Server Endpoint configuration
    webServer.serveStatic("/", LittleFS, "/");
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
      SerialPrintln("Request Home Page Received");

      request->send(LittleFS, "/index.html", "text/html");
    });

    webServer.on("/settings", HTTP_GET, [](AsyncWebServerRequest * request) {
      SerialPrintln("Request for Settings Received");
      
      String json = getCurrentSettingValues();
      request->send(200, "application/json", json);
      json = String();
    });
    
    webServer.on("/health", HTTP_GET, [](AsyncWebServerRequest * request) {
      SerialPrintln("Request for Health Check Received");
      
      String json = getCurrentSettingValues();
      request->send(200, "text/plain", "Healthy");
    });
    
    webServer.on("/reboot", HTTP_GET, [](AsyncWebServerRequest * request) {
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
    
    webServer.on("/reset-units", HTTP_GET, [](AsyncWebServerRequest * request) {
      SerialPrintln("Request to Reset Units Received");
      
      //This will be picked up in the loop
      isPendingUnitsReset = 1;
      
      request->redirect("/?is-resetting-units=true");
    });
    
    webServer.on("/reset-wifi", HTTP_GET, [](AsyncWebServerRequest * request) {
      SerialPrintln("Request to Reset WiFi Received");
      
#if WIFI_SETUP_MODE == AP
      SerialPrintln("WiFi mode set to AP so will reset WiFi settings");
      IPAddress ip = WiFi.localIP();
      
      String html = "<div style='text-align:center'>";
      html += "<font face='arial'><h1>Split Flap - Resetting WiFi...</h1>";
      html += "<p>WiFi Settings have been erased. Device will now reboot...<p>";
      html += "<p>You will now be able to connect to this device in AP mode to configure the WiFi once more<p>";
      html += "<p>You can go to the main home page after this time by clicking the button below or going to '/'.</p>";
      html += "<p><a href=\"http://" + ip.toString() + "\">Home</a></p>";
      html += "</font>";
      html += "</div>";
      
      request->send(200, "text/html", html);

      delay(5024);

      SerialPrintln("Done waiting");
      
      //wifiManager.resetSettings();
      //isPendingReboot = 1;
#else
      SerialPrintln("WiFi mode is not AP so nothing can be done here");
      IPAddress ip = WiFi.localIP();
      
      String html = "<div style='text-align:center'>";
      html += "<font face='arial'><h1>Split Flap - Unable to Reset WiFi</h1>";
      html += "<p>Device is not configured via AP. WiFi settings can only be updated via a Sketch upload via Arduino IDE.<p>";
      html += "<p>You can go to the main home page after this time by clicking the button below or going to '/'.</p>";
      html += "<p><a href=\"http://" + ip.toString() + "\">Home</a></p>";
      html += "</font>";
      html += "</div>";
      
      request->send(200, "text/html", html);
#endif      
    });

    webServer.on("/scheduled-message/remove", HTTP_DELETE, [](AsyncWebServerRequest * request) {
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

    webServer.on("/", HTTP_POST, [](AsyncWebServerRequest * request) {
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

#ifdef OTA_ENABLE
    webServer.on("/ota", HTTP_GET, [](AsyncWebServerRequest * request) {
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

    delay(250);
    webServer.begin();

    SerialPrintln("Master module ready!");
    SerialPrintln("#######################################################");
  }
  else {
    if (isPendingReboot) {
      SerialPrintln("Reboot is pending to be able to continue device function. Hold please...");
      SerialPrintln("#######################################################");
    }
    else {
      SerialPrintln("Unable to connect to WiFi... Not starting web server");
      SerialPrintln("Please hard restart your device to try connect again");
      SerialPrintln("#######################################################");
    }
  }
}

/* .----------------------------------------------------. */
/* | ___                _             _                 | */
/* || _ \_  _ _ _  _ _ (_)_ _  __ _  | |   ___ ___ _ __ | */
/* ||   | || | ' \| ' \| | ' \/ _` | | |__/ _ / _ | '_ \| */
/* ||_|_\\_,_|_||_|_||_|_|_||_\__, | |____\___\___| .__/| */
/* |                          |___/               |_|   | */
/* '----------------------------------------------------' */
void loop() {
  //Reboot in here as if we restart within a request handler, no response is returned
  if (isPendingReboot) {
    SerialPrintln("Rebooting Now... Fairwell!");
    SerialPrintln("#######################################################");
    delay(200);
    ESP.restart();
    return;
  }

  //Do nothing if WiFi is not configured
  if (!wifiConfigured) {
    //Show there is an error via text on display
    currentDeviceMode = DEVICE_MODE_TEXT;
    alignment = ALIGNMENT_MODE_CENTER;
    showText("OFFLINE");
    delay(100);

    return;
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
      showText(timezone.dateTime(dateFormat));
    } 
    else if (currentDeviceMode == DEVICE_MODE_CLOCK) {
      showText(timezone.dateTime(clockFormat));
    } 
  }
}
