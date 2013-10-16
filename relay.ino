#include "SPI.h"
#include "Ethernet.h"
#include "WebServer.h"

static uint8_t mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
static uint8_t ip[] = { 192, 168, 10, 210 };

#define RELAY1  9
#define RELAY2  8

byte relay_pins[] = {RELAY1, RELAY2, NULL};

#define PREFIX ""
#define NAMELEN 32
#define VALUELEN 32

WebServer webserver(PREFIX, 80);

void init_relays()
{
  byte counter = 0;
  
  while(NULL != relay_pins[counter])
  {
    pinMode(relay_pins[counter], OUTPUT);           
    counter++;
  }
}

void concat(char *destination, char const source[])
{
    destination += strlen(destination);
    
    while (*source)
        *(destination++) = *source++;
    
    *destination = '\0';
}

void print_relays_status(char *content)
{
  byte counter = 0;
  char buffer[128];
  strcpy(content, "[");

  while(NULL != relay_pins[counter])
  {
    sprintf(buffer, "{\"state\":\"%d\", \"pin\":\"%d\"}%s", digitalRead(relay_pins[counter]), relay_pins[counter], NULL == relay_pins[counter + 1] ? "" : ",");
    concat(content, buffer);
    counter++;
  }
  strcat(content, "]");
}

int toggle_relay_status(int pin)
{
  int state = 0;
  if (0 == digitalRead(pin))
  {
    digitalWrite(pin, state = HIGH);
  } else {
    digitalWrite(pin, state = LOW);
  }
  
  return state;
}

void statusCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
  server.httpSuccess("application/json","Cache-Control: no-cache, no-store, must-revalidate\nPragma: no-cache\nExpires: 0\n");
  char content[250] = "";

  if (type != WebServer::HEAD)
  {
    print_relays_status(content);
    server.print(content);
  }
}

void toggleCmd(WebServer &server, WebServer::ConnectionType type, char *url_tail, bool tail_complete)
{
  URLPARAM_RESULT rc;
  char name[NAMELEN], value[VALUELEN];

  if (type == WebServer::GET)
  {
    while (strlen(url_tail))
    {
      rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
      if (rc != URLPARAM_EOS && strcmp(name, "pin") == 0)
      {
        unsigned int pin_number = atoi(value);
        if (0 != pin_number) {
        toggle_relay_status(pin_number);
        statusCmd(server, type, url_tail, tail_complete);
        break;
        }
      }
    } 
  }
}

void indexCmd(WebServer &server, WebServer::ConnectionType type, char *, bool)
{
  server.httpSuccess();

  if (type != WebServer::HEAD)
  {
    P(content) = 
"<!doctype html>"
"<html>"
"<script type=\"text/javascript\">function microAjax(B,A){this.bindFunction=function(E,D){return function(){return E.apply(D,[D])}};this.stateChange=function(D){if(this.request.readyState==4){this.callbackFunction(this.request.responseText)}};this.getRequest=function(){if(window.ActiveXObject){return new ActiveXObject(\"Microsoft.XMLHTTP\")}else{if(window.XMLHttpRequest){return new XMLHttpRequest()}}return false};this.postBody=(arguments[2]||\"\");this.callbackFunction=A;this.url=B;this.request=this.getRequest();if(this.request){var C=this.request;C.onreadystatechange=this.bindFunction(this.stateChange,this);if(this.postBody!==\"\"){C.open(\"POST\",B,true);C.setRequestHeader(\"X-Requested-With\",\"XMLHttpRequest\");C.setRequestHeader(\"Content-type\",\"application/x-www-form-urlencoded\");C.setRequestHeader(\"Connection\",\"close\")}else{C.open(\"GET\",B,true)}C.send(this.postBody)}};</script>"
"<link rel=\"stylesheet\" href=\"http://yui.yahooapis.com/pure/0.3.0/pure-min.css\">"
"<ul id=\"relay-list\"><ul>"
"<script type=\"text/javascript\">"
"function setCaption(item, index) {"
"  var element = document.getElementById('relay-' + item.pin);"
"  element.innerHTML = 'Relay ' + index + ' : ' + (item.state == 0 ? 'off' : 'on');"
"  element.className = 'button ' + (item.state == 0 ? 'off' : 'on');"
"}"

"function toggleRelay(subject) {"
"  microAjax('/toggle.html?pin=' + subject.pin, function (string) {"
"    if ((result = eval(string)) instanceof Array) {"
"      result.forEach(function(item, index) {"
"        if (subject.pin == item.pin) {"
"          setCaption(item, index);"
"        }"
"      });"
"    }"
"  });"
"}"

"function displayRelayList() {"
"  microAjax('/status.html', function (string) {"
"    var container = document.getElementById('relay-list');"
"    if ((result = eval(string)) instanceof Array) {"
"      result.forEach(function(item, index) {"
"        var button = document.createElement('li');"
"        container.appendChild(button);"
"        button.id = 'relay-' + item.pin;"
"        button.onclick = function() {toggleRelay(item)};"
"        setCaption(item, index);"
"      })"
"    }"
"  });"
"}"

"displayRelayList();"
"</script>"
"</html>";
    server.printP(content);
  }
}

void setup()
{
  init_relays();

  if (0 == Ethernet.begin(mac)) {
    Ethernet.begin(mac, ip);
  }

  webserver.setDefaultCommand(&indexCmd);
  webserver.setFailureCommand(&indexCmd);

  webserver.addCommand("status.html", &statusCmd);
  webserver.addCommand("toggle.html", &toggleCmd);
  
  webserver.begin();
}

void loop()
{
  char buff[64];
  int len = 64;

  webserver.processConnection(buff, &len);
}
