/*
   @title     StarLight
   @file      LedLayer.h
   @date      20241105
   @repo      https://github.com/MoonModules/StarLight
   @Authors   https://github.com/MoonModules/StarLight/commits/main
   @Copyright © 2024 Github StarLight Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#pragma once
// FastLED optional flags to configure drivers, see https://github.com/FastLED/FastLED/blob/master/src/platforms/esp/32
// RMT driver (default)
// #define FASTLED_ESP32_FLASH_LOCK 1    // temporarily disabled FLASH file access while driving LEDs (may prevent random flicker)
// #define FASTLED_RMT_BUILTIN_DRIVER 1  // in case your app needs to use RMT units, too (slower)
// I2S parallel driver
// #define FASTLED_ESP32_I2S true        // to use I2S parallel driver (instead of RMT)
// #define I2S_DEVICE 1                  // I2S driver: allows to still use I2S#0 for audio (only on esp32 and esp32-s3)
// #define FASTLED_I2S_MAX_CONTROLLERS 8 // 8 LED pins should be enough (default = 24)

#include "FastLED.h" //CRGB

#include "../Sys/SysModModel.h" //for Coord3D

#ifndef STARLIGHT_MAXLEDS
  #define STARLIGHT_MAXLEDS 8192 //any board can do this
#endif

class LedsLayer; //forward

#define _1D 1
#define _2D 2
#define _3D 3

//should not contain variables/bytes to keep mem as small as possible!!
class Effect {
public:
  virtual ~Effect() = default;

  virtual const char * name() {return "noname";}
  virtual const char * tags() {return "";}
  virtual uint8_t dim() {return _1D;};

  virtual void setup(LedsLayer &leds, Variable parentVar);

  virtual void loop(LedsLayer &leds) {}
};

class Projection {
public:
  virtual ~Projection() = default;

  virtual const char * name() {return "noname";}
  virtual const char * tags() {return "";}

  virtual void setup(LedsLayer &leds, Variable parentVar) {}

  //per frame
  virtual void loop(LedsLayer &leds) {}
  
  //setupPixels
  virtual void addPixelsPre(LedsLayer &leds) {}

  //setupPixel
  virtual void addPixel(LedsLayer &leds, Coord3D &pixel) {} //not const as pixel is changed

  //loopPixel
  virtual void XYZ(LedsLayer &leds, Coord3D &pixel) {}
};

enum mapType {
  m_color,
  m_onePixel,
  m_morePixels,
  m_count //keep as last entry
};

struct PhysMap {
  union {
    struct {                 //condensed rgb
      uint16_t rgb14: 14;    //14 bits (554 RGB)
      byte mapType:2;        //2 bits (4)
    }; //16 bits
    uint16_t indexP: 14;   //16384 one physical pixel (type==1) index to ledsP array
    uint16_t indexes:14;  //16384 multiple physical pixels (type==2) index in std::vector<std::vector<uint16_t>> mappingTableIndexes;
  }; // 2 bytes

  PhysMap() {
    mapType = m_color; // the default until indexP is added
    rgb14 = 0;
  }

  void addIndexP(LedsLayer &leds, uint16_t indexP);

}; // 2 bytes

//StarLight implementation of segment.data
class SharedData {

  private:
    byte *data = nullptr;
    uint16_t index = 0;
    bool dataAllocated = true;

  public:
    uint16_t bytesAllocated = 0;
    bool alertIfChanged = false;

  SharedData() {
    ppf("SharedData constructor %d %d\n", index, bytesAllocated);
  }
  ~SharedData() {
    ppf("SharedData destructor WIP %d %d\n", index, bytesAllocated);
    clear();
  }

  void clear() {
    ppf("SharedData clearing data %d %d %p\n", index, bytesAllocated, data);
    if (data) {
      free(data);
      data = nullptr;
    }
    bytesAllocated = 0;
    alertIfChanged = false;
    dataAllocated = true;
    begin();
  }

  //sets the effectData pointer back to 0 so loop effect can go through it
  void begin() {
    index = 0;
  }

  //returns the next pointer to a specified type (length for arrays)
  template <typename Type>
  Type * readWrite(int length = 1) {
    if (!dataAllocated) return nullptr;
    size_t newIndex = index + length * sizeof(Type);
    if (newIndex > bytesAllocated) {
      size_t newSize = bytesAllocated + (1 + ( newIndex - bytesAllocated)/32) * 32; // add a multitude of 32 bytes
      ppf("sharedData.readWrite add more %d->%d %d->%d\n", index, newIndex, bytesAllocated, newSize);
      if (bytesAllocated == 0)
        data = (byte*) malloc(newSize);
      else
        data = (byte*)reallocf(data, newSize);
      if (data != nullptr) { //only if alloc is successful
        memset(data, 0, newSize); //init data with 0
        if (alertIfChanged)
          ppf("dev sharedData.readWrite reallocating, this should not happen ! %d -> %d\n", bytesAllocated, newSize);
        bytesAllocated = newSize;
      }
      else {
        ppf("dev sharedData.readWrite, alloc not successful %d->%d %d->%d\n", index, newIndex, bytesAllocated, newSize);
        dataAllocated = false;
      }
    }
    // ppf("bind %d->%d %d\n", index, newIndex, bytesAllocated);
    Type * returnValue  = reinterpret_cast<Type *>(data + index);
    index = newIndex; //add consumed amount of bytes, index is next byte which will be pointed to
    return returnValue;
  }

  //returns the next pointer initialized by a value (length for arrays not supported yet)
  template <typename Type>
  Type * write(Type initValue) {
    Type * returnValue =  readWrite<Type>();
    *returnValue = initValue;
    return returnValue;
  }

  //returns the next value (length for arrays not supported yet)
  template <typename Type>
  Type read() {
    Type *result = readWrite<Type>(); //not supported for arrays yet
    return *result;
  }

  bool success() const {
    return dataAllocated;
  }

};

class LedsLayer {

public:

  Coord3D size = {8,8,1}; //not 0,0,0 to prevent div0 eg in Octopus2D

  Effect *effect = nullptr;
  Projection *projection = nullptr;

  //using cached virtual class methods! 4 bytes each - that's for now the price we pay for speed
      //setting cached virtual class methods! (By chatGPT so no source and don't understand how it works - scary!)
      //   (don't know how it works as it is not referring to derived classes, just to the base class but later it calls the derived class method)
  void (Projection::*addPixelsPreCached)(LedsLayer &) = &Projection::addPixelsPre;
  void (Projection::*addPixelCached)(LedsLayer &, Coord3D &) = &Projection::addPixel;
  void (Projection::*XYZCached)(LedsLayer &, Coord3D &) = &Projection::XYZ;
  void (Projection::*loopCached)(LedsLayer &) = &Projection::loop;

  uint8_t effectDimension = UINT8_MAX;
  uint8_t projectionDimension = UINT8_MAX;

  Coord3D start = {0,0,0}, middle = {0,0,0}, end = {0,0,0};//{UINT16_MAX,UINT16_MAX,UINT16_MAX}; //default

  #ifdef STARBASE_USERMOD_MPU6050
    bool proGyro = false;
  #endif
  uint8_t proTiltSpeed = 128;
  uint8_t proPanSpeed = 128;
  uint8_t proRollSpeed = 128;

  SharedData effectData;
  SharedData projectionData;

  std::vector<PhysMap> mappingTable;
  uint16_t mappingTableSizeUsed = 0;
  std::vector<std::vector<uint16_t>> mappingTableIndexes;
  uint16_t mappingTableIndexesSizeUsed = 0;
  
  bool doMap = true; //so a mapping will be made

  CRGBPalette16 palette;

  #ifdef STARBASE_USERMOD_LIVE
    uint8_t liveEffectID = UINT8_MAX;
  #endif

  int XY(int x, int y) {
    return XYZ(x, y, 0);
  }

  //use inBounds with care (or not at all) is sPC, gPC just ignores out of bounds
  bool inBounds(int x, int y, int z = 0) const;
  bool inBounds(const Coord3D & pos) const;

  int XYZUnprojected(int x, int y, int z) const;
  int XYZUnprojected(const Coord3D &pixel) const;

  int XYZ(int x, int y, int z); // function not const as it calls projection which changes things
  int XYZ(Coord3D pixel); //pixel not const as pixel can be changed by projection, not & is it can change the pixel locally for projections ... (because called with {x,y,z} ..._

  LedsLayer() {
    ppf("LedsLayer constructor (PhysMap:%d)\n", sizeof(PhysMap));
  }

  ~LedsLayer() {
    ppf("LedsLayer destructor\n");
    fadeToBlackBy();
    doMap = true; // so loop is not running while deleting
    for (std::vector<uint16_t> mappingTableIndex: mappingTableIndexes) {
      mappingTableIndex.clear();
    }
    mappingTableIndexes.clear();
    mappingTable.clear();
  }

  void triggerMapping();

  //set in operator[], used by other operators
  uint16_t operatorIndexV = 0;
  CRGB operatorCRGB;

  //leds = leds[indexV] ,[] needs to return LedsLayer to allow other operators to work on it
  LedsLayer& operator[](const uint16_t indexV) {
    operatorIndexV = indexV;
    operatorCRGB = getPixelColor(operatorIndexV); 
    return *this;
  }

  //leds = leds[pos]
  LedsLayer& operator[](const Coord3D &pos) {
    operatorIndexV = XYZ(pos.x, pos.y, pos.z);
    operatorCRGB = getPixelColor(operatorIndexV);
    return *this;
  }

  //leds = color
  LedsLayer& operator=(const CRGB &color) {
    setPixelColor(operatorIndexV, color);
    return *this;
  }

  //leds = leds += color
  LedsLayer& operator+=(const CRGB &color) {
    setPixelColor(operatorIndexV, operatorCRGB + color);
    return *this;
  }

  //leds = leds != color
  LedsLayer& operator|=(const CRGB &color) {
    // setPixelColor(operatorIndexV, color);
    setPixelColor(operatorIndexV, operatorCRGB | color);
    return *this;
  }

  //leds = leds + color
  LedsLayer& operator+(const CRGB &color) {
    setPixelColor(operatorIndexV, operatorCRGB + color);
    return *this;
  }

  //CRGB = leds[].color - color
  CRGB& operator-(const CRGB &color) {
    // setPixelColor(operatorIndexV, getPixelColor(operatorIndexV) + color);
    operatorCRGB -= color;
    return operatorCRGB;
  }

  //CRGB = leds[] = leds[]
  // CRGB& operator=(LedsLayer& leds) {
  //   return operatorCRGB;
  // }


  //bool = leds != color
  bool operator!=(const CRGB &color) const {
    return operatorCRGB != color;
  }

  //bool = leds[]<crgb
  bool operator<(const CRGB &color) const {
    return operatorCRGB < color;
  }


  // maps the virtual led to the physical led(s) and assign a color to it
  void setPixelColor(int indexV, const CRGB& color);
  void setPixelColor(int x, int y, const CRGB& color) {setPixelColor(XYZ(x, y, 0), color);}
  void setPixelColor(int x, int y, int z, const CRGB& color) {setPixelColor(XYZ(x, y, z), color);}
  void setPixelColor(const Coord3D &pixel, const CRGB& color) {setPixelColor(XYZ(pixel), color);}

  // temp methods until all effects have been converted to Palette / 2 byte mapping mode
  void setPixelColorPal(int indexV, uint8_t palIndex, uint8_t palBri = 255);
  void setPixelColorPal(const Coord3D &pixel, const uint8_t palIndex, const uint8_t palBri = 255) {setPixelColorPal(XYZ(pixel), palIndex, palBri);}

  void blendPixelColor(int indexV, const CRGB& color, uint8_t blendAmount);
  void blendPixelColor(const Coord3D &pixel, const CRGB& color, const uint8_t blendAmount) {blendPixelColor(XYZ(pixel), color, blendAmount);}

  CRGB getPixelColor(int indexV) const;
  CRGB getPixelColor(int x, int y) {return getPixelColor(XYZ(x, y, 0));} //not const because of XYZ ...
  CRGB getPixelColor(int x, int y, int z) {return getPixelColor(XYZ(x, y, z));}
  CRGB getPixelColor(const Coord3D &pixel) {return getPixelColor(XYZ(pixel));}

  void addPixelColor(int indexV, const CRGB &color) {setPixelColor(indexV, getPixelColor(indexV) + color);}
  void addPixelColor(int x, int y, const CRGB &color) {setPixelColor(XYZ(x, y, 0), getPixelColor(XYZ(x, y, 0)) + color);}
  void addPixelColor(int x, int y, int z, const CRGB &color) {setPixelColor(XYZ(x, y, z), getPixelColor(XYZ(x, y, z)) + color);}
  void addPixelColor(const Coord3D &pixel, const CRGB &color) {setPixelColor(pixel, getPixelColor(pixel) + color);}

  void fadeToBlackBy(uint8_t fadeBy = 255);
  void fill_solid(const CRGB& color);
  void fill_rainbow(uint8_t initialhue, uint8_t deltahue);

  //checks if a virtual pixel is mapped to a physical pixel (use with XY() or XYZ() to get the indexV)
  bool isMapped(int indexV) const {
    return indexV < mappingTableSizeUsed && (mappingTable[indexV].mapType == m_onePixel || mappingTable[indexV].mapType == m_morePixels);
  }

  void blur1d(fract8 blur_amount)
  {
    const uint8_t keep = 255 - blur_amount;
    const uint8_t seep = blur_amount >> 1;
    CRGB carryover = CRGB::Black;
    for( uint16_t i = 0; i < size.x; ++i) {
        CRGB cur = getPixelColor(i);
        CRGB part = cur;
        part.nscale8( seep);
        cur.nscale8( keep);
        cur += carryover;
        if( i) addPixelColor(i-1, part);
        setPixelColor(i, cur);
        carryover = part;
    }
  }

  void blur2d(fract8 blur_amount)
  {
      blurRows(size.x, size.y, blur_amount);
      blurColumns(size.x, size.y, blur_amount);
  }

  void blurRows(uint16_t width, uint16_t height, fract8 blur_amount)
  {
  /*    for (uint16_t row = 0; row < height; row++) {
          CRGB* rowbase = leds + (row * width);
          blur1d( rowbase, width, blur_amount);
      }
  */
      // blur rows same as columns, for irregular matrix
      uint8_t keep = 255 - blur_amount;
      uint8_t seep = blur_amount >> 1;
      for (uint16_t row = 0; row < height; row++) {
          CRGB carryover = CRGB::Black;
          for (uint16_t i = 0; i < width; i++) {
              CRGB cur = getPixelColor(i, row, 0);
              CRGB part = cur;
              part.nscale8( seep);
              cur.nscale8( keep);
              cur += carryover;
              if( i) addPixelColor(i-1, row, 0, part);
              setPixelColor(i, row, 0, cur);
              carryover = part;
          }
      }
  }

  // blurColumns: perform a blur1d on each column of a rectangular matrix
  void blurColumns(uint16_t width, uint16_t height, fract8 blur_amount)
  {
      // blur columns
      uint8_t keep = 255 - blur_amount;
      uint8_t seep = blur_amount >> 1;
      for (uint16_t col = 0; col < width; ++col) {
          CRGB carryover = CRGB::Black;
          for (uint16_t i = 0; i < height; ++i) {
              CRGB cur = getPixelColor(col, i, 0);
              CRGB part = cur;
              part.nscale8( seep);
              cur.nscale8( keep);
              cur += carryover;
              if( i) addPixelColor(col, i-1, 0, part);
              setPixelColor(col, i, 0, cur);
              carryover = part;
          }
      }
  }

  //to do: merge with drawLine to support 2D and 3D
  void drawLine3D(int x1, int y1, int z1, int x2, int y2, int z2, CRGB color, bool soft = false, uint8_t depth = UINT8_MAX)
  {
        // WLEDMM shorten line according to depth
    if (depth < UINT8_MAX) {
      if (depth == 0) return;         // nothing to paint
      if (depth<2) {x2 = x1; y2=y1; z2=z1;} // single pixel
      else {                          // shorten line
        x1 *=2; y1 *=2; z1 *=2; // we do everything "*2" for better rounding
        int dx1 = ((int(2*x2) - int(x1)) * int(depth)) / 255;  // X distance, scaled down by depth 
        int dy1 = ((int(2*y2) - int(y1)) * int(depth)) / 255;  // Y distance, scaled down by depth
        int dz1 = ((int(2*z2) - int(z1)) * int(depth)) / 255;  // Y distance, scaled down by depth
        x1 = (x1 + dx1 +1) / 2;
        y1 = (y1 + dy1 +1) / 2;
        z1 = (z1 + dz1 +1) / 2;
        x1 /=2; y1 /=2; z1 /=2;
      }
    }

    //to do implement soft

    //Bresenham
    setPixelColor(x1, y1, z1, color);
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int dz = abs(z2 - z1);
    int xs;
    int ys;
    int zs;
    if (x2 > x1)
      xs = 1;
    else
      xs = -1;
    if (y2 > y1)
      ys = 1;
    else
      ys = -1;
    if (z2 > z1)
      zs = 1;
    else
      zs = -1;
  
    // Driving axis is X-axis"
    if (dx >= dy && dx >= dz) {
      int p1 = 2 * dy - dx;
      int p2 = 2 * dz - dx;
      while (x1 != x2) {
        x1 += xs;
        if (p1 >= 0) {
          y1 += ys;
          p1 -= 2 * dx;
        }
        if (p2 >= 0) {
          z1 += zs;
          p2 -= 2 * dx;
        }
        p1 += 2 * dy;
        p2 += 2 * dz;
        setPixelColor(x1, y1, z1, color);
      }
  
      // Driving axis is Y-axis"
    }
    else if (dy >= dx && dy >= dz) {
      int p1 = 2 * dx - dy;
      int p2 = 2 * dz - dy;
      while (y1 != y2) {
        y1 += ys;
        if (p1 >= 0) {
          x1 += xs;
          p1 -= 2 * dy;
        }
        if (p2 >= 0) {
          z1 += zs;
          p2 -= 2 * dy;
        }
        p1 += 2 * dx;
        p2 += 2 * dz;
        setPixelColor(x1, y1, z1, color);
      }
  
      // Driving axis is Z-axis"
    }
    else {
      int p1 = 2 * dy - dz;
      int p2 = 2 * dx - dz;
      while (z1 != z2) {
        z1 += zs;
        if (p1 >= 0) {
          y1 += ys;
          p1 -= 2 * dz;
        }
        if (p2 >= 0) {
          x1 += xs;
          p2 -= 2 * dz;
        }
        p1 += 2 * dy;
        p2 += 2 * dx;
        setPixelColor(x1, y1, z1, color);
      }
    }
  }
 
  void drawLine(int x0, int y0, int x1, int y1, CRGB color, bool soft = false, uint8_t depth = UINT8_MAX) {

    // WLEDMM shorten line according to depth
    if (depth < UINT8_MAX) {
      if (depth == 0) return;         // nothing to paint
      if (depth<2) {x1 = x0; y1=y0; } // single pixel
      else {                          // shorten line
        x0 *=2; y0 *=2; // we do everything "*2" for better rounding
        int dx1 = ((int(2*x1) - int(x0)) * int(depth)) / 255;  // X distance, scaled down by depth 
        int dy1 = ((int(2*y1) - int(y0)) * int(depth)) / 255;  // Y distance, scaled down by depth
        x1 = (x0 + dx1 +1) / 2;
        y1 = (y0 + dy1 +1) / 2;
        x0 /=2; y0 /=2;
      }
    }

    const int16_t dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
    const int16_t dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;

    // single pixel (line length == 0)
    if (dx+dy == 0) {
      setPixelColor(x0, y0, 0, color);
      return;
    }

    if (soft) {
      // Xiaolin Wu’s algorithm
      const bool steep = dy > dx;
      if (steep) {
        // we need to go along longest dimension
        std::swap(x0,y0);
        std::swap(x1,y1);
      }
      if (x0 > x1) {
        // we need to go in increasing fashion
        std::swap(x0,x1);
        std::swap(y0,y1);
      }
      float gradient = x1-x0 == 0 ? 1.0f : float(y1-y0) / float(x1-x0);
      float intersectY = y0;
      for (int x = x0; x <= x1; x++) {
        unsigned keep = float(0xFFFF) * (intersectY-int(intersectY)); // how much color to keep
        unsigned seep = 0xFFFF - keep; // how much background to keep
        int y = int(intersectY);
        if (steep) std::swap(x,y);  // temporarily swap if steep
        // pixel coverage is determined by fractional part of y co-ordinate
        // WLEDMM added out-of-bounds check: "unsigned(x) < cols" catches negative numbers _and_ too large values
        setPixelColor(x, y, 0, blend(color, getPixelColor(x, y, 0), keep));
        int xx = x+int(steep);
        int yy = y+int(!steep);
        setPixelColor(xx, yy, 0, blend(color, getPixelColor(xx, yy, 0), seep));
      
        intersectY += gradient;
        if (steep) std::swap(x,y);  // restore if steep
      }
    } else {
      // Bresenham's algorithm
      int err = (dx>dy ? dx : -dy)/2;   // error direction
      for (;;) {
        // if (x0 >= cols || y0 >= rows) break; // WLEDMM we hit the edge - should never happen
        setPixelColor(x0, y0, 0, color);
        if (x0==x1 && y0==y1) break;
        int e2 = err;
        if (e2 >-dx) { err -= dy; x0 += sx; }
        if (e2 < dy) { err += dx; y0 += sy; }
      }
    }
  }

  void drawCircle(int cx, int cy, uint8_t radius, CRGB col, bool soft) {
    if (radius == 0) return;
    if (soft) {
      // Xiaolin Wu’s algorithm
      int rsq = radius*radius;
      int x = 0;
      int y = radius;
      unsigned oldFade = 0;
      while (x < y) {
        float yf = sqrtf(float(rsq - x*x)); // needs to be floating point
        unsigned fade = float(0xFFFF) * (ceilf(yf) - yf); // how much color to keep
        if (oldFade > fade) y--;
        oldFade = fade;
        setPixelColor(XY(cx+x, cy+y), blend(col, getPixelColor(XY(cx+x, cy+y)), fade));
        setPixelColor(XY(cx-x, cy+y), blend(col, getPixelColor(XY(cx-x, cy+y)), fade));
        setPixelColor(XY(cx+x, cy-y), blend(col, getPixelColor(XY(cx+x, cy-y)), fade));
        setPixelColor(XY(cx-x, cy-y), blend(col, getPixelColor(XY(cx-x, cy-y)), fade));
        setPixelColor(XY(cx+y, cy+x), blend(col, getPixelColor(XY(cx+y, cy+x)), fade));
        setPixelColor(XY(cx-y, cy+x), blend(col, getPixelColor(XY(cx-y, cy+x)), fade));
        setPixelColor(XY(cx+y, cy-x), blend(col, getPixelColor(XY(cx+y, cy-x)), fade));
        setPixelColor(XY(cx-y, cy-x), blend(col, getPixelColor(XY(cx-y, cy-x)), fade));
        setPixelColor(XY(cx+x, cy+y-1), blend(getPixelColor(XY(cx+x, cy+y-1)), col, fade));
        setPixelColor(XY(cx-x, cy+y-1), blend(getPixelColor(XY(cx-x, cy+y-1)), col, fade));
        setPixelColor(XY(cx+x, cy-y+1), blend(getPixelColor(XY(cx+x, cy-y+1)), col, fade));
        setPixelColor(XY(cx-x, cy-y+1), blend(getPixelColor(XY(cx-x, cy-y+1)), col, fade));
        setPixelColor(XY(cx+y-1, cy+x), blend(getPixelColor(XY(cx+y-1, cy+x)), col, fade));
        setPixelColor(XY(cx-y+1, cy+x), blend(getPixelColor(XY(cx-y+1, cy+x)), col, fade));
        setPixelColor(XY(cx+y-1, cy-x), blend(getPixelColor(XY(cx+y-1, cy-x)), col, fade));
        setPixelColor(XY(cx-y+1, cy-x), blend(getPixelColor(XY(cx-y+1, cy-x)), col, fade));
        x++;
      }
    } else {
      // Bresenham’s Algorithm
      int d = 3 - (2*radius);
      int y = radius, x = 0;
      while (y >= x) {
        setPixelColor(XY(cx+x, cy+y), col);
        setPixelColor(XY(cx-x, cy+y), col);
        setPixelColor(XY(cx+x, cy-y), col);
        setPixelColor(XY(cx-x, cy-y), col);
        setPixelColor(XY(cx+y, cy+x), col);
        setPixelColor(XY(cx-y, cy+x), col);
        setPixelColor(XY(cx+y, cy-x), col);
        setPixelColor(XY(cx-y, cy-x), col);
        x++;
        if (d > 0) {
          y--;
          d += 4 * (x - y) + 10;
        } else {
          d += 4 * x + 6;
        }
      }
    }
  }

  //shift is used by drawText indicating which letter it is drawing
  void drawCharacter(unsigned char chr, int x = 0, int y = 0, uint8_t font = 0, CRGB col = CRGB::Red, uint16_t shiftPixel = 0, uint16_t shiftChr = 0);

  void drawText(const char * text, int x = 0, int y = 0, uint8_t font = 0, CRGB col = CRGB::Red, uint16_t shiftPixel = 0) {
    const int numberOfChr = text?strnlen(text, 256):0; //max 256 charcters
    for (int shiftChr = 0; shiftChr < numberOfChr; shiftChr++) {
      drawCharacter(text[shiftChr], x, y, font, col, shiftPixel, shiftChr);
    }
  }

  void addPixelsPre(uint8_t rowNr);
  void addPixel(Coord3D pixel, uint8_t rowNr);
  void addPixelsPost(uint8_t rowNr);

}; //LedsLayer

//     sin8/cos8   sin16/cos16
//0:   128, 255    0 32645
//64:  255, 128    32645 0
//128: 128, 1      0 -32645
//192: 1, 127      -32645 0

static unsigned trigoCached = 1;
static unsigned trigoUnCached = 1;

struct Trigo {
  virtual ~Trigo() = default;

  uint16_t period = 360; //default period 360
  Trigo(uint16_t period = 360) {this->period = period;}
  float sinValue[3]; uint16_t sinAngle[3] = {UINT16_MAX,UINT16_MAX,UINT16_MAX}; //caching of sinValue=sin(sinAngle) for tilt, pan and roll
  float cosValue[3]; uint16_t cosAngle[3] = {UINT16_MAX,UINT16_MAX,UINT16_MAX}; //caching of cosValue=cos(cosAngle) for tilt, pan and roll
  virtual float sinBase(uint16_t angle) {return sinf(M_TWOPI * angle / period);}
  virtual float cosBase(uint16_t angle) {return cosf(M_TWOPI * angle / period);}
  int16_t sin(int16_t factor, uint16_t angle, uint8_t cache012 = 0) {
    if (sinAngle[cache012] != angle) {sinAngle[cache012] = angle; sinValue[cache012] = sinBase(angle);trigoUnCached++;} else trigoCached++;
    return factor * sinValue[cache012];
  };
  int16_t cos(int16_t factor, uint16_t angle, uint8_t cache012 = 0) {
    if (cosAngle[cache012] != angle) {cosAngle[cache012] = angle; cosValue[cache012] = cosBase(angle);trigoUnCached++;} else trigoCached++;
    return factor * cosValue[cache012];
  };
  // https://msl.cs.uiuc.edu/planning/node102.html
  Coord3D pan(Coord3D in, Coord3D middle, uint16_t angle) {
    Coord3D inM = in - middle;
    Coord3D out;
    out.x = cos(inM.x, angle, 0) + sin(inM.z, angle, 0);
    out.y = inM.y;
    out.z = - sin(inM.x, angle, 0) + cos(inM.z, angle, 0);
    return out + middle;
  }
  Coord3D tilt(Coord3D in, Coord3D middle, uint16_t angle) {
    Coord3D inM = in - middle;
    Coord3D out;
    out.x = inM.x;
    out.y = cos(inM.y, angle, 1) - sin(inM.z, angle, 1);
    out.z = sin(inM.y, angle, 1) + cos(inM.z, angle, 1);
    return out + middle;
  }
  Coord3D roll(Coord3D in, Coord3D middle, uint16_t angle) {
    Coord3D inM = in - middle;
    Coord3D out;
    out.x = cos(inM.x, angle, 2) - sin(inM.y, angle, 2);
    out.y = sin(inM.x, angle, 2) + cos(inM.y, angle, 2);
    out.z = inM.z;
    return out + middle;
  }
  Coord3D rotate(Coord3D in, Coord3D middle, uint16_t tiltAngle, uint16_t panAngle, uint16_t rollAngle, uint16_t period = 360) {
    this->period = period;
    return roll(pan(tilt(in, middle, tiltAngle), middle, panAngle), middle, rollAngle);
  }
};

struct Trigo8: Trigo { //FastLed sin8 and cos8
  using Trigo::Trigo;
  float sinBase(uint16_t angle) override {return (sin8(256.0f * angle / period) - 128) / 127.0f;}
  float cosBase(uint16_t angle) override {return (cos8(256.0f * angle / period) - 128) / 127.0f;}
};
struct Trigo16: Trigo { //FastLed sin16 and cos16
  using Trigo::Trigo;
  float sinBase(uint16_t angle) override {return sin16(65536.0f * angle / period) / 32645.0f;}
  float cosBase(uint16_t angle) override {return cos16(65536.0f * angle / period) / 32645.0f;}
};

static Trigo trigoTiltPanRoll(255); // Trigo8 is hardly any faster (27 vs 28 fps) (spanXY=28)