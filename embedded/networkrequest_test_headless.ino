#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0F, 0x4D, 0x49 };
char server[] = "http://smart-seating.herokuapp.com";    // name address for Google (using DNS)
IPAddress ip(192, 168, 0, 177);
EthernetClient client;

void setup() {
  
  pinMode(4,OUTPUT); // Disable W5100 SD module
  digitalWrite(4,HIGH);

  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    ; // Failed to configure using DHCP
    // try to congifure using IP address instead of DHCP:
    Ethernet.begin(mac, ip);
  }

  // W5100 init delay
  delay(1000);

  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) {
    // Make a HTTP request:
    client.println("GET /read/moulees2 HTTP/1.1");
    client.println("Host: smart-seating.herokuapp.com");
    client.println("Connection: close");
    client.println();
  } 
  
  else {
    pinMode(8, OUTPUT);
    digitalWrite(8, HIGH);
  }
  
}

void loop() {

  // Read server response
  if (client.available()) {
    char c = client.read();
  }

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    client.stop();
    while(true);
  }
}
