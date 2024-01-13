//Replace/add a scheduled message
void addScheduledMessage(String scheduledText, long scheduledDateTimeUnix) {
  SerialPrintln("Processing New Scheduled Message");

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
    scheduledMessages.add({scheduledText, scheduledDateTimeUnix});
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
      showText(scheduledMessage.Message, 7500);

      scheduledMessages.remove(scheduledMessageIndex);
      break;
    }
  }
}