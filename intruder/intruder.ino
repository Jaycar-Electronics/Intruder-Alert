#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

const char *wifi_ssid = "YOUR NETWORK NAME";
const char *wifi_pass = "YOUR NETWORK PASSWORD"; 

//                                                  ie:  /motion/     ie: key/12341346564";
const char *ifttt_url = "https://maker.ifttt.com/trigger/EVENT_NAME/with/key/MAKER_KEY";

const int led_red = D6;
const int led_green = D0;
const int pir_sense = D7;
const int button_sense = D8;

bool armed = false;

BearSSL::WiFiClientSecure client;

void setup()
{
  setupPins();
  Serial.begin(9600);
  WiFi.begin(wifi_ssid, wifi_pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.println("Connected!");
  Serial.printf("Network: %s\n", wifi_ssid);
  Serial.printf("IP: %s\n", WiFi.localIP().toString().c_str());
  client.setInsecure();
}
void loop()
{
  int detectedMotion = digitalRead(pir_sense);

  if (detectedMotion)
  {
    flashLed(led_green, 200);
    flashLed(led_green, 200);

    if (armed)
    {
      sendNotification();
    }
  }

  if (!armed)
  {
    //flash rapidly to show not armed
    digitalWrite(led_red, !digitalRead(led_red));
    delay(1000);
  }

  if (checkButton())
  {
    //we are arming or not arming
    digitalWrite(led_green, LOW);
    digitalWrite(led_red, LOW);
    armed = !armed;
    if (armed)
    {
      delay(1000);
      flashLed(led_red, 200);
      flashLed(led_red, 200);
      flashLed(led_red, 200);
      flashLed(led_red, 3000);
    }
  }
}
// =====================================================
// Private functions below
// =====================================================
void flashLed(int pin, int ms)
{
  digitalWrite(pin, HIGH);
  delay(ms);
  digitalWrite(pin, LOW);
  delay(ms);
}

void setupPins()
{
  pinMode(led_green, OUTPUT);
  pinMode(led_red, OUTPUT);
  pinMode(pir_sense, INPUT_PULLDOWN_16);
  pinMode(button_sense, INPUT_PULLUP);
}

bool checkButton()
{
  if (digitalRead(button_sense) == HIGH)
  {
    delay(1000);

    if (digitalRead(button_sense) == HIGH)
    {
      return true;
    }
  }
  return false;
}

void sendNotification()
{
  // should only be called when wanting to send a notification
  HTTPClient http;
  http.begin(client, ifttt_url);
  int r = http.GET();

  if (r < 0)
  {
    Serial.println(http.errorToString(r));
  }
  else
  {
    http.writeToStream(&Serial);
    Serial.println();
  }
  http.end();
}