#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <FastLED.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define NUM_LEDS 121
#define DATA_PIN D4

const char* ssid  = "";
const char* password = "";

typedef struct { 
  int r;
  int g;
  int b;
} color_t;

typedef struct {
  int pixel;
  int r;
  int g;
  int b;
} pixel_color_t;

int time_it_is[] = {110, 111, 113, 114, 115};
  
int time_minutes[][12] = {
  { 13,  12,  11,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1},
  {117, 118, 119, 120,  85,  84,  83,  82,  -1,  -1,  -1,  -1},
  {109, 108, 107, 106,  85,  84,  83,  82,  -1,  -1,  -1,  -1},
  { 92,  93,  94,  95,  96,  97,  98,  85,  84,  83,  82,  -1},
  {105, 104, 103, 102, 101, 100,  99,  85,  84,  83,  82,  -1},
  {117, 118, 119, 120,  81,  80,  79,  66,  67,  68,  69,  -1},
  { 66,  67,  68,  69,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1},
  {117, 118, 119, 120,  85,  84,  83,  82,  66,  67,  68,  69},
  {105, 104, 103, 102, 101, 100,  99,  81,  80,  79,  -1,  -1},
  { 92,  93,  94,  95,  96,  97,  98,  81,  80,  79,  -1,  -1},
  {109, 108, 107, 106,  81,  80,  79,  -1,  -1,  -1,  -1,  -1},
  {117, 118, 119, 120,  81,  80,  79,  -1,  -1,  -1,  -1,  -1}
};

int time_hours[][6] = {
  {71, 72, 73, 74, 75, -1},
  {63, 62, 61, 60, -1, -1},
  {65, 64, 63, 62, -1, -1},
  {45, 46, 47, 48, -1, -1},
  {36, 35, 34, 33, -1, -1},
  {51, 52, 53, 54, -1, -1},
  {20, 19, 18, 17, 16, -1},
  {60, 59, 58, 57, 56, 55},
  {23, 24, 25, 26, -1, -1},
  {40, 39, 38, 37, -1, -1},
  {27, 28, 29, 30, -1, -1},
  {43, 42, 41, -1, -1, -1}
};

CRGB leds[NUM_LEDS];
int r = 255;
int g = 255;
int b = 255;
int offset = 0;
int hour = -1;
int minute = -1;
String ip = "";

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", offset, 3600000);

ESP8266WebServer server(80);
String header;

color_t bgColor;
color_t fgColor;
int timezone;

String mode = "time";

color_t hexToRgb(String value) {
  value.replace("#", "");
  int number = (int) strtol( value.c_str(), NULL, 16);
  
  int r = number >> 16;
  int g = number >> 8 & 0xFF;
  int b = number & 0xFF;
  
  color_t rgb;
  rgb.r = r;
  rgb.g = g;
  rgb.b = b;

  return rgb;
}

String rgbToHex(const color_t hex) {
  long hexColor = ((long)hex.r << 16L) | ((long)hex.g << 8L) | (long)hex.b;

  String out = String(hexColor, HEX);

  while(out.length() < 6) {
    out = "0" + out;
  }

  return out;
}

void setOffset(String value) {
  timeClient.setTimeOffset(value.toInt() * 3600);
}

void setTime(int hour, int minute) { 
  minute = (minute - (minute % 5));
  
  if(minute >= 25) {
    hour += 1;
  }

  minute = minute / 5;
  hour = hour % 12;

  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i].setRGB(bgColor.r * 0.2, bgColor.g * 0.2, bgColor.b * 0.2);
  }
  
  for(int i = 0; i < 5; i++) {
    leds[time_it_is[i]].setRGB(fgColor.r, fgColor.g, fgColor.b);
  }

  for(int m = 0; m < 12; m++) {
    if(time_minutes[minute][m] >= 0) {
      leds[time_minutes[minute][m]].setRGB(fgColor.r, fgColor.g, fgColor.b);
    }
  }

  for(int h = 0; h < 6; h++) {
    if(time_hours[hour][h] >= 0) {
        leds[time_hours[hour][h]].setRGB(fgColor.r, fgColor.g, fgColor.b);
    }
  }

  FastLED.show();
}

String getTimeForm() {
  String content = "";

  content += "<div>";
  content += "<label>Foreground color</label>";
  content += "<input name=\"fg\" value=\"#" + rgbToHex(fgColor) + "\" type=\"color\">";
  content += "</div>";
  content += "<div>";
  content += "<label>Background color</label>";
  content += "<input name=\"bg\" value=\"#" + rgbToHex(bgColor) + "\" type=\"color\">";
  content += "</div>";
  content += "<div>";
  content += "<label>Timezone</label>";
  content += "<select name=\"tz\">";
  
  for(int i = -12; i < 13; i++) {

    String label = String(i);
    
    if(i > 0) {
      label = "+" + label; 
    }    

    content += htmlOption(label, String(i), String(timezone));
  }
  
  content += "</select>";
  content += "</div>";

  return content;
}

String htmlOption(String label, String value, String store) {
  String content = "<option value=\"" + value + "\"";
  
  if (store == value) {
    content += " selected=\"selected\"";
  }
  
  content += ">" + label + "</option>";

  return content;
}

void change() {
  bool change = false;

  if(server.hasArg("mode")) {
    mode = server.arg("mode");
  }
  
  if(mode == "time"){
    if(server.hasArg("fg")) {
      fgColor = hexToRgb(server.arg("fg"));
      change = true;
    }
  
    if(server.hasArg("bg")) {
      bgColor = hexToRgb(server.arg("bg"));
      change = true;
    }
    
    if(server.hasArg("tz")) {
      timezone = server.arg("tz").toInt();
      timeClient.setTimeOffset(timezone * 3600);
      change = true;
    }
  }

  if(change == true) {
    show();
  }
}

void handleRootPath() {
  String content = "";

  change();

  content += "<!DOCTYPE html><html>";
  content += "<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  content += "<style>";
  content += "* { box-sizing: border-box; }";
  content += "html, body { font-family: Helvetica; margin: 0; padding: 0; }";
  content += ".form { margin: auto; max-width: 400px; }";
  content += ".form div { margin: 0; padding: 20px 0; width: 100%; font-size: 1.4rem; }";
  content += "label { width: 60%; display: inline-block; margin: 0; vertical-align: middle; }";
  content += "input, select { width: 38%; display: inline-block; margin: 0; border: 1px solid #eee; padding: 0; height: 40px; vertical-align: middle; }";
  content += "button { display: inline-block; width: 100%; font-size: 1.4rem; background-color: green; border: 1px solid #eee; color: #fff; padding-top: 10px; padding-bottom: 10px; }";
  content += "</style>";
  content += "</head>";
  content += "<body>";  
  
  content += "<h1>WordClock WebServer</h1>";
  content += "<form class=\"form\" method=\"post\" action=\"\">";
  content += "<div>";
  content += "<select name=\"mode\">";

  content += htmlOption("Time", "time", mode);

  content += "</select>";
  content += "</div>";
  content += "<div>";
  content += "<button type=\"submit\">Change mode</button>";
  content += "</div>";
  content += "</form>";
  
  content += "<form class=\"form\" method=\"post\" action=\"\">";

  if(mode == "time") {
    content += getTimeForm();
  }
  
  content += "<div>";
  content += "<button type=\"submit\">Save</button>";
  content += "</div>";
  content += "</form>";
  content += "</body></html>";

  server.sendHeader("Location", "http://" + ip);
  server.send(200, "text/html", content);
}

void show() {  
  if(mode == "time") {
    setTime(hour, minute);
  }
}

void setup() {
  Serial.begin(74880);

  bgColor.r = 0;
  bgColor.g = 0;
  bgColor.b = 0;

  fgColor.r = 255;
  fgColor.g = 255;
  fgColor.b = 255;

  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
  FastLED.setBrightness(50);

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  ip = WiFi.localIP() + "";
  
  server.on("/", handleRootPath);
  server.begin();

  timeClient.begin();
  timeClient.update();

  for(int i = 0; i < NUM_LEDS; i++) {
    leds[i].setRGB(0, 0, 0);
  }

  FastLED.show();
  show();
}

void loop() {
  timeClient.update();
  
  int h = timeClient.getHours();
  int m = timeClient.getMinutes();

  if(h != hour || m != minute) {
    if(mode == "time") {
      show();
    }
    hour = h;
    minute = m;
  }
  
  server.handleClient();
}
