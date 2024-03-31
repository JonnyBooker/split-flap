//Replace/add a scheduled message and persist the result afterwards
void addAndPersistScheduledMessage(String scheduledText, long scheduledDateTimeUnix, bool showIndefinitely) {
  addScheduledMessage(scheduledText, scheduledDateTimeUnix, showIndefinitely);
  writeScheduledMessagesToFile();
}

//Replace/add a scheduled message
void addScheduledMessage(String scheduledText, long scheduledDateTimeUnix, bool showIndefinitely) {
  SerialPrintln("Processing New Scheduled Message");
  SerialPrintln("-- Message: " + scheduledText);
  SerialPrint("-- Unix: ");
  SerialPrintln(scheduledDateTimeUnix);
  SerialPrint("-- Show Indefinitely: ");
  SerialPrintln(showIndefinitely ? "Yes" : "No");

  //Find the existing scheduled message and update it if one exists
  for(int scheduledMessageIndex = 0; scheduledMessageIndex < scheduledMessages.size(); scheduledMessageIndex++) {
    ScheduledMessage scheduledMessage = scheduledMessages[scheduledMessageIndex];

    if (scheduledDateTimeUnix == scheduledMessage.ScheduledDateTimeUnix) {
      SerialPrintln("Removing Existing Scheduled Message due to be shown, it will be replaced");
      scheduledMessages.remove(scheduledMessageIndex);
      break;
    }
  }

  //Add the new scheduled message with the new value
  if (scheduledDateTimeUnix > timezone.now()) {
    SerialPrintln("Adding new Scheduled Message");
    scheduledMessages.add({scheduledText, scheduledDateTimeUnix, showIndefinitely});
  }
  else {
    SerialPrintln("Not adding Scheduled Message as it is in the past");
  }
}

//Remove and delete a existing scheduled message if found
bool removeScheduledMessage(long scheduledDateTimeUnix) {
  //Find the existing scheduled message and delete it
  for(int scheduledMessageIndex = 0; scheduledMessageIndex < scheduledMessages.size(); scheduledMessageIndex++) {
    ScheduledMessage scheduledMessage = scheduledMessages[scheduledMessageIndex];

    if (scheduledDateTimeUnix == scheduledMessage.ScheduledDateTimeUnix) {
      SerialPrintln("Deleting Scheduled Message due to be shown: " + scheduledMessage.Message);
      scheduledMessages.remove(scheduledMessageIndex);
      
      writeScheduledMessagesToFile();

      return true;
    }
  }
  
  return false;
}

//Check if scheduled message is due to be shown
void checkScheduledMessages() {   
  //Iterate over the current bunch of scheduled messages. If we find one where the current time exceeds when we should show
  //the message, then we need to show that message immediately
  unsigned long currentTimeUnix = timezone.now();
  for(int scheduledMessageIndex = 0; scheduledMessageIndex < scheduledMessages.size(); scheduledMessageIndex++) {
    ScheduledMessage scheduledMessage = scheduledMessages[scheduledMessageIndex];

    if (currentTimeUnix > scheduledMessage.ScheduledDateTimeUnix) {
      SerialPrintln("Scheduled Message due to be shown: " + scheduledMessage.Message);
      SerialPrint("Scheduled Message to be Shown Indefinitely: ");
      SerialPrintln(scheduledMessage.ShowIndefinitely ? "Yes" : "No");

      if (scheduledMessage.ShowIndefinitely) {
        deviceMode = DEVICE_MODE_TEXT;
        inputText = scheduledMessage.Message;
        showText(scheduledMessage.Message);
      }
      else {
        showText(scheduledMessage.Message, scheduledMessageDisplayTimeMillis);
      }      

      scheduledMessages.remove(scheduledMessageIndex);
      writeScheduledMessagesToFile();
      break;
    }
  }
}

//Parse JSON scheduled messages into the current known scheduled messages
void readScheduledMessagesFromJson(String scheduledMessagesJson) {
  if (scheduledMessagesJson != "") {    
    int addedScheduledMessageCount = 0;

    JsonDocument jsonDocument;
    DeserializationError deserialisationError = deserializeJson(jsonDocument, scheduledMessagesJson);

    if (!deserialisationError) {
      if (jsonDocument.is<JsonArray>()) {
        for (JsonVariant value : jsonDocument.as<JsonArray>()) {
          long scheduledDateTimeUnix = value["scheduledDateTimeUnix"];
          String message = value["message"];
          bool showIndefinitely = value["showIndefinitely"];

          addScheduledMessage(message, scheduledDateTimeUnix, showIndefinitely);
          addedScheduledMessageCount++;
        }
        
        //If there is a difference in scheduled messages, re-write the messages
        if (addedScheduledMessageCount != scheduledMessages.size()) {
          SerialPrintln("Read message count and scheduled message count differ, writing updated messages");
          writeScheduledMessagesToFile();
        }
      } 
      else {
        SerialPrintln("Invalid JSON Array found, scrapping and starting again");
        writeEmptyScheduledMessagesToFile();
      }
    }
    else {
      SerialPrintln("Invalid JSON found, scrapping and starting again");
      writeEmptyScheduledMessagesToFile();
    }
  }
}

//Convert the scheduled messages to a JSON document and save to file
void writeScheduledMessagesToFile() {
  if (scheduledMessages.size()) {
    JsonDocument scheduledMessagesDocument;
    for(int scheduledMessageIndex = 0; scheduledMessageIndex < scheduledMessages.size(); scheduledMessageIndex++) {
      ScheduledMessage scheduledMessage = scheduledMessages[scheduledMessageIndex];
      
      scheduledMessagesDocument[scheduledMessageIndex]["scheduledDateTimeUnix"] = scheduledMessage.ScheduledDateTimeUnix;
      scheduledMessagesDocument[scheduledMessageIndex]["message"] = scheduledMessage.Message;
      scheduledMessagesDocument[scheduledMessageIndex]["showIndefinitely"] = scheduledMessage.ShowIndefinitely;
    }

    String scheduledMessagesJson;
    serializeJson(scheduledMessagesDocument, scheduledMessagesJson);
    writeFile(LittleFS, scheduledMessagesPath, scheduledMessagesJson.c_str());
  }
  else {
    SerialPrintln("No Scheduled Messages Left - Writing Empty to File");
    writeEmptyScheduledMessagesToFile();
  }
}

//Write an empty array to the schedule messages file, used in case something goes terribly wrong
void writeEmptyScheduledMessagesToFile() {
  writeFile(LittleFS, scheduledMessagesPath, "[]");
}
