/*  ___   ___  ___  _   _  ___   ___   ____ ___  ____  
 * / _ \ /___)/ _ \| | | |/ _ \ / _ \ / ___) _ \|    \ 
 *| |_| |___ | |_| | |_| | |_| | |_| ( (__| |_| | | | |
 * \___/(___/ \___/ \__  |\___/ \___(_)____)___/|_|_|_|
 *                  (____/ 
 * Make RFIC module and MEGA-IoT shield to make a remote control security door system
 * Tutorial URL  http://osoyoo.com/?p=29001
 * CopyRight www.osoyoo.com
 */
#include <Servo.h>
Servo head;
#define SERVO_PIN 3
#define RED_LED 11
#define GREEN_LED 12
#define BUZZER 5

#include <SPI.h>
#include <RFID.h>
unsigned char my_rfid[] = {186,11,86,89,190}; // read http://osoyoo.com/?p=28943 to get your RFID value and replace this line
RFID rfid(48,49);   //D48--RFID module SDA pin、D49 RFID module RST pin

#include "WiFiEsp.h"
//#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial softserial(A9, A8); // A9 to ESP_TX, A8 to ESP_RX by default
//#endif

char ssid[] = "****"; // replace ****** with your network SSID (name)
char pass[] = "****"; // replace ****** with your network password
int status = WL_IDLE_STATUS;

int DoorStatus = LOW;

WiFiEspServer server(80);

// use a ring buffer to increase speed and reduce memory allocation
RingBuffer buf(8);

void setup()
{ 
  pinMode(RED_LED,OUTPUT);
  pinMode(GREEN_LED,OUTPUT);
  pinMode(BUZZER,OUTPUT);

  head.attach(SERVO_PIN); 
  close_door();
  SPI.begin();
  rfid.init();
  pinMode(LED_BUILTIN, OUTPUT);	// initialize digital pin LED_BUILTIN as an output.
  Serial.begin(9600);   // initialize serial for debugging
    softserial.begin(115200);
  softserial.write("AT+CIOBAUD=9600\r\n");
  softserial.write("AT+RST\r\n");
  softserial.begin(9600);    // initialize serial for ESP module
  WiFi.init(&softserial);    // initialize ESP module

  // check for the presence of the shield
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue
    while (true);
  }

  // attempt to connect to WiFi network
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network
    status = WiFi.begin(ssid, pass);
  }

  Serial.println("You're connected to the network");
  printWifiStatus();
  
  // start the web server on port 80
  server.begin();
}


void loop()
{
   //search card
  if (rfid.isCard()) {
    Serial.println("Find the card!");
   //read serial number
    if (rfid.readCardSerial()) {

      if (compare_rfid(rfid.serNum,my_rfid))
      {
      Serial.println("match");
       open_door();
    }
      else
      { 
             Serial.println("not match");
        close_door();
       digitalWrite(BUZZER,HIGH);
       
      }
    }
    rfid.selectTag(rfid.serNum);
  }
  
  rfid.halt();
  
  WiFiEspClient client = server.available();  // listen for incoming clients

  if (client) {                               // if you get a client,
    Serial.println("New client");             // print a message out the serial port
    buf.init();                               // initialize the circular buffer
    while (client.connected()) {              // loop while the client's connected
      if (client.available()) {               // if there's bytes to read from the client,
        char c = client.read();               // read a byte, then
        buf.push(c);                          // push it to the ring buffer

        // printing the stream to the serial monitor will slow down
        // the receiving of data from the ESP filling the serial buffer
        //Serial.write(c);
        
        // you got two newline characters in a row
        // that's the end of the HTTP request, so send a response
        if (buf.endsWith("\r\n\r\n")) {
          sendHttpResponse(client);
          break;
        }

        // Check to see if the client request was "GET /H" or "GET /L":
        if (buf.endsWith("GET /H")) {
          Serial.println("Open door");
          open_door();
          DoorStatus = HIGH;
        }
        else if (buf.endsWith("GET /L")) {
          Serial.println("Close Door");
          close_door();
          DoorStatus = LOW;
        }      else if (buf.endsWith("GET /Q")) {
          Serial.println("Turn OFF Buzzer");
          digitalWrite(BUZZER, LOW);    // turn the LED off by making the voltage LOW
        }
      }
    }
    
    // close the connection
    client.stop();
    Serial.println("Client disconnected");
  }
}


void sendHttpResponse(WiFiEspClient client)
{
  // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
  // and a content-type so the client knows what's coming, then a blank line:
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();
  
  // the content of the HTTP response follows the header:
  client.print("The Door is ");
  if (DoorStatus==1) 
    client.print("Open");
  else client.print("Close");
  client.println("<p>");
  client.println("Click <a href=\"/H\">here</a> Open Door<br>");
  client.println("Click <a href=\"/L\">here</a> Close Door<br>");
  client.println("Click <a href=\"/Q\">here</a> Turn off Buzzer<br>");
   
  // The HTTP response ends with another blank line:
  client.println();
}

void printWifiStatus()
{
  // print the SSID of the network you're attached to
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print where to go in the browser
  Serial.println();
  Serial.print("To see this page in action, open a browser to http://");
  Serial.println(ip);
  Serial.println();
}
boolean compare_rfid(unsigned char x[],unsigned char y[])
{
  for (int i=0;i<5;i++)
  {
    if(x[i]!=y[i]) return false;
  }
  return true;
}
void open_door()
{
    head.write(90);
    digitalWrite(GREEN_LED,HIGH);
    digitalWrite(RED_LED,LOW);
    digitalWrite(BUZZER,LOW);
    DoorStatus = HIGH;
}
void close_door()
{
    head.write(0);
    digitalWrite(GREEN_LED,LOW);
    digitalWrite(RED_LED,HIGH);
    DoorStatus = LOW;
}
