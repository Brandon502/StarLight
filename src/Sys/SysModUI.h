/*
   @title     StarMod
   @file      SysModUI.h
   @date      20230730
   @repo      https://github.com/ewoudwijma/StarMod
   @Authors   https://github.com/ewoudwijma/StarMod/commits/main
   @Copyright (c) 2023 Github StarMod Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
*/

#pragma once
#include <vector>
#include "ArduinoJson.h"
#include "Module.h"

typedef void(*UCFun)(JsonObject);
typedef void(*LoopFun)(JsonObject, uint8_t*);

struct VarLoop {
  JsonObject var;
  LoopFun loopFun;
  size_t bufSize = 100;
  uint16_t interval = 160; //160ms default
  unsigned long lastMillis = 0;
  unsigned long counter = 0;
  unsigned long prevCounter = 0;
};

class SysModUI:public Module {

public:
  SysModUI();

  //serve index.htm
  void setup();

  void loop();

  JsonObject initModule(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = true, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "module", value, readOnly, uiFun, chFun, loopFun);
  }

  JsonObject initTable(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = true, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "table", value, readOnly, uiFun, chFun, loopFun);
  }

  JsonObject initText(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = true, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "text", value, readOnly, uiFun, chFun, loopFun);
  }

  JsonObject initPassword(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = true, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "password", value, readOnly, uiFun, chFun, loopFun);
  }

  JsonObject initNumber(JsonObject parent, const char * id, int value, bool readOnly = true, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "number", value, readOnly, uiFun, chFun, loopFun);
  }

  //init a range slider, range between 0 and 255!
  JsonObject initSlider(JsonObject parent, const char * id, int value, bool readOnly = true, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "range", value, readOnly, uiFun, chFun, loopFun);
  }

  JsonObject initCanvas(JsonObject parent, const char * id, int value, bool readOnly = true, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<int>(parent, id, "canvas", value, readOnly, uiFun, chFun, loopFun);
  }

  JsonObject initCheckBox(JsonObject parent, const char * id, bool value, bool readOnly = true, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<bool>(parent, id, "checkbox", value, readOnly, uiFun, chFun, loopFun);
  }

  JsonObject initButton(JsonObject parent, const char * id, const char * value = nullptr, bool readOnly = true, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "button", value, readOnly, uiFun, chFun, loopFun);
  }

  JsonObject initSelect(JsonObject parent, const char * id, uint8_t value, bool readOnly = true, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<uint8_t>(parent, id, "select", value, readOnly, uiFun, chFun, loopFun);
  }

  JsonObject initTextArea(JsonObject parent, const char * id, const char * value, bool readOnly = true, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    return initVarAndUpdate<const char *>(parent, id, "textarea", value, readOnly, uiFun, chFun, loopFun);
  }

  template <typename Type>
  JsonObject initVarAndUpdate(JsonObject parent, const char * id, const char * type, Type value, bool readOnly = true, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr) {
    JsonObject var = initVar(parent, id, type, readOnly, uiFun, chFun, loopFun);
    bool isPointer = std::is_pointer<Type>::value;
    //set a default if not a value yet
    if (var["value"].isNull() && (!isPointer || value)) var["value"] = value; //if value is a pointer, it needs to have a value
    //tbd check if value in case of constchar* needs to be copied using (char *)...
    //no call of fun for buttons otherwise all buttons will be fired including restart delete model.json and all that jazz!!! 
    if (strcmp(type,"button")!=0 && chFun && (!isPointer || value)) chFun(var); //!isPointer because 0 is also a value then
    return var;
  }

  JsonObject initVar(JsonObject parent, const char * id, const char * type, bool readOnly = true, UCFun uiFun = nullptr, UCFun chFun = nullptr, LoopFun loopFun = nullptr);

  //run the change function and send response to all? websocket clients
  static void setChFunAndWs(JsonObject var, const char * value = nullptr);

  //interpret json and run commands or set values
  static const char * processJson(JsonVariant &json); //static for setupJsonHandlers

  //called to rebuild selects and tables (tbd: also label and comments is done again, that is not needed)
  void processUiFun(const char * id);

private:
  static bool varLoopsChanged;

  static int varCounter; //not static crashes ??? (not called async...?)

  static std::vector<UCFun> ucFunctions;
  static std::vector<VarLoop> loopFunctions;

};

static SysModUI *ui;