#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h> //json parsing util
#include <NTPClient.h>
#include <WiFiUdp.h>

//LED light includes
#include <FastLED.h>
#define DATA_PIN    5
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
#define NUM_LEDS    24
CRGB leds[NUM_LEDS];
#define BRIGHTNESS          96
//end LED light specs

const char* ssid = "Steamed LANs";
const char* password = "albanyexpression";
String node = "willy";

//initalize memory space for JSON
StaticJsonDocument<200> doc;

//Your Domain name with URL path or IP address with path
//http request declares
int HTTP_PORT = 443;
String HTTP_METHOD = "POST";
char HOST_NAME[] = "isellmiataparts.com";
String PATH_NAME = "https://isellmiataparts.com/friendship/friendship.php";
String GET_PATH = "https://isellmiataparts.com/friendship/status.json";
String token = "grandpasconnyandella";

const int buttonPin = 4;
int buttonState = 0;
int colorFromWeb = 0;
int deviceColor = 0;

//add timer interupts for how frequent to call http
static const unsigned long REFRESH_INTERVAL = 500; // ms
static unsigned long lastRefreshTime = 0;

//timer components for sleeping
const unsigned long SLEEP_INTERVAL = 1800; //set this to fall asleep after 30 mins of no input - 1800
int websiteUpdateTime = 0;
int updateTime = 0;
bool sleepStatus = false;

//put ISR in ram
void ICACHE_RAM_ATTR buttonPress();

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void setup() {
  Serial.begin(115200);

  //set button input
  pinMode(buttonPin, INPUT);

  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  websiteUpdateTime = getLastUpdateTime();
  updateTime = getTime(); //get the time at boot, assign it to update

  //setup fast LED

  delay(3000);
  Serial.println("setup LED");
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  //grab color from website, assign it to the device
  colorFromWeb = getCurrentColor();
  deviceColor = colorFromWeb;
  setLEDColor(deviceColor);
  FastLED.show(); 
  Serial.println("Setup Complete");

  //check for button press and go for interupt
  attachInterrupt(digitalPinToInterrupt(buttonPin), buttonPress, RISING);

}

void loop() 
{ 
  sleepTimer();
  //Check WiFi connection status
  delay(1000); //slow things down a bit for rapid button presses
  if(WiFi.status()== WL_CONNECTED)
  { 
    //if the website color is more up to date we should accept that color and set it.
    //if the device is out of sync with the web we should push the color
    //established - the device color is out of sync with the web
    if(colorFromWeb != deviceColor)
    {
      if(websiteUpdateTime > updateTime)
        { 
          setLEDColor(colorFromWeb);
          FastLED.show();
          sleepStatus = false;
        }
      if(updateTime > websiteUpdateTime)
        { 
          //update the website with the new color.
          int updateStatus = postUpdate(token, deviceColor);
          colorFromWeb = deviceColor;
          if (updateStatus>0) 
          {
            Serial.print("HTTP Response code: ");
            Serial.println(updateStatus);
          }
          else 
          {
            Serial.print("Error code: ");
            Serial.println(updateStatus);
          }
        }
    }
    //every 30s check the web and get the color
    colorFromWeb = httpGetColorRateLimited(deviceColor);
  }
  else 
  {
    Serial.println("WiFi Disconnected");
    fill_solid( leds, NUM_LEDS, CRGB::Red);
    FastLED.show();
  }
}

int postUpdate(String token, int color)
{
      WiFiClientSecure client;
      client.setInsecure();
      HTTPClient https;
      https.begin(client, PATH_NAME);
      https.addHeader("Content-Type", "application/json");
      String postData = "{\"token\":\"" + token + "\",\"status\":\"" + color + "\",\"node\":\"" + node + "\"}";
      int httpResponseCode = https.POST(postData);
      // Free resources
      https.end();
      updateTime = getTime(); //reset the update time to now.
      return httpResponseCode;
}

int colorSelector(int currentColor)
{
  int newColor = currentColor;
  if(currentColor <=8) { newColor++; }
  if(currentColor ==8) { newColor=1; }
  return newColor;
}

int getCurrentColor()
{
  int currentColor = 0;
  int lastUpdate = 0;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;
  if (https.begin(client, GET_PATH)) 
  {
    int httpCode = https.GET();
    //retrieve json via https req
    const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
    DynamicJsonDocument doc(capacity);
    // Parse JSON object
    DeserializationError error = deserializeJson(doc, client);
    if (error) 
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      client.stop();
    }
    currentColor = doc["Light Status"];
    websiteUpdateTime = doc["Last Update"];
  }
  return currentColor;
}

int getLastUpdateTime()
{
  int webUpdateTime = 1;
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;
  if (https.begin(client, GET_PATH)) 
  {
    int httpCode = https.GET();
    //retrieve json via https req
    const size_t capacity = JSON_OBJECT_SIZE(3) + JSON_ARRAY_SIZE(2) + 60;
    DynamicJsonDocument doc(capacity);
    // Parse JSON object
    DeserializationError error = deserializeJson(doc, client);
    if (error) 
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      client.stop();
    }
    webUpdateTime = doc["Last Update"];
  }
  if(webUpdateTime > updateTime) { sleepStatus = false; } //wake up the device if the web has an update
  return webUpdateTime;
}

void setLEDColor(int currentColor)
{
  //receive an INT from the json and translate it to a string for CRGB
  if(currentColor == 0) { fill_solid( leds, NUM_LEDS, CRGB::Black); }
  if(currentColor == 1) { fill_solid( leds, NUM_LEDS, CRGB::GhostWhite); }
  if(currentColor == 2) { fill_solid( leds, NUM_LEDS, CRGB::Yellow); }
  if(currentColor == 3) { fill_solid( leds, NUM_LEDS, CRGB::LightGreen); }
  if(currentColor == 4) { fill_solid( leds, NUM_LEDS, CRGB::Blue); }
  if(currentColor == 5) { fill_solid( leds, NUM_LEDS, CRGB::Purple); }
  if(currentColor == 6) { fill_solid( leds, NUM_LEDS, CRGB::PowderBlue); }
  if(currentColor == 7) { fill_solid( leds, NUM_LEDS, CRGB::LightSeaGreen); }
  if(currentColor == 8) { fill_solid( leds, NUM_LEDS, CRGB::Magenta); }
  FastLED.show();  
}

//next two functions are to provide a timer for ring color updates
int httpGetColorRateLimited(int currentDeviceColor)
{
	static const unsigned long REFRESH_INTERVAL = 30000; // ms
	static unsigned long lastRefreshTime = 0;
	//returns the current color if it's not time to update yet
  int colorFromWeb = currentDeviceColor;

	if(millis() - lastRefreshTime >= REFRESH_INTERVAL)
	{
		lastRefreshTime += REFRESH_INTERVAL;
                colorFromWeb = getCurrentColor();
                Serial.println("Grabbing Current Color from Web and Returning");
                Serial.println(colorFromWeb);
	}
  return colorFromWeb;
}

void buttonPress()
{
  if(!sleepStatus) //this way we don't spin the color if the device is currently asleep
  { 
    //button pressed, iterate the color.  Run only one time on this button press.
    deviceColor = colorSelector(deviceColor);
  }

  setLEDColor(deviceColor);
  FastLED.show();
  //button pressed, reset input
  updateTime = getTime();
  sleepStatus = false; //button press, wake up
}

void sleepTimer()
{  
  //if the device has been online more than 30 mins or received an update within 30 mins
  //ALSO if the web hasn't updated in 30 mins
  if(getTime() - updateTime >= SLEEP_INTERVAL && getTime() - websiteUpdateTime >= SLEEP_INTERVAL)
  {
    setLEDColor(0);
    FastLED.show();
    //set sleep status to true so device knows where it is when it comes back online (address color advancement)
    sleepStatus = true;
  } 
}

unsigned long getTime() {
  timeClient.update();
  unsigned long now = timeClient.getEpochTime();
  return now;
}
