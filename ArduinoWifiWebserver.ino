#include <WiFiNINA.h>

//Don't forget to create "arduino_secrets.h" (using "arduino_secrets_template.h" as a template)
#include "arduino_secrets.h" 

#define NUM_ANALOG_INPUTS 6
#define NUM_DIGITAL_INPUTS 14

WiFiServer server(80);
//If you want to set IP address manually use something like
//WiFi.config(IPAddress(192, 168, 0, 77));
//Otherwise IP address will be assigned automatically (192.168.4.1 if not overridden by WiFi network)
// - you can check it in Serial Monitor

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT); //Will be used for /builtinLedOn and /builtinLedOff commands
  while (!connectToWifiNetwork()) {
    delay(5000);
  }
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String currentLine = "";
    String command = ""; //GET request command specified after "/"
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // Serial.write(c);  //For debug purposes only
        if (c == '\n') {
          //two EOL symbols in a row means end of request - time to response
          if (currentLine.length() == 0) {
            processCommand(client, command);
            break; //exit while (client.connected()) 
          } else {
            if (currentLine.startsWith("GET /")) {
              int endOfCommand = currentLine.indexOf(' ', 5);
              command = (endOfCommand == -1) ? currentLine.substring(5) : currentLine.substring(5, endOfCommand);
            }
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    delay(1); // give the web browser some time to receive the data
    client.stop();
  }
}

boolean connectToWifiNetwork() {
  char ssid[] = SECRET_SSID;
  char pass[] = SECRET_PASS;
  int status = WL_IDLE_STATUS;
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    return false;
  }
  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.print("Current firmware version ");
    Serial.print(fv);
    Serial.print(" is outdated. Please consider upgrading firmware to version ");
    Serial.println(WIFI_FIRMWARE_LATEST_VERSION);
  }
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);
    // wait 10 seconds for connection:
    delay(10000);
  }
  server.begin();
  printWifiStatusToSerial();
  return true;
}

void printWifiStatusToSerial() {
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}

void processCommand(WiFiClient& client, String& command) {
  if (command == "analogStatus") {
    printAnalogInputsStatus(client);
  } else if (command == "digitalStatus") {
    printDigitalInputsStatus(client);
  } else if (command == "wifiStatus") {
    printWiFiStatus(client);
  } else if (command == "builtinLedOn") {
    setLedValueAndPrintResponse(client, HIGH);
  } else if (command == "builtinLedOff") {
    setLedValueAndPrintResponse(client, LOW);
  } else {
    printDefaultResponse(client, command);
  }
}

void printAvailableCommands(WiFiClient& client) {
  client.println("Available commands:<br />");
  client.print("<a href=\"/analogStatus\">/analogStatus</a><br />");
  client.print("<a href=\"/digitalStatus\">/digitalStatus</a><br />");
  client.print("<a href=\"/wifiStatus\">/wifiStatus</a><br />");
  client.print("<a href=\"/builtinLedOn\">/builtinLedOn</a><br />");
  client.print("<a href=\"/builtinLedOff\">/builtinLedOff</a><br />");
}

void printResponseHeader(WiFiClient& client, int refreshRateSec) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close");
  if (refreshRateSec > 0) {
    client.print("Refresh: ");
    client.println(refreshRateSec);
  }
  client.println();
  client.println("<!DOCTYPE HTML>");
}

void printAnalogInputsStatus(WiFiClient& client) {
  printResponseHeader(client, 5);
  client.println("<html>");
  client.println("Analog inputs:<br />");
  for (int i = 0; i < NUM_ANALOG_INPUTS; ++i) {
    int inputValue = analogRead(i);
    client.print("A");
    client.print(i);
    client.print(": ");
    client.print(inputValue);
    client.println("<br />");
  }
  client.println("</html>");
}

void printDigitalInputsStatus(WiFiClient& client) {
  printResponseHeader(client, 5);
  client.println("<html>");
  client.println("Digital inputs:<br />");
  for (int i = 0; i < NUM_DIGITAL_INPUTS; ++i) {
    int inputValue = digitalRead(i);
    client.print(i);
    client.print(": ");
    client.print(inputValue);
    client.println("<br />");
  }
  client.println("</html>");
}

void printWiFiStatus(WiFiClient& client) {
  printResponseHeader(client, 20);
  client.println("<html>");
  client.print("SSID: ");
  client.print(WiFi.SSID());
  client.println("<br />");

  IPAddress ip = WiFi.localIP();
  client.print("IP Address: ");
  client.print(ip);
  client.println("<br />");

  long rssi = WiFi.RSSI();
  client.print("signal strength (RSSI):");
  client.print(rssi);
  client.println(" dBm<br />");
  client.println("</html>");
}

void printDefaultResponse(WiFiClient& client, String& command) {
  printResponseHeader(client, 0);
  client.println("<html>");
  if (command.length() != 0) {
    client.print("Unknown command: ");
    client.print(command);
    client.println("<br />");
  }
  printAvailableCommands(client);
  client.println("</html>");
}

//ledValue expected to be LOW or HIGH
void setLedValueAndPrintResponse(WiFiClient& client, int ledValue) {
  digitalWrite(LED_BUILTIN, ledValue);
  client.println("<html>");
  client.print("Builtin led is set to ");
  client.print(ledValue);
  client.println("<br />");
  printAvailableCommands(client);
  client.println("</html>");
}
