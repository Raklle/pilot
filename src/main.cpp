#include <Arduino.h>
#include <SoftwareSerial.h>
#include <IRremote.hpp>
#include <tuple>
#include <vector>
#include <WiFi.h>

#define IR_RECEIVE_PIN 4
#define IR_SEND_PIN 2
#define MAX_COMMANDS 20

char cmd;



const char* ssid = "AndroidAP1234";
const char* password = "pbrg5463";
WiFiServer server(80);
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;


String header;


bool sendCommand(int i);
bool deleteCommand(int i);
int getCode(String name);

struct Command {
    decode_type_t protocol;
    uint16_t address;
    uint16_t command;
    String name;
    bool deleted;
};

Command commands[MAX_COMMANDS];
int usedCommands = 0;

void printCommandsAsHTMLTable(Client& client) {

    client.println("<table>");

    client.println("<tr><th>Send</th><th>Protocol</th><th>Command</th><th>Delete</th></tr>");

    for (int i = 0; i < usedCommands; ++i) {
      if (!commands[i].deleted){
        client.print("<tr><td>");
        client.print("<p><a href=\"/send/0");
        client.print(i);
        client.print("\"><button class=\"button\">");
        client.print(commands[i].name);
        client.print("</button></a></p>");
        client.print("</td><td>");
        client.print(commands[i].protocol);
        client.print("</td><td>");
        client.print(commands[i].command);
        client.print("</td><td>");
        client.print("<p><a href=\"/delet/0");
        client.print(i);
        client.print("\"><button class=\"button\">");
        client.print("Delete");
        client.print("</button></a></p>");
        client.println("</td></tr>");
      }
    }

    client.println("</table>");
}

bool isDuplicate(Command newCommand) {
    if (usedCommands == 0) {
        return false;
    }

    Command lastCommand = commands[usedCommands - 1];

    if (lastCommand.protocol == newCommand.protocol &&
        lastCommand.address == newCommand.address &&
        lastCommand.command == newCommand.command) {
        return true;
    }

    return false;
}


String urlDecode(String input);

void startServer()
{
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}


void setup() {
  Serial.begin(9600);
  IrReceiver.begin(IR_RECEIVE_PIN);
  IrSender.begin(IR_SEND_PIN);
  Serial.print("\n");
  startServer();
}

void processServer(){
  WiFiClient client = server.available();
  if (client) {
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");
    String currentLine = "";
    while (client.connected() && currentTime - previousTime <= timeoutTime) {
      currentTime = millis();
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        header += c;
        if (c == '\n') {
          if (currentLine.length() == 0) {

            if (header.indexOf("GET /save") >= 0) {
              int nameIndex = header.indexOf("name=");
              if (nameIndex >= 0) {
                int nameEndIndex = header.indexOf(" ", nameIndex);
                if (nameEndIndex < 0) {
                  nameEndIndex = header.indexOf("&", nameIndex);
                }
                if (nameEndIndex >= 0) {
                  String nameValue = header.substring(nameIndex + 5, nameEndIndex);
                  nameValue = urlDecode(nameValue);
                  Serial.println("Send ir code");
                  getCode(nameValue);
                  Serial.println("Saving: " + nameValue);
                }
              }
            }else if (header.indexOf("GET /send") >= 0){
              int startIndex = header.indexOf("/send/");
              String commandIndex = header.substring(startIndex + 6, startIndex + 8);
              Serial.println("Sending: " + commandIndex);
              int i = commandIndex.toInt();
              sendCommand(i);
            }else if (header.indexOf("GET /delet") >= 0){
              int startIndex = header.indexOf("/delet/");
              String commandIndex = header.substring(startIndex + 7, startIndex + 9);
              Serial.println("Deleting: " + commandIndex);
              int i = commandIndex.toInt();
              deleteCommand(i);
            }


            // HTTP header
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            //Begining of HTML page
            
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");

            client.println("<style>table { font-family: arial, sans-serif; border-collapse: collapse; width: 100%;}td, th { border: 1px solid #dddddd; text-align: left; padding: 8px;}tr:nth-child(even) { background-color: #dddddd;}</style>");
            client.println("</head><body>");

            printCommandsAsHTMLTable(client);

            client.println("<form action=\"/save\"><input type=\"text\" name=\"name\"> <input type=\"submit\" value=\"Dodaj\"></form>");

            client.println("</body></html>");
            client.println();
        
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }

    header = "";

    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

String urlDecode(String input) {
  String decoded = "";
  char temp[] = "00";
  unsigned int len = input.length();
  unsigned int i = 0;
  while (i < len) {
    char decodedChar;
    char encodedChar = input.charAt(i++);
    if (encodedChar == '%') {
      if (i + 1 < len) {
        temp[0] = input.charAt(i++);
        temp[1] = input.charAt(i++);
        decodedChar = strtol(temp, NULL, 16);
      }
    } else if (encodedChar == '+') {
      decodedChar = ' ';
    } else {
      decodedChar = encodedChar;
    }
    decoded += decodedChar;
  }
  return decoded;
}

void loop()
{
  processServer();
}

void addCommand(const Command& newCommand)
{
  if (usedCommands < MAX_COMMANDS) {
      commands[usedCommands] = newCommand;
      usedCommands++;
  } else {
      Serial.println("No empty space available!");
  }
}


bool sendCommand(int i)
{
  Serial.println("sending ");
  IrSender.sendSamsung(commands[i].address, commands[i].command, 3);
  // IrSender.sendNEC(commands[i].address, commands[i].command, 3);

  return true;
}

int getCode(String name)
{

  while(!IrReceiver.decode())
  {
    IrReceiver.resume();

    if(IrReceiver.decodedIRData.command != 0 && !isDuplicate({IrReceiver.decodedIRData.protocol, IrReceiver.decodedIRData.address, IrReceiver.decodedIRData.command, name})){
      addCommand({IrReceiver.decodedIRData.protocol, IrReceiver.decodedIRData.address, IrReceiver.decodedIRData.command, name});
      return 0;
    }
  }

  
}

bool deleteCommand(int i){
    commands[i].deleted = true;
}

