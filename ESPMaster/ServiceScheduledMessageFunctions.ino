//Check if scheduled message is due to be shown
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