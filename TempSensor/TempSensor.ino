/*
 created 29 Jun. 2015
 by Ivan Marchand

 This example code is in the public domain.

 */

#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>

#include <IRremote.h>

#include <DHT.h>
#define DHTPIN 4     // what pin we're connected to
#define DHTTYPE DHT22   // DHT 11


DHT dht(DHTPIN, DHTTYPE);
YunServer server;
IRsend irsend;

IRrecv irrecv(11);
decode_results results;

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
  // Initialize Bridge
  Bridge.begin();
  // Initialize Server
  server.listenOnLocalhost();
  server.begin();
  irrecv.enableIRIn(); // Start the receiver
}

void addJsonString(YunClient client, String key, String value, bool first=false) {
    if (first) {
      client.print("{\"");
    }
    else {
      client.print(",\"");
    }
    client.print(key);
    client.print("\":\"" );
    client.print(value);
    client.print("\"" );
}

void addJsonFloat(YunClient client, String key, float value, bool first=false) {
    if (first) {
      client.print("{\"");
    }
    else {
      client.print(",\"");
    }
    client.print(key);
    client.print("\":" );
    client.print(value);
}

void addJsonInt(YunClient client, String key, int value, bool first=false) {
    if (first) {
      client.print("{\"");
    }
    else {
      client.print(",\"");
    }
    client.print(key);
    client.print("\":" );
    client.print(value);
}

void addJsonULong(YunClient client, String key, unsigned long value, bool first=false) {
    if (first) {
      client.print("{\"");
    }
    else {
      client.print(",\"");
    }
    client.print(key);
    client.print("\":" );
    client.print(value);
}

#ifdef IR_RAW 
void sendIRRaw(YunClient client) {
  unsigned int data[200] = {0};
  unsigned int i = 0;
  char last = '\0';
  for ( ; i < 200 ; i++)
  {
    data[i] = client.parseInt() * 10;
    last = client.read();
    if (last != ',')
    {
      break;
    }
  }
  int hz = 38;
  if (last == '/')
  {
    hz = client.parseInt();
  }
  addJson(client, "size", i+1);
  addJson(client, "khz", hz);

  // Send IR code
  irsend.sendRaw(data, i+1, hz);
}
#endif

void dumpToJson(YunClient client, decode_results *results) {
  int count = results->rawlen;
  if (results->decode_type == UNKNOWN) {
    addJsonString(client, "type", "UNKNOWN");
  }
  else if (results->decode_type == NEC) {
    addJsonString(client, "type", "NEC");
  }
  else if (results->decode_type == SONY) {
    addJsonString(client, "type", "SONY");
  }
  else if (results->decode_type == RC5) {
    addJsonString(client, "type", "RC5");
  }
  else if (results->decode_type == RC6) {
    addJsonString(client, "type", "RC6");
  }
  else if (results->decode_type == LG) {
    addJsonString(client, "type", "LG");
  }
  else if (results->decode_type == JVC) {
    addJsonString(client, "type", "JVC");
  }
  else if (results->decode_type == AIWA_RC_T501) {
    addJsonString(client, "type", "AIWA_RC_T501");
  }
  else if (results->decode_type == WHYNTER) {
    addJsonString(client, "type", "WHYNTER");
  }
  addJsonULong(client, "value", results->value);
  addJsonInt(client, "nbbits", results->bits);
}

void getIRCode(YunClient client) {
    irrecv.resume(); // Receive the next value
    for (unsigned int i = 0 ; i < 10 ; i++) {
      if (irrecv.decode(&results)) {
        dumpToJson(client, &results);
        break;
      }
      delay(1000);
    }
}

void sendIRCommand(YunClient client) {
  String protocol = client.readStringUntil('/');
  String valueHex = client.readStringUntil('/');
  unsigned long value;
  sscanf(valueHex.c_str(), "%lx", &value); // if hex string
  int nbits = client.parseInt();
  // Option frequency param
  int hz = 38;
  if (client.read() == '/')
  {
    hz = client.parseInt();
  }
  addJsonULong(client, "value", value);
  addJsonInt(client, "khz", hz);

  if (protocol == "SAMSUNG") {
    irsend.sendSAMSUNG(value, nbits);
  }
  else if (protocol == "NEC") {
    irsend.sendNEC(value, nbits);
  }
  else if (protocol == "SONY") {
    irsend.sendSony(value, nbits);
  }
  else if (protocol == "RC5") {
    irsend.sendRC5(value, nbits);
  }
  else if (protocol == "RC6") {
    irsend.sendRC6(value, nbits);
  }
  else if (protocol == "DISH") {
    irsend.sendDISH(value, nbits);
  }
  #ifdef IR_PANASONIC
  else if (protocol == "PANASONIC") {
    irsend.sendPanasonic(value, nbits);
  }
  #endif
  #ifdef IR_JVC
  else if (protocol == "JVC") {
    irsend.sendJVC(value, nbits, 1);
  }
  #endif
  #ifdef IR_WHYNTER
  else if (protocol == "WHYNTER") {
    irsend.sendWhynter(value, nbits);
  }
  #endif
  else {
    addJsonString(client, "error", "Unknown protocol");
  }
}

void getTemperature(YunClient client) {
  String tempUnit = client.readStringUntil('\r');

  // print the results to the serial monitor:
  float temperature = dht.readTemperature(tempUnit == "F");
  addJsonFloat(client, "temperature", temperature);
  float humidity = dht.readHumidity();
  addJsonFloat(client, "humidity", humidity);

}

void process(YunClient client) {
  String command = client.readStringUntil('/');
  addJsonString(client, "command", command, true);

  if (command == "getTemp") {
    getTemperature(client);
  }
  else if (command == "sendIRCommand") {
    sendIRCommand(client);
  }
#ifdef IR_RAW 
  else if (command == "sendIRRaw") {
    sendIRRaw(client);
  }
#endif
  else if (command == "getIRCode") {
    getIRCode(client);
  }
  else {
    addJsonString(client, "error", "Unknown Command");
  }
  client.print("}" );
}

void loop() {
  YunClient client = server.accept();

  if (client) {
    process(client);
    client.stop();
  }

  delay(50);
}
