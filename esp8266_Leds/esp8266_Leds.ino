#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* ssid = "Ethernet"; // Students
const char* password = "78949379"; // sedniuchi

const char* mqtt_server = "185.138.177.188";
//const char* mqtt_server = "192.168.3.44";
const unsigned int mqtt_port = 1888;

char buf[10];

WiFiClient espClient;
PubSubClient client(espClient);

const int BLUE_LED = 16;
const int GREEN_LED = 14;
const int YELLOW_LED = 12;
const int RED_LED = 13;



/**************************************************

**************************************************/
void OLEDSetup() {
  // Start the I2C interface
  Wire.begin();

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  // init done
  // Clear the buffer.
  WriteConsole("Welcome!", 2);
}


void WriteConsole(String text) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.println(text);
  display.display();
}

void WriteConsole(String text, int size) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(size);
  display.setTextColor(WHITE, BLACK);
  display.println(text);
  display.display();
}

void setup() {
  Serial.begin(115200);

  delay(500);

  OLEDSetup();

  delay(1000);
  
  WriteConsole("Starting...", 2);

  pinMode(BLUE_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(YELLOW_LED, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  setup_wifi();
  client.setServer(mqtt_server, 1888);
  client.setCallback(callback);
}

void publishLedStates() {
  char states[100];
  snprintf(states, sizeof(states), "B: %s \nG: %s\nY: %s\nR: %s",
    digitalRead(BLUE_LED) ? "ON" : "OFF",
    digitalRead(GREEN_LED) ? "ON" : "OFF",
    digitalRead(YELLOW_LED) ? "ON" : "OFF",
    digitalRead(RED_LED) ? "ON" : "OFF"
   );
  client.publish("ESP8266-CONTROLS", states);
}

void toggleLed(uint8_t pin) {
  digitalWrite(pin, digitalRead(pin) ? LOW : HIGH);
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message;
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print(message);
  Serial.println();


  if (message == "PING") {
    publishLedStates();
    return;
  }

  // Switch on the LED if an BLUE_ON was received as message
  if (message == "BLUE_TOGGLE") {
    toggleLed(BLUE_LED);
  }

  if (message == "GREEN_TOGGLE") {
    toggleLed(GREEN_LED);
  }

  if (message == "YELLOW_TOGGLE") {
    toggleLed(YELLOW_LED);
  }

  if (message == "RED_TOGGLE") {
    toggleLed(RED_LED);
  }

  publishLedStates();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    WriteConsole("Attempting MQTT connection...");
    delay(1000);
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      WriteConsole("Connected!", 2);
      publishLedStates();

      client.subscribe("ESP8266-LEDS");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  WriteConsole("Connecting to "+ String(ssid));
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
