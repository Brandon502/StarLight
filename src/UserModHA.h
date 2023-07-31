#include <WiFi.h>
#include <ArduinoHA.h>

#define BROKER_ADDR     IPAddress(192,168,178,42)

class UserModHA:public Module {

public:

  UserModHA() :Module("Home Assistant support") {
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);

    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  };
  static void onStateCommand(bool state, HALight* sender) {
      Serial.print("State: ");
      Serial.println(state);

      sender->setState(state); // report state back to the Home Assistant
  }

  static void onBrightnessCommand(uint8_t brightness, HALight* sender) {
      Serial.print("Brightness: ");
      Serial.println(brightness);

      sender->setBrightness(brightness); // report brightness back to the Home Assistant
  }

  static void onRGBColorCommand(HALight::RGBColor color, HALight* sender) {
      Serial.print("Red: ");
      Serial.println(color.red);
      Serial.print("Green: ");
      Serial.println(color.green);
      Serial.print("Blue: ");
      Serial.println(color.blue);

      sender->setRGBColor(color); // report color back to the Home Assistant
  }

  void connected() {
    print->print("%s %s\n", __PRETTY_FUNCTION__, name);
    // set device's details (optional)
    device.setName("Playground");
    device.setSoftwareVersion("0.0.1");


    // configure light (optional)
    light->setName("LEDs");

    // Optionally you can set retain flag for the HA commands
    // light.setRetain(true);

    // Maximum brightness level can be changed as follows:
    // light.setBrightnessScale(50);

    // Optionally you can enable optimistic mode for the HALight.
    // In this mode you won't need to report state back to the HA when commands are executed.
    // light.setOptimistic(true);

    // handle light states
    light->onStateCommand(onStateCommand);
    light->onBrightnessCommand(onBrightnessCommand); // optional
    light->onRGBColorCommand(onRGBColorCommand); // optional

    mqtt->begin(BROKER_ADDR);
    print->print("%s %s %s\n", __PRETTY_FUNCTION__, name, success?"success":"failed");
  }

  void loop(){
    // Module::loop();
    mqtt->loop();
  }

  private:
    WiFiClient client;
    HADevice device;
    HAMqtt* mqtt = new HAMqtt(client, device);
    HALight* light = new HALight("playgroud", HALight::BrightnessFeature | HALight::RGBFeature);
};

static UserModHA *hamod;