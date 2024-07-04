/*
   @title     StarLight
   @file      LedProjections.h
   @date      20240228
   @repo      https://github.com/MoonModules/StarLight
   @Authors   https://github.com/MoonModules/StarLight/commits/main
   @Copyright © 2024 Github StarLight Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

//should not contain variables/bytes to keep mem as small as possible!!

class NoneProjection: public Projection {
  const char * name() {return "None";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💫";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //NoneProjection


class DefaultProjection: public Projection {
  const char * name() {return "Default";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💫";}

}; //DefaultProjection

class MultiplyProjection: public Projection {
  const char * name() {return "Multiply";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💫";}

  public: //to use in Preset1Projection

  void adjustSizeAndPixel(Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    Coord3D proMulti = mdl->getValue("proMulti"); //, rowNr
    //promulti can be 0,0,0 but /= protects from /div0
    sizeAdjusted /= proMulti; sizeAdjusted = sizeAdjusted.maximum(Coord3D{1,1,1}); //size min 1,1,1
    midPosAdjusted /= proMulti;
    pixelAdjusted = pixelAdjusted%sizeAdjusted; // pixel % size
    // ppf("Multiply %d,%d,%d\n", leds->size.x, leds->size.y, leds->size.z);
  }

  void adjustMapped(Coord3D &mapped, Coord3D sizeAdjusted, Coord3D pixelAdjusted, Coord3D midPosAdjusted) {
    // if mirrored find the indexV of the mirrored pixel
    bool mirror = mdl->getValue("mirror");

    if (mirror) {
      Coord3D mirrors = pixelAdjusted / sizeAdjusted; //place the pixel in the right quadrant
      if (mirrors.x %2 != 0) mapped.x = sizeAdjusted.x - 1 - mapped.x;
      if (mirrors.y %2 != 0) mapped.y = sizeAdjusted.y - 1 - mapped.y;
      if (mirrors.z %2 != 0) mapped.z = sizeAdjusted.z - 1 - mapped.z;
    }
  }

  void controls(Leds &leds, JsonObject parentVar) {
    ui->initCoord3D(parentVar, "proMulti", {2,2,1}, 0, 10, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "MultiplyX");
        return true;
      case onChange:
        ui->initCheckBox(var, "mirror", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
          case onChange:
            if (rowNr < leds.fixture->listOfLeds.size()) {
              leds.fixture->listOfLeds[rowNr]->doMap = true;
              leds.fixture->doMap = true;
            }
            return true;
          default: return false;
        }});
        if (rowNr < leds.fixture->listOfLeds.size()) {
          leds.fixture->listOfLeds[rowNr]->doMap = true;
          leds.fixture->doMap = true;
        }
        return true;
      default: return false;
    }});
  }
}; //MultiplyProjection

class TiltPanRollProjection: public Projection {
  const char * name() {return "TiltPanRoll";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💫";}

  public: //to use in Preset1Projection

  void adjustXYZ(Leds &leds, Coord3D &pixel) {
    #ifdef STARBASE_USERMOD_MPU6050
      if (leds.proGyro) {
        pixel = trigoTiltPanRoll.tilt(pixel, leds.size/2, mpu6050->gyro.x);
        pixel = trigoTiltPanRoll.pan(pixel, leds.size/2, mpu6050->gyro.y);
        pixel = trigoTiltPanRoll.roll(pixel, leds.size/2, mpu6050->gyro.z);
      }
      else 
    #endif
    {
      if (leds.proTiltSpeed) pixel = trigoTiltPanRoll.tilt(pixel, leds.size/2, sys->now * 5 / (255 - leds.proTiltSpeed));
      if (leds.proPanSpeed) pixel = trigoTiltPanRoll.pan(pixel, leds.size/2, sys->now * 5 / (255 - leds.proPanSpeed));
      if (leds.proRollSpeed) pixel = trigoTiltPanRoll.roll(pixel, leds.size/2, sys->now * 5 / (255 - leds.proRollSpeed));
      if (leds.fixture->fixSize.z == 1) pixel.z = 0; // 3d effects will be flattened on 2D fixtures
    }
  }

  void controls(Leds &leds, JsonObject parentVar) {
    //tbd: implement variable by reference for rowNrs
    #ifdef STARBASE_USERMOD_MPU6050
      ui->initCheckBox(parentVar, "proGyro", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case onChange:
          if (rowNr < leds.fixture->listOfLeds.size())
            leds.fixture->listOfLeds[rowNr]->proGyro = mdl->getValue(var, rowNr);
          return true;
        default: return false;
      }});
    #endif
    ui->initSlider(parentVar, "proTilt", 128, 0, 254, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        if (rowNr < leds.fixture->listOfLeds.size())
          leds.fixture->listOfLeds[rowNr]->proTiltSpeed = mdl->getValue(var, rowNr);
        return true;
      default: return false;
    }});
    ui->initSlider(parentVar, "proPan", 128, 0, 254, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        if (rowNr < leds.fixture->listOfLeds.size())
          leds.fixture->listOfLeds[rowNr]->proPanSpeed = mdl->getValue(var, rowNr);
        return true;
      default: return false;
    }});
    ui->initSlider(parentVar, "proRoll", 128, 0, 254, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "Roll speed");
        return true;
      case onChange:
        if (rowNr < leds.fixture->listOfLeds.size())
          leds.fixture->listOfLeds[rowNr]->proRollSpeed = mdl->getValue(var, rowNr);
        return true;
      default: return false;
    }});
  }
}; //TiltPanRollProjection

class DistanceFromPointProjection: public Projection {
  const char * name() {return "Distance ⌛";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💫";}

  public: //to use in Preset1Projection

  void controls(Leds &leds, JsonObject parentVar) {
    // ui->initCoord3D(parentVar, "proCenter", {8,8,8}, 0, NUM_LEDS_Max, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
    //   case onUI:
    //     ui->setLabel(var, "Center");
    //     return true;
    //   case onChange:
    //     //initiate projectAndMap
    //     ppf("proCenter %d %d\n", rowNr, leds.fixture->listOfLeds.size());
    //     if (rowNr < leds.fixture->listOfLeds.size()) {
    //       leds.fixture->listOfLeds[rowNr]->doMap = true; //Guru Meditation Error: Core  1 panic'ed (StoreProhibited). Exception was unhandled.
    //       leds.fixture->doMap = true;
    //     }
    //     // ui->setLabel(var, "Size");
    //     return true;
    //   default: return false;
    // }});
  }
}; //DistanceFromPointProjection

class Preset1Projection: public Projection {
  const char * name() {return "Preset1";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💫";}

  void adjustSizeAndPixel(Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    MultiplyProjection mp;
    mp.adjustSizeAndPixel(sizeAdjusted, pixelAdjusted, midPosAdjusted);
  }

  void adjustMapped(Coord3D &mapped, Coord3D sizeAdjusted, Coord3D pixelAdjusted, Coord3D midPosAdjusted) {
    MultiplyProjection mp;
    mp.adjustMapped(mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted);
  }

  void adjustXYZ(Leds &leds, Coord3D &pixel) {
    TiltPanRollProjection tp;
    tp.adjustXYZ(leds, pixel);
  }

  void controls(Leds &leds, JsonObject parentVar) {
    DistanceFromPointProjection dp;
    dp.controls(leds, parentVar);
    MultiplyProjection mp;
    mp.controls(leds, parentVar);
    TiltPanRollProjection tp;
    tp.controls(leds, parentVar);
  }
}; //Preset1Projection

class RandomProjection: public Projection {
  const char * name() {return "Random";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💫";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //RandomProjection

class ReverseProjection: public Projection {
  const char * name() {return "Reverse WIP";}
  const char * tags() {return "💡";}

  public:

  void adjustSizeAndPixel(Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    // UI Variables
    bool reverseX = mdl->getValue("reverse X");
    bool reverseY = mdl->getValue("reverse Y");
    bool reverseZ = mdl->getValue("reverse Z");

    if (reverseX) pixelAdjusted.x = sizeAdjusted.x - pixelAdjusted.x - 1;
    if (reverseY) pixelAdjusted.y = sizeAdjusted.y - pixelAdjusted.y - 1;
    if (reverseZ) pixelAdjusted.z = sizeAdjusted.z - pixelAdjusted.z - 1;
  }

  void controls(Leds &leds, JsonObject parentVar) {
    ui->initCheckBox(parentVar, "reverse X", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
    if (leds.effectDimension >= _2D) {
      ui->initCheckBox(parentVar, "reverse Y", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case onChange:
          leds.fixture->listOfLeds[rowNr]->doMap = true;
          leds.fixture->doMap = true;
          return true;
        default: return false;
      }});
    }
    if (leds.effectDimension == _3D) {
      ui->initCheckBox(parentVar, "reverse Z", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case onChange:
          leds.fixture->listOfLeds[rowNr]->doMap = true;
          leds.fixture->doMap = true;
          return true;
        default: return false;
      }});
    }
  }
}; //ReverseProjection

class MirrorProjection: public Projection {
  const char * name() {return "Mirror WIP";}
  const char * tags() {return "💡";}

  public:

  void adjustSizeAndPixel(Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    // UI Variables
    bool mirrorX = mdl->getValue("mirror X");
    bool mirrorY = mdl->getValue("mirror Y");
    bool mirrorZ = mdl->getValue("mirror Z");

    if (mirrorX) {
      if (pixelAdjusted.x >= sizeAdjusted.x / 2) pixelAdjusted.x = sizeAdjusted.x - 1 - pixelAdjusted.x;
      sizeAdjusted.x = (sizeAdjusted.x + 1) / 2;
    }
    if (mirrorY) {
      if (pixelAdjusted.y >= sizeAdjusted.y / 2) pixelAdjusted.y = sizeAdjusted.y - 1 - pixelAdjusted.y;
      sizeAdjusted.y = (sizeAdjusted.y + 1) / 2;
    }
    if (mirrorZ) {
      if (pixelAdjusted.z >= sizeAdjusted.z / 2) pixelAdjusted.z = sizeAdjusted.z - 1 - pixelAdjusted.z;
      sizeAdjusted.z = (sizeAdjusted.z + 1) / 2;
    }
}

  void controls(Leds &leds, JsonObject parentVar) {
    ui->initCheckBox(parentVar, "mirror X", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
    if (leds.projectionDimension >= _2D) {
      ui->initCheckBox(parentVar, "mirror Y", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case onChange:
          leds.fixture->listOfLeds[rowNr]->doMap = true;
          leds.fixture->doMap = true;
          return true;
        default: return false;
      }});
    }
    if (leds.projectionDimension == _3D) {
      ui->initCheckBox(parentVar, "mirror Z", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case onChange:
          leds.fixture->listOfLeds[rowNr]->doMap = true;
          leds.fixture->doMap = true;
          return true;
        default: return false;
      }});
    }
  }
}; //MirrorProjection

class GroupingProjection: public Projection {
  const char * name() {return "Grouping";}
  const char * tags() {return "💡";}

  public:

  void adjustSizeAndPixel(Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    // UI Variables
    Coord3D grouping = mdl->getValue("Grouping");
    grouping = grouping.maximum(Coord3D{1, 1, 1}); // {1, 1, 1} is the minimum value
    if (grouping == Coord3D{1, 1, 1}) return;

    pixelAdjusted.x = pixelAdjusted.x / grouping.x;
    pixelAdjusted.y = pixelAdjusted.y / grouping.y;
    pixelAdjusted.z = pixelAdjusted.z / grouping.z;

    sizeAdjusted.x = (sizeAdjusted.x + grouping.x - 1) / grouping.x; // round up
    sizeAdjusted.y = (sizeAdjusted.y + grouping.y - 1) / grouping.y;
    sizeAdjusted.z = (sizeAdjusted.z + grouping.z - 1) / grouping.z;
  }

  void controls(Leds &leds, JsonObject parentVar) {
    ui->initCoord3D(parentVar, "Grouping", {1,1,1}, 0, 100, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
  }
}; //GroupingProjection

class SpacingProjection: public Projection {
  const char * name() {return "Spacing WIP";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💡";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //SpacingProjection

class TransposeProjection: public Projection {
  const char * name() {return "Transpose";}
  const char * tags() {return "💡";}

  public:

  void adjustSizeAndPixel(Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    // UI Variables
    bool transposeXY = mdl->getValue("transpose XY");
    bool transposeXZ = mdl->getValue("transpose XZ");
    bool transposeYZ = mdl->getValue("transpose YZ");

    if (transposeXY) { int temp = pixelAdjusted.x; pixelAdjusted.x = pixelAdjusted.y; pixelAdjusted.y = temp; }
    if (transposeXZ) { int temp = pixelAdjusted.x; pixelAdjusted.x = pixelAdjusted.z; pixelAdjusted.z = temp; }
    if (transposeYZ) { int temp = pixelAdjusted.y; pixelAdjusted.y = pixelAdjusted.z; pixelAdjusted.z = temp; }
  }

  void controls(Leds &leds, JsonObject parentVar) {
    ui->initCheckBox(parentVar, "transpose XY", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
    if (leds.effectDimension == _3D) {
      ui->initCheckBox(parentVar, "transpose XZ", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case onChange:
          leds.fixture->listOfLeds[rowNr]->doMap = true;
          leds.fixture->doMap = true;
          return true;
        default: return false;
      }});
      ui->initCheckBox(parentVar, "transpose YZ", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case onChange:
          leds.fixture->listOfLeds[rowNr]->doMap = true;
          leds.fixture->doMap = true;
          return true;
        default: return false;
      }});
    }
  }
}; //TransposeProjection

class KaleidoscopeProjection: public Projection {
  const char * name() {return "Kaleidoscope WIP";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💫";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //KaleidoscopeProjection

// Pinwheel WIP
// Currently 1D to 2D/3D May be possible to make 2D to 2D/3D
class PinwheelProjection: public Projection {
  const char * name() {return "Pinwheel WIP";}
  const char * tags() {return "💡";}

  void adjustSizeAndPixel(Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    sizeAdjusted.x = mdl->getValue("Petals");
    sizeAdjusted.y = 1;
    sizeAdjusted.z = 1;
  }

  void adjustMapped(Coord3D &mapped, Coord3D sizeAdjusted, Coord3D pixelAdjusted, Coord3D midPosAdjusted) {
    // get mdl variables
    // Coord3D center = mdl->getValue("Center");
    Coord3D center = midPosAdjusted; // lazy testing
    int swirlVal   = mdl->getValue("swirlVal");
    bool reverse   = mdl->getValue("reverse");
    int angleRange = max(1, int(mdl->getValue("angleRange")));
    int petals     = max(1, sizeAdjusted.x); // sizeAdjusted.x == mdl->getValue("Petals");
    int zTwist     = mdl->getValue("zTwist");

    if (swirlVal < 0) { // swap x and y instead of negative swirlVal use transpose later
      swirlVal = abs(swirlVal);
      int temp = pixelAdjusted.x;
      pixelAdjusted.x = pixelAdjusted.y;
      pixelAdjusted.y = temp;
    }

    // 3D distance option 
    // swirlFactor = sqrt(sq(pixelAdjusted.x - midPosAdjusted.x) + sq(pixelAdjusted.y - midPosAdjusted.y) + sq(pixelAdjusted.z - midPosAdjusted.z)) * swirlVal;
    float swirlFactor = hypot(pixelAdjusted.x - midPosAdjusted.x, pixelAdjusted.y - midPosAdjusted.y) * swirlVal;
    float radians     = atan2(pixelAdjusted.y - midPosAdjusted.y, pixelAdjusted.x - midPosAdjusted.x);
    float angle       = degrees(radians);

    int value = round(angle) + 180 + swirlFactor + (zTwist * pixelAdjusted.z);
    value %= angleRange;
    
    if (reverse) value = angleRange - value - 1; // Reverse

    value = round(value / (angleRange / float(petals))); // Petals
    value %= petals;

    mapped.x = value;
    mapped.y = 1;
    mapped.z = 1;

    // if (pixelAdjusted.x == 0 && pixelAdjusted.y == 0 && pixelAdjusted.z == 0) ppf("Pinwheel Center: (%d, %d) SwirlVal: %f angleRange: %d\n", midPosAdjusted.x, midPosAdjusted.y, swirlVal, angleRange);
    // if (pixelAdjusted.x == 0) { //print first column
    //   ppf(" pixelAdjusted: %d,%d,%d sizeAdjusted: %d,%d,%d", pixelAdjusted.x, pixelAdjusted.y, pixelAdjusted.z, sizeAdjusted.x, sizeAdjusted.y, sizeAdjusted.z);
    //   ppf(" mapped %d,%d,%d", mapped.x, mapped.y, mapped.z);
    //   ppf(" angle %f angleRange %d value %d\n", angle, angleRange, value);
    // }
  }
  void controls(Leds &leds, JsonObject parentVar) {
    // Slider range supposed to be 0 - 255, but -15 to 15 seems to work fine?
    ui->initSlider(parentVar, "swirlVal", 0, -15, 15, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "Swirl"); // Does nothing?
        return true;
      case onChange:
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
    // Testing zTwist range -42 to 42 arbitrary values for testing. Hide later if not 3D fixture
    // Not sure if I like this, need to try on physical setup later.
    ui->initSlider(parentVar, "zTwist", 0, -42, 42, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "zTwist");
        return true;
      case onChange:
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
    // Angle range 0 - angleRange. For testing purposes
    ui->initNumber(parentVar, "angleRange", 360, 1, 720, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
    // Naming petals, arms, blades, rays? Controls virtual strip length.
    ui->initNumber(parentVar, "Petals", 360, 1, 360, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});

    // use reverse class when implemented
    ui->initCheckBox(parentVar, "reverse", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
    // UI add multiply / mirror / tilt pan roll

  }
}; //PinwheelProjection

class TestProjection: public Projection {
  const char * name() {return "Test";}
  //uint8_t dim() {return _1D;} // every projection should work for all D
  const char * tags() {return "💡";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //TestProjection