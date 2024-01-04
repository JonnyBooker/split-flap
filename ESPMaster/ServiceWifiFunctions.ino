//Initialize WiFi
void initWiFi() {
  int wifiConnectTimeoutSeconds = 300;

  WiFi.mode(WIFI_STA);
  WiFi.hostname("Split-Flap");

#if WIFI_SETUP_MODE == AP
  SerialPrintln("Setting up WiFi AP Setup Mode");

  wifiManager.setTitle("Split-Flap Setup");
  wifiManager.setDarkMode(true);
  wifiManager.setShowInfoUpdate(false);
  wifiManager.setConfigPortalBlocking(true);
  wifiManager.setConfigPortalTimeout(wifiConnectTimeoutSeconds);
  wifiManager.setConnectTimeout(60);
  
  //Set the menu options
  std::vector<const char *> menu = { "wifi", "info" ,"param", "sep", "restart", "exit" };
  wifiManager.setMenu(menu);

  SerialPrintln("Attempting to connect to WiFi... Will fallback to AP mode to allow configuring of WiFi if fails...");
  if(wifiManager.autoConnect("Split-Flap-AP")) {
    SerialPrint("Successfully Connected to WiFi. IP Address: ");
    SerialPrintln(WiFi.localIP());

    wifiConfigured = true;
  }

#else
  SerialPrintln("Setting up WiFi Direct");

  if (ssid != "" && password != "") {
    int maxAttemptsCount = 0;
    
    WiFi.begin(ssid, password);
    SerialPrint("Connecting");

    while (WiFi.status() != WL_CONNECTED && maxAttemptsCount != wifiConnectTimeoutSeconds) {
      if (maxAttemptsCount % 10 == 0) {
        SerialPrint('\n');
      }
      else {
        SerialPrint('.');
      }

      delay(1000);

      maxAttemptsCount++;      
    }

    //If we reached the max timeout
    if (maxAttemptsCount != wifiConnectTimeoutSeconds) {
      SerialPrint("Successfully Connected to WiFi. IP Address: ");
      SerialPrintln(WiFi.localIP());

      wifiConfigured = true;
    }
  }

#endif
}
