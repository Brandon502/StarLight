/*
   @title     StarBase
   @file      SysModule.cpp
   @date      20241105
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "SysModule.h"

#include "Sys/SysModUI.h"

void SysModule::addPresets(JsonObject parentVar) {
  Variable parentVariable = Variable(parentVar);
  Variable currentVar = ui->initSelect(parentVariable, "preset", (uint8_t)0);

  currentVar.subscribe(onUI, [this](Variable variable, uint8_t rowNr, uint8_t eventType) {
    ppf("publish preset.onUI %s.%s [%d]\n", variable.pid(), variable.id(), rowNr);
    JsonArray options = variable.setOptions();

    JsonArray presets = mdl->presets->as<JsonObject>()[name]; //presets of this module

    for (int i=0; i<16; i++) {
      StarString buf;
      if (i >= presets.size() || presets[i].isNull() || presets[i]["name"].isNull()) {
        buf.format("%02d: Empty", i);
      } else {
        // buf.format("%02d: %s", i, presets[i]["name"].as<const char *>()); //this causes crashes in asyncwebserver !!!
        buf = presets[i]["name"].as<const char *>();
        ppf("preset.onUI %02d %s (%d)\n", i, buf.getString(), buf.length());
      }
      options.add(buf.getString()); //copy!
    }

    // print->printJson("  options", options);

  });

  currentVar.subscribe(onChange, [this](Variable variable, uint8_t rowNr, uint8_t eventType) {

    //on Change: save the old value, retrieve the new value and set all values
    //load the complete presets.json, make the changes, save it (using save button...

    uint8_t value = variable.var["value"];
    ppf("publish preset.onchange %s.%s [%d] %d %s\n", variable.pid(), variable.id(), rowNr, value, variable.valueString().c_str());

    JsonObject allPresets = mdl->presets->as<JsonObject>();

    if (!allPresets.isNull()) {

      JsonArray presets = allPresets[name];
      if (!presets.isNull()) {

        // JsonVariant m = mdl->findVar("m", name); //find this module (effects)

        if (presets[value].is<JsonObject>()) { //always?
          for (JsonPair pidPair: presets[value].as<JsonObject>()) {
            for (JsonPair idPair: pidPair.value().as<JsonObject>()) {
              ppf("load %s.%s: %s\n", pidPair.key().c_str(), idPair.key().c_str(), idPair.value().as<String>().c_str());
              if (pidPair.key() != "name") {
                JsonVariant jv = idPair.value();
                if (jv.is<JsonArray>()) {
                  uint8_t rowNr = 0;
                  for (JsonVariant element: jv.as<JsonArray>()) {
                    mdl->setValue(pidPair.key().c_str(), idPair.key().c_str(), element, rowNr++);
                  }
                }
                else
                  mdl->setValue(pidPair.key().c_str(), idPair.key().c_str(), jv);
              }
            }
          }
        }
      }
    }

  });

  currentVar = ui->initButton(parentVariable, "assignPreset", false);

  currentVar.subscribe(onChange, [this, &parentVariable](Variable variable, uint8_t rowNr, uint8_t eventType) {
    ppf("assignPreset.onUI \n");
    //save this to the first free slot
    //give that a name
    //select that one

    Variable presetVariable = Variable(name, "preset");

    uint8_t value = presetVariable.var["value"];

    JsonObject allPresets = mdl->presets->as<JsonObject>();
    if (allPresets.isNull()) allPresets = mdl->presets->to<JsonObject>(); //create

    JsonArray presets = allPresets[name];
    if (presets.isNull()) presets = allPresets[name].to<JsonArray>();

    JsonObject m = mdl->findVar("m", name); //find this module (effects)

    // print->printJson("m", m); ppf("\n");
    // print->printJson("pv", parentVariable.var); ppf("\n");

    StarString result;

    if (!m["n"].isNull()) {
      // print->printJson("walk for", m["n"]); ppf("\n");
      presets[value].to<JsonObject>();//empty
      mdl->walkThroughModel([presets, value, &result](JsonObject parentVar, JsonObject var) {
        Variable variable = Variable(var);
        if (!variable.readOnly() &&  strncmp(variable.id(), "preset", 32) != 0 ) { //exclude preset
          ppf("save %s.%s: %s\n", variable.pid(), variable.id(), variable.valueString().c_str());
          presets[value][variable.pid()][variable.id()] = var["value"];

          if (var["type"] == "text") {
            result += variable.valueString().c_str(); //concat
            result.catSep(", ");
          } else if (var["type"] == "select") {
            char option[32];
            if (variable.valIsArray())
              variable.getOption(option, var["value"][0]); //only one for now
            else
              variable.getOption(option, var["value"]);
            ppf("add option %s.%s[0] %s\n", variable.pid(), variable.id(), option);
            result += option; //concat
            result.catSep(", ");
          }
        }
        return JsonObject(); //don't stop
      }, m); //walk using m["n"]
    }

    if (result.length() == 0) {
      result.format("Preset %d", value);
    }
    presets[value]["name"] = result.getString();

    presetVariable.publish(onUI); //reload ui for new list of values

  });

  currentVar = ui->initButton(parentVariable, "clearPreset", false); //clear preset

  currentVar.subscribe(onChange, [this](Variable variable, uint8_t rowNr, uint8_t eventType) {
    ppf("delete.onUI \n");
    //free this slot
    //remove the name

    Variable presetVariable = Variable(name, "preset");

    uint8_t value = presetVariable.var["value"];

    JsonObject allPresets = mdl->presets->as<JsonObject>();

    if (!allPresets.isNull()) {

      JsonArray presets = allPresets[name];
      if (!presets.isNull()) {

        if (value < presets.size()) {

          presets[value] = (char*)0; // set element in valArray to 0 (is content deleted from memory?)

          presetVariable.publish(onUI); //reload ui for new list of values
        }
      }
    }

  });

  ui->initVCR(parentVariable, "vcr", false);

}