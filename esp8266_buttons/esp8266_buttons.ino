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

const char* ssid = "network name";
const char* password = "network password";

const char* mqtt_server = "mqtt broker ip";
const unsigned int mqtt_port = 1888; // "mqtt broker port"

char buf[10];

WiFiClient espClient;
PubSubClient client(espClient);

struct BUTTON {
  int buttonIO;
  int outputState;
  int buttonState;
  int lastButtonState;
  String sName;
  int buttonNumber;
  int processOut;
};

const int BUTTON1 = 16;
const int BUTTON2 = 14;
const int BUTTON3 = 12;
const int BUTTON4 = 13;

BUTTON BTN1 = { BUTTON1, LOW, 0, HIGH, "B1", 1, 0 };
BUTTON BTN2 = { BUTTON2, LOW, 0, HIGH, "B2", 2, 0 };
BUTTON BTN3 = { BUTTON3, LOW, 0, HIGH, "B3", 3, 0 };
BUTTON BTN4 = { BUTTON4, LOW, 0, HIGH, "B4", 4, 0 };

unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers


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

  delay(200);
  Serial.println("Starting...");

  WriteConsole("Starting...", 2);

  pinMode(BUTTON1, INPUT);
  pinMode(BUTTON2, INPUT);
  pinMode(BUTTON3, INPUT);
  pinMode(BUTTON4, INPUT);
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
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

  WriteConsole(message, 2);
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    WriteConsole("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      WriteConsole("Connected!", 2);
      // Once connected, publish an announcement...
      client.subscribe("ESP8266-CONTROLS");
      client.publish("ESP8266-LEDS", "PING");
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
  checkButtons();
}

void checkButtons() {
  checkButton(&BTN1);
  checkButton(&BTN2);
  checkButton(&BTN3);
  checkButton(&BTN4);
  processOutput(&BTN1);
  processOutput(&BTN2);
  processOutput(&BTN3);
  processOutput(&BTN4);
}

/**************************************************

**************************************************/
void checkButton(BUTTON* b) {
  // read the state of the switch into a local variable:
  int reading = digitalRead(b->buttonIO);

  // check to see if you just pressed the button
  // (i.e. the input went from HIGH to LOW), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != b->lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // if the button state has changed:
    if (reading != b->buttonState) {
      b->buttonState = reading;

      // only toggle the LED if the new button state is LOW
      
      if (b->buttonState == LOW) {
        if (b->sName == "B1") {
          client.publish("ESP8266-LEDS", "BLUE_TOGGLE");
        }
        if (b->sName == "B2") {
          client.publish("ESP8266-LEDS", "GREEN_TOGGLE");
        }
        if (b->sName == "B3") {
          client.publish("ESP8266-LEDS", "YELLOW_TOGGLE");
        }
        if (b->sName == "B4") {
          client.publish("ESP8266-LEDS", "RED_TOGGLE");
        }
      }
    }
  }

  // set the LED:
  //  digitalWrite(ledPin, ledState);

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  b->lastButtonState = reading;
}


/**************************************************

**************************************************/
void processOutput(BUTTON* b) {
  unsigned char ucRes;
  unsigned int i;
  if (b->processOut == 1) {
    b->processOut = 0;
    Serial.println("processing output");

  }
}
