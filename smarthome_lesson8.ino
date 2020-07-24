/*
 OSOYOO Smarthome example: Remote Control Servo
 
 A simple web server that lets you control a servor via a web page.
 This sketch will print the IP address of your ESP8266 module (once connected)
 to the Serial monitor. From there, you can open that address in a web browser
 to control the servo on pin D9, when the servo turn to 180 degree,the green led
 on D4 will be lit.

 For more details see: http://osoyoo.com/?p=28938
*/

#include "WiFiEsp.h"
#include <Servo.h>
Servo myservo;//create servo object to control a servo
#define greenled 12 //connect greenled to digital pin4
// Emulate softserial on pins A9/A8 if not present
//#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial softserial(A9, A8); // RX, TX
//#endif

char ssid[] = "NETGEAR00";            // your network SSID (name)
char pass[] = "weijianweiye";        // your network password
int status = WL_IDLE_STATUS;

int ledStatus = LOW;

WiFiEspServer server(80);

// use a ring buffer to increase speed and reduce memory allocation
RingBuffer buf(8);

void setup()
{
  myservo.attach(3);//attachs the servo on pin 9 to servo object
  myservo.write(0);//back to 0 degrees 
 delay(15);//wait for a second
  pinMode(greenled, OUTPUT);	// initialize digital pin greenled as an output.
  Serial.begin(9600);   // initialize serial for debugging

  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  Serial.println("start");
  // set the data rate for the SoftwareSerial port
  softserial.begin(115200);
  softserial.println("AT+UART_DEF=9600,8,1,0,0\r\n");
  softserial.write("AT+RST\r\n");
  delay(100);
  softserial.begin(9600);
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
          Serial.println("OPEN THE DOOR!");
          ledStatus = HIGH;
          digitalWrite(greenled, HIGH);   // turn the led on (HIGH is the voltage level)
          myservo.write(180);//servo goes to 90 degrees 
         //delay(15);//wait for a second
        }
        else if (buf.endsWith("GET /L")) {
          Serial.println("CLOSE THE DOOR!");
          ledStatus = LOW;
          digitalWrite(greenled, LOW);    // turn the led off by making the voltage LOW
          myservo.write(0);//servo goes to 0 degrees 
         //delay(15);//wait for a second
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
  if (ledStatus == 1)
    client.println("OPEN");
    else  client.println("CLOSED");
  //client.print(ledStatus);
  client.println("<br>");
  client.println("<br>");
  
  client.println("Click <a href=\"/H\">here</a> open the door<br>");
  client.println("Click <a href=\"/L\">here</a> close the door<br>");
  
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
