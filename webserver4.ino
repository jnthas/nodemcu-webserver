#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h> 


ESP8266WebServer server(80);

struct Device
{
    String name;
    uint8_t port;
    bool state;
};

Device devices[] = {
  { .name = "Fan", .port = D1, .state = false },
  { .name = "Device 2", .port = D2, .state = false },
  { .name = "Device 3", .port = D3, .state = false },
  { .name = "Device 4", .port = D4, .state = false }
};


String MAIN_PAGE = "<!doctype html>"\
                    "<html lang='en'>"\
                    "<head>"\
                        "<meta charset='utf-8'>"\
                        "<title>Device Manager</title>"\
                        "<base href='/'>"\
                        "<meta name='viewport' content='width=device-width, initial-scale=1'>"\
                        "<script>"\
                            "function onToggleChange(port, toggleElem) {"\
                                "const isChecked = toggleElem.checked ? 'on' : 'off';"\
                                "const request = new XMLHttpRequest();"\
                                "const url = `/turn/${isChecked}?port=${port}`;"\
                                "request.open('POST', url);"\
                                "request.send();"\
                            "}"\
                        "</script>"\
                        "<style>"\
                            "*{margin:0;box-sizing:border-box}body,html{background-color:#e5e5e5;width:100%;height:100%;font-family:sans-serif;font-size:16px}main{min-height:100%;display:flex;flex-direction:column;align-items:center;justify-content:center}section{display:flex;flex-direction:column;justify-content:space-evenly;align-items:center;text-align:center;height:100%}section div{width:100%;margin:1rem 0;display:flex;justify-content:space-evenly;align-items:center}section div>span{display:block;text-align:left;font-weight:700;width:16rem}.switch{position:relative;display:inline-block;width:60px;height:34px}.switch input{opacity:0;width:0;height:0}.slider{position:absolute;cursor:pointer;top:0;left:0;right:0;bottom:0;background-color:#ccc;-webkit-transition:.4s;transition:.4s}.slider:before{position:absolute;content:\"\";height:26px;width:26px;left:4px;bottom:4px;background-color:#fff;-webkit-transition:.4s;transition:.4s}input:checked+.slider{background-color:#2196f3}input:focus+.slider{box-shadow:0 0 1px #2196f3}input:checked+.slider:before{-webkit-transform:translateX(26px);-ms-transform:translateX(26px);transform:translateX(26px)}.slider.round{border-radius:34px}.slider.round:before{border-radius:50%}"\
                        "</style>"\
                    "</head>"\
                    "<body>"\
                        "<main>"\
                            "<h1>Devices</h1>"\
                            "<section>"\
                            "{DEVICE_ELEM}"\
                            "</section>"\
                        "</main>"\
                    "</body>"\
                    "</html>";


String DEVICE_ELEM = "<div><span>{DEVICE_NAME}</span><label class='switch'><input type='checkbox' checked onchange=\"onToggleChange('{DEV_PORT}', this)\"><span class='slider round'></span></label></div>";


void showAllDevices() {

  String message = "Devices:\n\n";
  for (uint8_t i = 0; i < sizeof(devices)/16; i++) {
    message += " " + devices[i].name + " Port: " + devices[i].port + " State: " + devices[i].state + "\n";
  }
  server.send(200, "text/plain", message);
  
}

void controlPort(int port, bool state) {
  digitalWrite(port, state);
}


void handleRoot() {
  String all_dev_elems = "";
  for (uint8_t i = 0; i < sizeof(devices)/16; i++) {
    String dev = DEVICE_ELEM;
    String state = getState(devices[i].state);
    String stateLabel = getState(!devices[i].state);
    dev.replace("{DEVICE_NAME}", devices[i].name);
    dev.replace("{DEV_STATE}", state);
    dev.replace("{DEV_PORT}", String(devices[i].port, DEC));
    stateLabel.toUpperCase();
    dev.replace("{DEV_STATE_LABEL}", stateLabel);
    
    all_dev_elems += dev;
  }
  String page = MAIN_PAGE;
  page.replace("{DEVICE_ELEM}", all_dev_elems);
  server.send(200, "text/html", page);
  
}

String getState(bool state) {
  if (state) {
    return String("on");
  }

  return String("off");
}


void handleTurnDevices(bool state) {
  
  if (server.hasArg("port")) {
    uint8_t index = findDeviceIndexByPort(server.arg("port").toInt());
    devices[index].state = !devices[index].state;
    controlPort(devices[index].port, devices[index].state);
    handleRoot();
  } else {
    server.send(200, "text/plain", "no port found");
  }
  
}


uint8_t findDeviceIndexByPort(int port) {
  for (uint8_t i = 0; i < sizeof(devices)/16; i++) {
    if (devices[i].port == port) {
      return i;
    }
  }
}


void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void initializePorts() {
  for (uint8_t i = 0; i < sizeof(devices)/16; i++) {
    pinMode(devices[i].port, OUTPUT);
    digitalWrite(devices[i].port, devices[i].state);
  }
}


void setup(void) {
  
  initializePorts();
  
  Serial.begin(115200);

  WiFiManager wifiManager;
  wifiManager.resetSettings();
  wifiManager.autoConnect("AutoConnectAP");

  server.on("/", handleRoot);

  server.on("/turn/on", []() {
    handleTurnDevices(true);
  });

  server.on("/turn/off", []() {
    handleTurnDevices(false);
  });

  server.on("/devices", showAllDevices);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
 
}
