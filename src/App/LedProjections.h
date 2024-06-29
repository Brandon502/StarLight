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

// PROJECTIONS
class NoneProjection: public Projection { // Coded in LedFixture.cpp?
  const char * name() {return "None";}
  const char * tags() {return "💫";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //NoneProjection

class DefaultProjection: public Projection {
  const char * name() {return "Default";}
  const char * tags() {return "💫";}

  public:

  void adjustSizeAndPixel(Leds &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {

    if (leds.size != Coord3D{0,0,0}) return;
    ppf ("Default Projection %dD -> %dD Effect  Size: %d,%d,%d Pixel: %d,%d,%d ->", leds.projectionDimension, leds.effectDimension, sizeAdjusted.x, sizeAdjusted.y, sizeAdjusted.z, pixelAdjusted.x, pixelAdjusted.y, pixelAdjusted.z);
    switch (leds.effectDimension) {
      case _1D: // effectDimension 1DxD
          sizeAdjusted.x = sqrt(sq(max(sizeAdjusted.x - midPosAdjusted.x, midPosAdjusted.x)) + 
                                sq(max(sizeAdjusted.y - midPosAdjusted.y, midPosAdjusted.y)) + 
                                sq(max(sizeAdjusted.z - midPosAdjusted.z, midPosAdjusted.z))) + 1;
          sizeAdjusted.y = 1;
          sizeAdjusted.z = 1;
          break;
      case _2D: // effectDimension 2D
          switch (leds.projectionDimension) {
              case _1D: // 2D1D
                  sizeAdjusted.x = sqrt(sizeAdjusted.x * sizeAdjusted.y * sizeAdjusted.z); // only one is > 1, square root
                  sizeAdjusted.y = sizeAdjusted.x * sizeAdjusted.y * sizeAdjusted.z / sizeAdjusted.x;
                  sizeAdjusted.z = 1;
                  break;
              case _2D: // 2D2D
                  // find the 2 axes
                  if (sizeAdjusted.x > 1) {
                      if (sizeAdjusted.y <= 1) {
                          sizeAdjusted.y = sizeAdjusted.z;
                      }
                  } else {
                      sizeAdjusted.x = sizeAdjusted.y;
                      sizeAdjusted.y = sizeAdjusted.z;
                  }
                  sizeAdjusted.z = 1;
                  break;
              case _3D: // 2D3D
                  sizeAdjusted.x = sizeAdjusted.x + sizeAdjusted.y / 2;
                  sizeAdjusted.y = sizeAdjusted.y / 2 + sizeAdjusted.z;
                  sizeAdjusted.z = 1;
                  break;
          }
          break;
      case _3D: // effectDimension 3D
          switch (leds.projectionDimension) {
              case _1D:
                  sizeAdjusted.x = std::pow(sizeAdjusted.x * sizeAdjusted.y * sizeAdjusted.z, 1/3); // only one is > 1, cube root
                  break;
              case _2D:
                  break;
              case _3D:
                  break;
          }
          break;
    }
    ppf (" Size: %d,%d,%d Pixel: %d,%d,%d\n", sizeAdjusted.x, sizeAdjusted.y, sizeAdjusted.z, pixelAdjusted.x, pixelAdjusted.y, pixelAdjusted.z);
  }

  void adjustMapped(Leds &leds, Coord3D &mapped, Coord3D sizeAdjusted, Coord3D pixelAdjusted, Coord3D midPosAdjusted) {
    switch (leds.effectDimension) {
      case _1D: // effectDimension 1DxD
          mapped.x = pixelAdjusted.distance(midPosAdjusted);
          mapped.y = 0;
          mapped.z = 0;
          break;
      case _2D: // effectDimension 2D
          switch (leds.projectionDimension) {
              case _1D: // 2D1D
                  mapped.x = (pixelAdjusted.x + pixelAdjusted.y + pixelAdjusted.z) % leds.size.x; // only one > 0
                  mapped.y = (pixelAdjusted.x + pixelAdjusted.y + pixelAdjusted.z) / leds.size.x; // all rows next to each other
                  mapped.z = 0;
                  break;
              case _2D: // 2D2D
                  if (sizeAdjusted.x > 1) {
                      mapped.x = pixelAdjusted.x;
                      if (sizeAdjusted.y > 1) {
                          mapped.y = pixelAdjusted.y;
                      } else {
                          mapped.y = pixelAdjusted.z;
                      }
                  } else {
                      mapped.x = pixelAdjusted.y;
                      mapped.y = pixelAdjusted.z;
                  }
                  mapped.z = 0;
                  break;
              case _3D: // 2D3D
                  mapped.x = pixelAdjusted.x + pixelAdjusted.y / 2;
                  mapped.y = pixelAdjusted.y / 2 + pixelAdjusted.z;
                  mapped.z = 0;
                  break;
          }
          break;
      case _3D: // effectDimension 3D
          mapped = pixelAdjusted;
          break;
    }
    // ppf("Default %dD Effect -> %dD   %d,%d,%d -> %d,%d,%d\n", leds.effectDimension, leds.projectionDimension, pixelAdjusted.x, pixelAdjusted.y, pixelAdjusted.z, mapped.x, mapped.y, mapped.z);
  }

  void controls(Leds &leds, JsonObject parentVar) {
  }

}; //DefaultProjection

class PinwheelProjection: public Projection {
  // Currently 1D to 2D/3D May be possible to make 2D to 2D/3D
  const char * name() {return "Pinwheel";}
  const char * tags() {return "💡";}

  public:

  void adjustSizeAndPixel(Leds &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    sizeAdjusted.x = mdl->getValue("Petals");
    sizeAdjusted.y = 1;
    sizeAdjusted.z = 1;
  }

  void adjustMapped(Leds &leds, Coord3D &mapped, Coord3D sizeAdjusted, Coord3D pixelAdjusted, Coord3D midPosAdjusted) {
    // UI Variables
    int swirlVal   = leds.swirlVal; //mdl->getValue("swirlVal");
    bool reverse   = leds.reverseTransform & (1 << 6);  //mdl->getValue("reverse");
    int zTwist     = leds.zTwist; //mdl->getValue("zTwist");
    int angleRange = leds.angleRange; //max(1, int(mdl->getValue("angleRange")));
    float petals   = leds.petals; //max(1, int(mdl->getValue("Petals")));

    int dx = pixelAdjusted.x - midPosAdjusted.x;
    int dy = pixelAdjusted.y - midPosAdjusted.y;
    int swirlFactor = swirlVal == 0 ? 0 : hypot(dy, dx) * abs(swirlVal); // Only calculate if swirlVal != 0
    int angle       = degrees(atan2(dy, dx)) + 180;  // 0 - 360
    
    if (swirlVal < 0) angle = 360 - angle; // Reverse Swirl

    int value = angle + swirlFactor + (zTwist * pixelAdjusted.z);
    float petalWidth = angleRange / petals;
    value /= petalWidth;
    value %= int(petals);

    if (reverse) value = petals - value - 1; // Reverse Movement

    mapped.x = value;
    mapped.y = 0;
    mapped.z = 0;

    // if (pixelAdjusted.x == 0 && pixelAdjusted.y == 0 && pixelAdjusted.z == 0) ppf("Pinwheel  Center: (%d, %d) SwirlVal: %d angleRange: %d Petals: %f zTwist: %d\n", midPosAdjusted.x, midPosAdjusted.y, swirlVal, angleRange, petals, zTwist);
    // ppf("pixelAdjusted %d,%d,%d -> %d,%d,%d angle %d\n", pixelAdjusted.x, pixelAdjusted.y, pixelAdjusted.z, mapped.x, mapped.y, mapped.z, angle);
  }
  void controls(Leds &leds, JsonObject parentVar) {
    ui->initSlider(parentVar, "Swirl", 0, -30, 30, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.swirlVal = mdl->getValue("Swirl");
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
    // use reverse class when implemented
    ui->initCheckBox(parentVar, "reverse", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.reverseTransform = mdl->getValue("reverse") ? leds.reverseTransform | (1 << 6) : leds.reverseTransform & ~(1 << 6);
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
    // Testing zTwist range -42 to 42 arbitrary values for testing. Hide if not 3D fixture. Select pinwheel while using 3D fixture.
    if (leds.projectionDimension == _3D) {
      ui->initSlider(parentVar, "zTwist", 0, -42, 42, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
        case onChange:
          leds.zTwist = mdl->getValue("zTwist");
          leds.fixture->listOfLeds[rowNr]->doMap = true;
          leds.fixture->doMap = true;
          return true;
        default: return false;
      }});
    }
    // Angle range 0 - angleRange. For testing purposes
    ui->initNumber(parentVar, "angleRange", 360, 1, 720, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.angleRange = mdl->getValue("angleRange");
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
    // Naming petals, arms, blades, rays? Controls virtual strip length.
    ui->initNumber(parentVar, "Petals", 360, 1, 360, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.petals = mdl->getValue("Petals");
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
  }
}; //PinwheelProjection

class RandomProjection: public Projection { // Coded in LedFixture.cpp
  const char * name() {return "Random";}
  const char * tags() {return "💫";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //RandomProjection

class KaleidoscopeProjection: public Projection { // Empty WIP
  const char * name() {return "Kaleidoscope WIP";}
  const char * tags() {return "💫";}

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //KaleidoscopeProjection


// MODIFIERS
// Only adjust size and pixel, not mapped
// Currently first resize call is stored in leds->size. sizeAdjusted gets reset every pixel
// If this changes in the future modify these.
class MultiplyProjection: public Projection {
  const char * name() {return "Multiply";}
  const char * tags() {return "💫";}

  public:

  void adjustSizeAndPixel(Leds &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    // UI Variables
    Coord3D proMulti = leds.multiplySize; //mdl->getValue("proMulti");
    proMulti = proMulti.maximum(Coord3D{1, 1, 1}); // {1, 1, 1} is the minimum value
    if (proMulti == Coord3D{1, 1, 1}) return;
    
    bool mirror = mdl->getValue("mirror");

    // ppf ("Multiply %d,%d,%d Before Size: %d,%d,%d Pixel: %d,%d,%d ->", proMulti.x, proMulti.y, proMulti.z, sizeAdjusted.x, sizeAdjusted.y, sizeAdjusted.z, pixelAdjusted.x, pixelAdjusted.y, pixelAdjusted.z);
   
    sizeAdjusted = (sizeAdjusted + proMulti - 1) / proMulti; // round up
    midPosAdjusted /= proMulti;

    if (mirror) {
      Coord3D mirrors = pixelAdjusted / sizeAdjusted; //place the pixel in the right quadrant
      pixelAdjusted = pixelAdjusted % sizeAdjusted;
      if (mirrors.x %2 != 0) pixelAdjusted.x = sizeAdjusted.x - 1 - pixelAdjusted.x;
      if (mirrors.y %2 != 0) pixelAdjusted.y = sizeAdjusted.y - 1 - pixelAdjusted.y;
      if (mirrors.z %2 != 0) pixelAdjusted.z = sizeAdjusted.z - 1 - pixelAdjusted.z;
    }
    else pixelAdjusted = pixelAdjusted % sizeAdjusted;

    // ppf (" Size: %d,%d,%d Pixel: %d,%d,%d Mirrors: %d,%d,%d\n", sizeAdjusted.x, sizeAdjusted.y, sizeAdjusted.z, pixelAdjusted.x, pixelAdjusted.y, pixelAdjusted.z, mirrors.x, mirrors.y, mirrors.z);
  }

  void controls(Leds &leds, JsonObject parentVar) {
    ui->initCoord3D(parentVar, "proMulti", leds.multiplySize, 0, 10, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onUI:
        ui->setLabel(var, "MultiplyX");
        return true;
      case onChange:
        ui->initCheckBox(var, "mirror", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
          case onChange:
            if (rowNr < leds.fixture->listOfLeds.size()) {
              leds.mirrorTransform = mdl->getValue("mirror") ? leds.mirrorTransform | (1 << 3) : leds.mirrorTransform & ~(1 << 3);
              // leds.mirrorTransform ^= (1 << 3);
              leds.fixture->listOfLeds[rowNr]->doMap = true;
              leds.fixture->doMap = true;
            }
            return true;
          default: return false;
        }});
        if (rowNr < leds.fixture->listOfLeds.size()) {
          leds.multiplySize = mdl->getValue("proMulti");
          leds.fixture->listOfLeds[rowNr]->doMap = true;
          leds.fixture->doMap = true;
        }
        return true;
      default: return false;
    }});
  }
}; //MultiplyProjection

class ReverseProjection: public Projection { // Maybe transformer?
  const char * name() {return "Reverse WIP";}
  const char * tags() {return "💡";}

  public:

  void adjustSizeAndPixel(Leds &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    // UI Variables
    bool reverseX = leds.reverseTransform & (1 << 0);
    bool reverseY = leds.reverseTransform & (1 << 1);
    bool reverseZ = leds.reverseTransform & (1 << 2);
    // bool reverseX = mdl->getValue("reverse X");
    // bool reverseY = mdl->getValue("reverse Y");
    // bool reverseZ = mdl->getValue("reverse Z");

    if (reverseX) pixelAdjusted.x = sizeAdjusted.x - pixelAdjusted.x - 1;
    if (reverseY) pixelAdjusted.y = sizeAdjusted.y - pixelAdjusted.y - 1;
    if (reverseZ) pixelAdjusted.z = sizeAdjusted.z - pixelAdjusted.z - 1;
  }

  void controls(Leds &leds, JsonObject parentVar) {
    ui->initCheckBox(parentVar, "reverse X", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.reverseTransform = mdl->getValue("reverse X") ? leds.reverseTransform | (1 << 0) : leds.reverseTransform & ~(1 << 0);
        // leds.reverseTransform ^= (1 << 0);
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
    ui->initCheckBox(parentVar, "reverse Y", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.reverseTransform = mdl->getValue("reverse Y") ? leds.reverseTransform | (1 << 1) : leds.reverseTransform & ~(1 << 1);
        // leds.reverseTransform ^= (1 << 1);
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
    ui->initCheckBox(parentVar, "reverse Z", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.reverseTransform = mdl->getValue("reverse Z") ? leds.reverseTransform | (1 << 2) : leds.reverseTransform & ~(1 << 2);
        // leds.reverseTransform ^= (1 << 2);
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
  }
}; //ReverseProjection

class MirrorProjection: public Projection {
  const char * name() {return "Mirror WIP";}
  const char * tags() {return "💡";}

  public:

  void adjustSizeAndPixel(Leds &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    // UI Variables
    bool mirrorX = leds.mirrorTransform & (1 << 0);
    bool mirrorY = leds.mirrorTransform & (1 << 1);
    bool mirrorZ = leds.mirrorTransform & (1 << 2);

    // bool mirrorX = mdl->getValue("mirror X");
    // bool mirrorY = mdl->getValue("mirror Y");
    // bool mirrorZ = mdl->getValue("mirror Z");

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
        leds.mirrorTransform = mdl->getValue("mirror X") ? leds.mirrorTransform | (1 << 0) : leds.mirrorTransform & ~(1 << 0);
        // leds.mirrorTransform ^= (1 << 0);
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
    ui->initCheckBox(parentVar, "mirror Y", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.mirrorTransform = mdl->getValue("mirror Y") ? leds.mirrorTransform | (1 << 1) : leds.mirrorTransform & ~(1 << 1);
        // leds.mirrorTransform ^= (1 << 1);
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
    ui->initCheckBox(parentVar, "mirror Z", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.mirrorTransform = mdl->getValue("mirror Z") ? leds.mirrorTransform | (1 << 2) : leds.mirrorTransform & ~(1 << 2);
        // leds.mirrorTransform ^= (1 << 2);
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
  }
}; //MirrorProjection

class GroupingProjection: public Projection {
  const char * name() {return "Grouping";}
  const char * tags() {return "🔲";}

  public:

  void adjustSizeAndPixel(Leds &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    // UI Variables
    Coord3D grouping = leds.groupSize; //mdl->getValue("Grouping");
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
    ui->initCoord3D(parentVar, "Grouping", leds.groupSize, 0, 100, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.groupSize = mdl->getValue("Grouping");
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
  }
};

class SpacingProjection: public Projection { // Empty WIP
  const char * name() {return "Spacing WIP";}
  const char * tags() {return "💡";}

  public:

  void controls(Leds &leds, JsonObject parentVar) {
  }
}; //SpacingProjection

class TransposeProjection: public Projection { // Maybe transformer?
  const char * name() {return "Transpose";}
  const char * tags() {return "🔄";}

  public:

  void adjustSizeAndPixel(Leds &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    // UI Variables
    bool transposeXY = leds.reverseTransform & (1 << 3);
    bool transposeXZ = leds.reverseTransform & (1 << 4);
    bool transposeYZ = leds.reverseTransform & (1 << 5);

    // bool transposeXY = mdl->getValue("transpose XY");
    // bool transposeXZ = mdl->getValue("transpose XZ");
    // bool transposeYZ = mdl->getValue("transpose YZ");

    if (transposeXY) { int temp = pixelAdjusted.x; pixelAdjusted.x = pixelAdjusted.y; pixelAdjusted.y = temp; }
    if (transposeXZ) { int temp = pixelAdjusted.x; pixelAdjusted.x = pixelAdjusted.z; pixelAdjusted.z = temp; }
    if (transposeYZ) { int temp = pixelAdjusted.y; pixelAdjusted.y = pixelAdjusted.z; pixelAdjusted.z = temp; }
  }

  void controls(Leds &leds, JsonObject parentVar) {
    ui->initCheckBox(parentVar, "transpose XY", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.reverseTransform = mdl->getValue("transpose XY") ? leds.reverseTransform | (1 << 3) : leds.reverseTransform & ~(1 << 3);
        // leds.reverseTransform ^= (1 << 3);
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
    ui->initCheckBox(parentVar, "transpose XZ", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.reverseTransform = mdl->getValue("transpose XZ") ? leds.reverseTransform | (1 << 4) : leds.reverseTransform & ~(1 << 4);
        // leds.reverseTransform ^= (1 << 4);
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
    ui->initCheckBox(parentVar, "transpose YZ", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.reverseTransform = mdl->getValue("transpose YZ") ? leds.reverseTransform | (1 << 5) : leds.reverseTransform & ~(1 << 5);
        // leds.reverseTransform ^= (1 << 5);
        leds.fixture->listOfLeds[rowNr]->doMap = true;
        leds.fixture->doMap = true;
        return true;
      default: return false;
    }});
  }
}; //TransposeProjection

// TRANSFORMERS
// Transforms final projection output
// Shouldn't need to readjust all mappings
class TiltPanRollProjection: public Projection { // Currently calls default projection fix later.
  const char * name() {return "TiltPanRoll";}
  const char * tags() {return "💫";}

  public: //to use in Preset1Projection

  void adjustSizeAndPixel(Leds &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    DefaultProjection dp;
    dp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
  }

  void adjustMapped(Leds &leds, Coord3D &mapped, Coord3D sizeAdjusted, Coord3D pixelAdjusted, Coord3D midPosAdjusted) {
    DefaultProjection dp;
    dp.adjustMapped(leds, mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted);
  }

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

class NewTransposeProjection: public Projection { // Transformer
  const char * name() {return "Transpose";}
  const char * tags() {return "🔄";}

  public:

  void adjustXYZ(Leds &leds, Coord3D &pixel) {

    if (leds.reverseTransform == 0) return;

    bool transposeXY = leds.reverseTransform & (1 << 3); //mdl->getValue("transpose XY")
    if (transposeXY) { int temp = pixel.x; pixel.x = pixel.y; pixel.y = temp; }

    if (leds.projectionDimension < _3D) return;
    bool transposeXZ = leds.reverseTransform & (1 << 4); //mdl->getValue("transpose XZ");
    if (transposeXZ) { int temp = pixel.x; pixel.x = pixel.z; pixel.z = temp; }
    bool transposeYZ = leds.reverseTransform & (1 << 5); //mdl->getValue("transpose YZ");
    if (transposeYZ) { int temp = pixel.y; pixel.y = pixel.z; pixel.z = temp; }

  }

  void controls(Leds &leds, JsonObject parentVar) {
    ui->initCheckBox(parentVar, "transpose XY", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.reverseTransform ^= (1 << 3);
        return true;
      default: return false;
    }});
    ui->initCheckBox(parentVar, "transpose XZ", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.reverseTransform ^= (1 << 4);
        return true;
      default: return false;
    }});
    ui->initCheckBox(parentVar, "transpose YZ", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.reverseTransform ^= (1 << 5);
        return true;
      default: return false;
    }});
  }
}; //TransposeProjection

class NewReverseProjection: public Projection { //Transformer
  const char * name() {return "Reverse WIP";}
  const char * tags() {return "💡";}

  public:

  void adjustXYZ(Leds &leds, Coord3D &pixel) {
    
    if (leds.reverseTransform == 0) return;

    bool reverseX = leds.reverseTransform & (1 << 0); //mdl->getValue("reverse X");
    bool reverseY = leds.reverseTransform & (1 << 1); //mdl->getValue("reverse Y");
    bool reverseZ = leds.reverseTransform & (1 << 2); //mdl->getValue("reverse Z");

    if (reverseX) pixel.x = leds.size.x - pixel.x - 1;
    if (reverseY) pixel.y = leds.size.y - pixel.y - 1;
    if (reverseZ) pixel.z = leds.size.z - pixel.z - 1;
  }

  void controls(Leds &leds, JsonObject parentVar) {
    ui->initCheckBox(parentVar, "reverse X", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.reverseTransform ^= (1 << 0);
        return true;
      default: return false;
    }});
    ui->initCheckBox(parentVar, "reverse Y", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.reverseTransform ^= (1 << 1);
        return true;
      default: return false;
    }});
    ui->initCheckBox(parentVar, "reverse Z", false, false, [&leds](JsonObject var, unsigned8 rowNr, unsigned8 funType) { switch (funType) { //varFun
      case onChange:
        leds.reverseTransform ^= (1 << 2);
        return true;
      default: return false;
    }});
  }
}; //ReverseProjection


// PRESETS 
// Temp presets for testing
// Not needed with modifiers implemented in UI
class Preset1Projection: public Projection {
  const char * name() {return "Preset1";}
  const char * tags() {return "💫";}

  void adjustSizeAndPixel(Leds &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    DefaultProjection dp;
    dp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    MultiplyProjection mp;
    mp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
  }

  void adjustMapped(Leds &leds, Coord3D &mapped, Coord3D sizeAdjusted, Coord3D pixelAdjusted, Coord3D midPosAdjusted) {
    DefaultProjection dp;
    dp.adjustMapped(leds, mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted);
  }

  void adjustXYZ(Leds &leds, Coord3D &pixel) {
    TiltPanRollProjection tp;
    tp.adjustXYZ(leds, pixel);
  }

  void controls(Leds &leds, JsonObject parentVar) {
    MultiplyProjection mp;
    mp.controls(leds, parentVar);
    TiltPanRollProjection tp;
    tp.controls(leds, parentVar);
  }
}; //Preset1Projection

class PinwheelModProjection: public Projection {
  const char * name() {return "Modified Pinwheel";}
  const char * tags() {return "💫🏋️";}

  void adjustSizeAndPixel(Leds &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    TransposeProjection tp;
    tp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    ReverseProjection rp;
    rp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    MirrorProjection mp;
    mp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    GroupingProjection gp;
    gp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    MultiplyProjection multp;
    multp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    PinwheelProjection pp;
    pp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
  }

  void adjustMapped(Leds &leds, Coord3D &mapped, Coord3D sizeAdjusted, Coord3D pixelAdjusted, Coord3D midPosAdjusted) {
    PinwheelProjection pp;
    pp.adjustMapped(leds, mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted);
  }

  void controls(Leds &leds, JsonObject parentVar) {
    PinwheelProjection pp;
    pp.controls(leds, parentVar);
    MultiplyProjection multp;
    multp.controls(leds, parentVar);
    GroupingProjection gp;
    gp.controls(leds, parentVar);
    ReverseProjection rp;
    rp.controls(leds, parentVar);
    MirrorProjection mp;
    mp.controls(leds, parentVar);
    TransposeProjection tp;
    tp.controls(leds, parentVar);
  }
}; //DefaultModProjection

class DefaultModProjection: public Projection {
  const char * name() {return "Modified Default";}
  const char * tags() {return "💫🏋️";}

  void adjustSizeAndPixel(Leds &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    TransposeProjection tp;
    tp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    ReverseProjection rp;
    rp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    MirrorProjection mp;
    mp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    GroupingProjection gp;
    gp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    MultiplyProjection multp;
    multp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
    DefaultProjection dp;
    dp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);    
  }

  void adjustMapped(Leds &leds, Coord3D &mapped, Coord3D sizeAdjusted, Coord3D pixelAdjusted, Coord3D midPosAdjusted) {
    DefaultProjection dp;
    dp.adjustMapped(leds, mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted);
  }

  void controls(Leds &leds, JsonObject parentVar) {
    MultiplyProjection multp;
    multp.controls(leds, parentVar);
    GroupingProjection gp;
    gp.controls(leds, parentVar);
    ReverseProjection rp;
    rp.controls(leds, parentVar);
    MirrorProjection mp;
    mp.controls(leds, parentVar);
    TransposeProjection tp;
    tp.controls(leds, parentVar);
  }
}; //DefaultModProjection

//Experimental Projections Uses XYZ for reverse and transpose

// class PinwheelModProjection: public Projection {
//   const char * name() {return "Modified Exp Pinwheel";}
//   const char * tags() {return "💫🏋️";}

//   void adjustSizeAndPixel(Leds &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
//     MirrorProjection mp;
//     mp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
//     GroupingProjection gp;
//     gp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
//     MultiplyProjection multp;
//     multp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
//     PinwheelProjection pp;
//     pp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
//   }

//   void adjustMapped(Leds &leds, Coord3D &mapped, Coord3D sizeAdjusted, Coord3D pixelAdjusted, Coord3D midPosAdjusted) {
//     MirrorProjection mp;
//     mp.adjustMapped(leds, mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted);
//     GroupingProjection gp;
//     gp.adjustMapped(leds, mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted);
//     MultiplyProjection multp;
//     multp.adjustMapped(leds, mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted);
//     PinwheelProjection pp;
//     pp.adjustMapped(leds, mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted);
//   }

//   void adjustXYZ(Leds &leds, Coord3D &pixel) {
//     NewTransposeProjection tp;
//     tp.adjustXYZ(leds, pixel);
//     NewReverseProjection rp;
//     rp.adjustXYZ(leds, pixel);
//   }

//   void controls(Leds &leds, JsonObject parentVar) {
//     PinwheelProjection pp;
//     pp.controls(leds, parentVar);
//     MultiplyProjection multp;
//     multp.controls(leds, parentVar);
//     GroupingProjection gp;
//     gp.controls(leds, parentVar);
//     MirrorProjection mp;
//     mp.controls(leds, parentVar);
//     NewReverseProjection rp;
//     rp.controls(leds, parentVar);
//     NewTransposeProjection tp;
//     tp.controls(leds, parentVar);
//   }
// }; //DefaultModProjection

// class DefaultModProjection: public Projection {
//   const char * name() {return "Modified Exp Default";}
//   const char * tags() {return "💫🏋️";}

//   void adjustSizeAndPixel(Leds &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
//     MirrorProjection mp;
//     mp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
//     GroupingProjection gp;
//     gp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
//     MultiplyProjection multp;
//     multp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
//     DefaultProjection dp;
//     dp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);    
//   }

//   void adjustMapped(Leds &leds, Coord3D &mapped, Coord3D sizeAdjusted, Coord3D pixelAdjusted, Coord3D midPosAdjusted) {
//     MirrorProjection mp;
//     mp.adjustMapped(leds, mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted);
//     GroupingProjection gp;
//     gp.adjustMapped(leds, mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted);
//     MultiplyProjection multp;
//     multp.adjustMapped(leds, mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted);
//     DefaultProjection dp;
//     dp.adjustMapped(leds, mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted);
//   }

//   void adjustXYZ(Leds &leds, Coord3D &pixel) {
//     NewTransposeProjection tp;
//     tp.adjustXYZ(leds, pixel);
//     NewReverseProjection rp;
//     rp.adjustXYZ(leds, pixel);
//   }

//   void controls(Leds &leds, JsonObject parentVar) {
//     MultiplyProjection multp;
//     multp.controls(leds, parentVar);
//     GroupingProjection gp;
//     gp.controls(leds, parentVar);
//     MirrorProjection mp;
//     mp.controls(leds, parentVar);
//     NewReverseProjection rp;
//     rp.controls(leds, parentVar);
//     NewTransposeProjection tp;
//     tp.controls(leds, parentVar);
//   }
// }; //DefaultModProjection

// LEGACY Remove or rework later. Hardcoded in LedFixture.cpp
class DistanceFromPointProjection: public Projection {
  const char * name() {return "Distance ⌛";}
  const char * tags() {return "💫";}

  public: //to use in Preset1Projection

  void adjustSizeAndPixel(Leds &leds, Coord3D &sizeAdjusted, Coord3D &pixelAdjusted, Coord3D &midPosAdjusted) {
    DefaultProjection dp;
    dp.adjustSizeAndPixel(leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
  }

  void adjustMapped(Leds &leds, Coord3D &mapped, Coord3D sizeAdjusted, Coord3D pixelAdjusted, Coord3D midPosAdjusted) {
    DefaultProjection dp;
    dp.adjustMapped(leds, mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted);
  }
}; //DistanceFromPointProjection