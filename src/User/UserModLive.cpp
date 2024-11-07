/*
   @title     StarBase
   @file      UserModLive.h
   @date      20241105
   @repo      https://github.com/ewowi/StarBase, submit changes to this file as PRs to ewowi/StarBase
   @Authors   https://github.com/ewowi/StarBase/commits/main
   @Copyright © 2024 Github StarBase Commit Authors, asmParser © https://github.com/hpwit/ASMParser
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#ifdef STARBASE_USERMOD_LIVE //don't know why to exclude the .cpp as the .h is not included in this case ...

#include "UserModLive.h"

// #include <Arduino.h>

#include "../Sys/SysModPrint.h"
#include "../Sys/SysModUI.h"
#include "../Sys/SysModSystem.h"
#include "../Sys/SysModFiles.h"

// #define __RUN_CORE 0

#include "ESPLiveScript.h" //note: contains declarations AND definitions, therefore can only be included once!

long time1;
long time4;
static float _min = 9999;
static float _max = 0;
static uint32_t _nb_stat = 0;
static float _totfps;
static float fps = 0; //integer?
static unsigned long frameCounter = 0;
static uint8_t loopState = 0; //waiting on Live Script
Parser parser = Parser();

//external function implementation (tbd: move into class)

//tbd: move this to LedModFixture as show is Leds related...
static void show()
{
  frameCounter++;
    
  // SKIPPED: check nargs (must be 3 because arg[0] is self)
  long time2 = ESP.getCycleCount();

  // driver.showPixels(WAIT); // LEDS specific

  //show is done in LedModEffects!

  long time3 = ESP.getCycleCount();
  float k = (float)(time2 - time1) / 240000000;
  fps = 1 / k; //StarBase: class variable so it can be shown in UI!!!
  float k2 = (float)(time3 - time2) / 240000000;
  float fps2 = 1 / k2;
  float k3 = (float)(time2 - time4) / 240000000;
  float fps3 = 1 / k3;
  _nb_stat++;
  if (_min > fps && fps > 10 && _nb_stat > 10)
    _min = fps;
  if (_max < fps && fps < 5000 && _nb_stat > 10)
    _max = fps;
  if (_nb_stat > 10)
    _totfps += fps;
  if (_nb_stat%10000 == 0) //every 10 sec. (temp)
    //Serial.printf("current show fps:%.2f\tglobal fps:%.2f\tfps animation:%.2f\taverage:%.2f\tmin:%.2f\tmax:%.2f\r\n", fps2, fps3, fps, _totfps / (_nb_stat - 10), _min, _max);
    ppf("current show fps:%.2f\tglobal fps:%.2f\tfps animation:%.2f  average:%.2f min:%.2f max:%.2f\r\n",fps2, fps3,  fps, _totfps / (_nb_stat - 10), _min, _max);
  time1 = ESP.getCycleCount();
  time4 = time2;

  // SKIPPED: check that both v1 and v2 are int numbers
  // RETURN_VALUE(VALUE_FROM_INT(0), rindex);
  delay(1); //to feed the watchdog (also if loopState == 0)
  while (loopState != 0) { //not waiting on Live Script
    delay(1); //to feed the watchdog
    // set to 0 by main loop
  }
  //do Live Script cycle
  loopState = 1; //Live Script produced a frame, main loop will deal with it
  // ppf("loopState %d\n", loopState);
}

static void preKill()
{
  ppf("ELS preKill\n");
}
static void postKill()
{
  ppf("ELS postKill\n");
}

static void resetShowStats()
{
    float min = 999;
    float max = 0;
    _nb_stat = 0;
    _totfps = 0;
}

static float _hypot(float x,float y) {return hypot(x,y);}
static float _atan2(float x,float y) { return atan2(x,y);}
static float _sin(float j) {return sin(j);}
static float _cos(float j) {return cos(j);}
static float _triangle(float j) {return 1.0 - fabs(fmod(2 * j, 2.0) - 1.0);}
static float _time(float j) {
      float myVal = sys->now;
      myVal = myVal / 65535 / j;           // PixelBlaze uses 1000/65535 = .015259. 
      myVal = fmod(myVal, 1.0);               // ewowi: with 0.015 as input, you get fmod(millis/1000,1.0), which has a period of 1 second, sounds right
      return myVal;
}

  void UserModLive::setup() {
    SysModule::setup();

    //note: -mtext-section-literals needed in pio.ini, first only for s2, now for all due to something in setup...

    parentVar = ui->initUserMod(parentVar, name, 6310);

    ui->initSelect(parentVar, "script", UINT8_MAX, false, [this](EventArguments) { switch (eventType) {
      case onUI: {
        // variable.setComment("Fixture to display effect on");
        JsonArray options = variable.setOptions();
        options.add("None");
        files->dirToJson(options, true, ".sc"); //only files containing F(ixture), alphabetically

        return true; }
      case onChange: {
        //set script
        uint8_t fileNr = variable.value();

        ppf("%s script.onChange f:%d\n", name, fileNr);

        char fileName[32] = "";

        if (fileNr > 0 && fileNr != UINT8_MAX) { //not None and setup done
          fileNr--;  //-1 as none is no file
          files->seqNrToName(fileName, fileNr, ".sc");
          ppf("script.onChange f:%d n:%s\n", fileNr, fileName);

          uint8_t exeID = liveM->findExecutable(fileName);
          if (exeID == UINT8_MAX) {

            addDefaultExternals();

            //to run blinkSL.sc
            addExternalFun("void", "pinMode", "(int a1, int a2)", (void *)&pinMode);
            addExternalFun("void", "digitalWrite", "(int a1, int a2)", (void *)&digitalWrite);
            addExternalFun("void", "delay", "(int a1)", (void *)&delay);

            exeID = compile(fileName, "void main(){resetStat();setup();while(2>1){loop();sync();}}");
          }

          if (exeID != UINT8_MAX)
            liveM->executeBackgroundTask(exeID);
          else 
            ppf("mapInitAlloc task not created (compilation error?) %s\n", fileName);
        }
        else {
          // kill();
          ppf("script.onChange set to None\n");
        }

        return true; }
      default: return false; 
    }}); //script

    ui->initText(parentVar, "fps1", nullptr, 10, true, [this](EventArguments) { switch (eventType) {
      case onLoop1s:
        mdl->setValue(variable.var, "%.0f /s", fps, 0); //0 is to force format overload used
        return true;
      default: return false; 
    }});
    ui->initText(parentVar, "fps2", nullptr, 10, true, [this](EventArguments) { switch (eventType) {
      case onLoop1s:
        mdl->setValue(variable.var, "%d /s", frameCounter, 0); //0 is to force format overload used
        frameCounter = 0;
        return true;
      default: return false; 
    }});

    JsonObject tableVar = ui->initTable(parentVar, "scripts", nullptr, true);

    ui->initText(tableVar, "name", nullptr, 32, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        variable.var["value"].to<JsonArray>(); web->addResponse(variable.var, "value", variable.value()); // empty the value
        rowNr = 0;
        for (Executable &exec: scriptRuntime._scExecutables) {
          const char *name = exec.name.c_str();
          mdl->setValue(variable.var, JsonString(exec.name.c_str(), JsonString::Copied), rowNr++);
        }
        return true;
      default: return false;
    }});

    ui->initCheckBox(tableVar, "running", UINT8_MAX, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        variable.var["value"].to<JsonArray>(); web->addResponse(variable.var, "value", variable.value()); // empty the value
        rowNr = 0;
        for (Executable &exec: scriptRuntime._scExecutables) {
          mdl->setValue(variable.var, exec.isRunning(), rowNr++);
        }
        return true;
      default: return false;
    }});

    ui->initCheckBox(tableVar, "halted", UINT8_MAX, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        variable.var["value"].to<JsonArray>(); web->addResponse(variable.var, "value", variable.value()); // empty the value
        rowNr = 0;
        for (Executable &exec: scriptRuntime._scExecutables) {
          mdl->setValue(variable.var, exec.isHalted, rowNr++);
        }
        return true;
      default: return false;
    }});

    ui->initCheckBox(tableVar, "exe", UINT8_MAX, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        variable.var["value"].to<JsonArray>(); web->addResponse(variable.var, "value", variable.value()); // empty the value
        rowNr = 0;
        for (Executable &exec: scriptRuntime._scExecutables) {
          mdl->setValue(variable.var, exec.exeExist, rowNr++);
        }
        return true;
      default: return false;
    }});

    ui->initNumber(tableVar, "handle", UINT16_MAX, 0, UINT16_MAX, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        variable.var["value"].to<JsonArray>(); web->addResponse(variable.var, "value", variable.value()); // empty the value
        rowNr = 0;
        for (Executable &exec: scriptRuntime._scExecutables) {
          mdl->setValue(variable.var, exec.__run_handle_index, rowNr++);
        }
        return true;
      default: return false;
    }});

    ui->initText(tableVar, "size", nullptr, 32, true, [this](EventArguments) { switch (eventType) {
      case onSetValue:
        variable.var["value"].to<JsonArray>(); web->addResponse(variable.var, "value", variable.value()); // empty the value
        rowNr = 0;
        for (Executable &exec: scriptRuntime._scExecutables) {
          exe_info exeInfo = scriptRuntime.getExecutableInfo(exec.name);
          char text[30];
          print->fFormat(text, sizeof(text), "%d+%d=%d B", exeInfo.binary_size, exeInfo.data_size, exeInfo.total_size);
          mdl->setValue(variable.var, JsonString(text, JsonString::Copied), rowNr++);
        }
        return true;
      default: return false;
    }});

    ui->initButton(tableVar, "Kill", false, [this](EventArguments) { switch (eventType) {
      case onChange:
        if (rowNr < scriptRuntime._scExecutables.size())
          killAndDelete(scriptRuntime._scExecutables[rowNr].name.c_str());
        else
          ppf("dev try to kill a script which does not exist anymore... (%d)\n", rowNr);
        return true;
      default: return false;
    }});

    runningPrograms.setPrekill(preKill, postKill); //for clockless driver...
    runningPrograms.setFunctionToSync(show);

  } //setup

  void UserModLive::addDefaultExternals() {

    scScript = "";

    //Live Scripts defaults
    addExternalFun("void", "show", "()", (void *)&show); //comment if setup/loop model works
    // addExternalFun("void", "showM", "()", (void *)&UserModLive::showM); // warning: converting from 'void (UserModLive::*)()' to 'void*' [-Wpmf-conversions]
    addExternalFun("void", "resetStat", "()", (void *)&resetShowStats);

    addExternalFun("float", "atan2", "(float a1, float a2)",(void*)_atan2);
    addExternalFun("float", "hypot", "(float a1, float a2)",(void*)_hypot);
    addExternalFun("float", "sin", "(float a1)", (void *)_sin);
    addExternalFun("float", "cos", "(float a1)", (void *)_cos);
    addExternalFun("float", "time", "(float a1)", (void *)_time);
    addExternalFun("float", "triangle", "(float a1)", (void *)_triangle);
    addExternalFun("uint32_t", "millis", "()", (void *)millis);
  }

  void UserModLive::addExternalVal(string result, string name, void * ptr) {
    if (findLink(name, externalType::value) == -1) //allready added earlier
      addExternal(name, externalType::value, ptr);
    scScript += "external " + result + " " + name + ";\n";
  }

  void UserModLive::addExternalFun(string result, string name, string parameters, void * ptr) {
    if (findLink(name, externalType::function) == -1) //allready added earlier
      addExternal(name, externalType::function, ptr);
    scScript += "external " + result + " " + name + parameters + ";\n";
  }

  // void UserModLive::addExternalFun(string name, std::function<void(int)> fun) {
  //   addExternal(name, externalType::function, (void *)&fun)); //unfortionately InstructionFetchError, why does it work in initText etc?
  //   ppf("external %s(int arg1);\n", name.c_str()); //add to string
  // }
  // void UserModLive::addExternalFun(string name, std::function<void(int, int)> fun) {
  //   addExternal(name, externalType::function, (void *)&fun); //unfortionately InstructionFetchError
  //   ppf("external %s(int arg1, int arg2);\n", name.c_str()); //add to string
  // }

  //testing class functions instead of static
  void UserModLive::showM() {
    long time2 = ESP.getCycleCount();
    // driver.showPixels(WAIT);
    frameCounter++;

    float k = (float)(time2 - time1) / 240000000; //always 240MHz?
    fps = 1 / k;
    time1 = ESP.getCycleCount();
  }

  void UserModLive::loop() {
    if (loopState == 2) {// show has been called (in other loop)
      loopState = 0; //waiting on Live Script
      // ppf("loopState %d\n", loopState);
    }
    else if (loopState == 1) {
      loopState = 2; //other loop can call show (or preview)
      // ppf("loopState %d\n", loopState);
    }
  }

  void UserModLive::loop20ms() {
    //workaround temporary disabled (replace by run?)
    // if (strnstr(web->lastFileUpdated, ".sc", sizeof(web->lastFileUpdated)) != nullptr) {
    //   if (strnstr(web->lastFileUpdated, "del:/", sizeof(web->lastFileUpdated)) != nullptr) {
    //     if (strncmp(this->fileName, web->lastFileUpdated+4, sizeof(this->fileName)) == 0) { //+4 remove del:
    //       ppf("loop20ms kill %s\n", web->lastFileUpdated);
    //       kill();
    //     }
    //     //else nothing
    //   }
    //   else {
    //     ppf("loop20ms run %s -> %s\n", this->fileName, web->lastFileUpdated);
    //     run(web->lastFileUpdated);
    //   }
    //   strlcpy(web->lastFileUpdated, "", sizeof(web->lastFileUpdated));
    // }
  }

  void UserModLive::loop1s() {
    for (JsonObject childVar: Variable(mdl->findVar("LiveScripts", "scripts")).children())
      Variable(childVar).triggerEvent(onSetValue); //set the value (WIP)
  }

  void UserModLive::executeTask(uint8_t exeID, const char * function, int val)
  {
    if (val == UINT16_MAX)
      scriptRuntime._scExecutables[exeID].execute(string(function));
    else {
      Arguments arguments;
      arguments.add(val);
      scriptRuntime._scExecutables[exeID].execute(string(function), arguments);
    }
  }

  void UserModLive::executeBackgroundTask(uint8_t exeID, const char * function)
  {
    scriptRuntime._scExecutables[exeID].executeAsTask(string(function));
  }

  uint8_t UserModLive::compile(const char * fileName, const char * post) {
    ppf("live compile n:%s o:%s \n", fileName, this->fileName);
    //if(this->fileName!=NULL)
    killAndDelete(); //doesn't this kill running scripts, e.g. when changing a Live Fixture, a running Live Effect will be killed !
    killAndDelete(fileName); //kill any old script
    
    //this->fileName=(char *)_fileName;
    File f = files->open(fileName, "r");
    if (!f)
    {
      ppf("UserModLive setup script open %s for %s failed\n", fileName, "r");
      return UINT8_MAX;
    }
    else {

      unsigned preScriptNrOfLines = 0;

      for (size_t i = 0; i < scScript.length(); i++)
      {
        if (scScript[i] == '\n')
          preScriptNrOfLines++;
      }
      ppf("preScript of %s has %d lines\n", fileName, preScriptNrOfLines+1); //+1 to subtract the line from parser error line reported

      scScript += string(f.readString().c_str()); // add sc file

      if (post) scScript += post;

      size_t scripLines = 0;
      size_t lastIndex = 0;
      for (size_t i = 0; i < scScript.length(); i++)
      {
        if (scScript[i] == '\n' || i == scScript.length()-1) {
          ppf("%3d %s", scripLines+1, scScript.substr(lastIndex, i-lastIndex+1).c_str());
          scripLines++;
          lastIndex = i + 1;
        }
      }

      ppf("Before parsing of %s\n", fileName);
      ppf("%s:%d f:%d / t:%d (l:%d) B [%d %d]\n", __FUNCTION__, __LINE__, ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap(), esp_get_free_heap_size(), esp_get_free_internal_heap_size());

      Executable executable = parser.parseScript(&scScript);
      executable.name = string(fileName);

      if (executable.exeExist)
      {
        ppf("parsing %s done\n", fileName);
        ppf("%s:%d f:%d / t:%d (l:%d) B [%d %d]\n", __FUNCTION__, __LINE__, ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMaxAllocHeap(), esp_get_free_heap_size(), esp_get_free_internal_heap_size());

        scriptRuntime.addExe(executable);
        ppf("exe created %d\n", scriptRuntime._scExecutables.size());

        return scriptRuntime._scExecutables.size() - 1;
        // ppf("setup done\n");
        // strlcpy(this->fileName, fileName, sizeof(this->fileName));
      }
      else{
        return UINT8_MAX;
      }
      f.close();
    }
  }

  void UserModLive::killAndDelete(const char *name) {
    if (name != nullptr) { 
      scriptRuntime.kill(string(name));
      scriptRuntime.deleteExe(string(name));
    } else {
      scriptRuntime.killAndFreeRunningProgram();
    }
    // fix->liveFixtureID = nullptr; //to be sure! todo: nullify exec pointers fix->liveFixtureID and leds.liveEffectID
  }

  void UserModLive::killAndDelete(uint8_t exeID) {
    if (exeID < scriptRuntime._scExecutables.size())
      killAndDelete(scriptRuntime._scExecutables[exeID].name.c_str());
  }

  uint8_t UserModLive::findExecutable(const char *fileName) {
    uint8_t exeID = 0;
    for (Executable &exec: scriptRuntime._scExecutables) {
      if (exec.name.compare(string(fileName)) == 0)
        return exeID++;
    }
    return UINT8_MAX;
  }

  #endif
