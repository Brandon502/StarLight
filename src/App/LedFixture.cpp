/*
   @title     StarLight
   @file      LedFixture.cpp
   @date      20240228
   @repo      https://github.com/MoonModules/StarLight
   @Authors   https://github.com/MoonModules/StarLight/commits/main
   @Copyright © 2024 Github StarLight Commit Authors
   @license   GNU GENERAL PUBLIC LICENSE Version 3, 29 June 2007
   @license   For non GPL-v3 usage, commercial licenses must be purchased. Contact moonmodules@icloud.com
*/

#include "LedFixture.h"

#include "../Sys/SysModFiles.h"
#include "../Sys/SysStarJson.h"
#include "../Sys/SysModPins.h"


//load fixture json file, parse it and depending on the projection, create a mapping for it
void Fixture::projectAndMap() {
  unsigned long start = millis();
  char fileName[32] = "";

  if (files->seqNrToName(fileName, fixtureNr)) { // get the fixture.json
    StarJson starJson(fileName); //open fileName for deserialize

    // reset leds
    stackUnsigned8 rowNr = 0;
    for (Leds *leds: listOfLeds) {
      if (leds->doMap) {
        leds->fill_solid(CRGB::Black, true); //no blend
        leds->reverseTranform = 0;

        ppf("projectAndMap clear leds[%d] fx:%d pro:%d\n", rowNr, leds->fx, leds->projectionNr);
        leds->size = Coord3D{0,0,0};
        //vectors really gone now?
        for (PhysMap &map:leds->mappingTable) {
          if (map.isMultipleIndexes()) {
            map.indexes->clear();
            delete map.indexes;
          }
        }
        leds->mappingTable.clear();
        // leds->sharedData.reset(); //do not reset as want to save settings.
      }
      rowNr++;
    }

    //deallocate all led pins
    if (doAllocPins) {
      stackUnsigned8 pinNr = 0;
      for (PinObject &pinObject: pins->pinObjects) {
        if (strcmp(pinObject.owner, "Leds") == 0)
          pins->deallocatePin(pinNr, "Leds");
        pinNr++;
      }
    }

    stackUnsigned16 indexP = 0;
    stackUnsigned16 prevIndexP = 0;
    unsigned16 currPin; //lookFor needs u16

    //what to deserialize
    starJson.lookFor("width", (unsigned16 *)&fixSize.x);
    starJson.lookFor("height", (unsigned16 *)&fixSize.y);
    starJson.lookFor("depth", (unsigned16 *)&fixSize.z);
    starJson.lookFor("nrOfLeds", &nrOfLeds);
    starJson.lookFor("pin", &currPin);

    //lookFor leds array and for each item in array call lambdo to make a projection
    starJson.lookFor("leds", [this, &prevIndexP, &indexP, &currPin](std::vector<unsigned16> uint16CollectList) { //this will be called for each tuple of coordinates!

      if (uint16CollectList.size()>=1) { // process one pixel

        Coord3D pixel; //in mm !
        pixel.x = uint16CollectList[0];
        pixel.y = (uint16CollectList.size()>=2)?uint16CollectList[1]: 0;
        pixel.z = (uint16CollectList.size()>=3)?uint16CollectList[2]: 0;

        // ppf("led %d,%d,%d start %d,%d,%d end %d,%d,%d\n",x,y,z, startPos.x, startPos.y, startPos.z, endPos.x, endPos.y, endPos.z);

        stackUnsigned8 rowNr = 0;
        for (Leds *leds: listOfLeds) {

          if (leds->projectionNr != p_Random && leds->projectionNr != p_None) //only real projections
          if (leds->doMap) { //add pixel in leds mappingtable

            //set start and endPos between bounderies of fixture
            Coord3D startPosAdjusted = (leds->startPos).minimum(fixSize - Coord3D{1,1,1}) * 10;
            Coord3D endPosAdjusted = (leds->endPos).minimum(fixSize - Coord3D{1,1,1}) * 10;
            Coord3D midPosAdjusted = (leds->midPos).minimum(fixSize - Coord3D{1,1,1}); //not * 10

            // mdl->setValue("ledsStart", startPosAdjusted/10, rowNr); //rowNr
            // mdl->setValue("ledsEnd", endPosAdjusted/10, rowNr); //rowNr

            if (pixel >= startPosAdjusted && pixel <= endPosAdjusted ) { //if pixel between start and end pos

              Coord3D pixelAdjusted = (pixel - startPosAdjusted)/10; //pixelRelative to startPos in cm

              Coord3D sizeAdjusted = (endPosAdjusted - startPosAdjusted)/10 + Coord3D{1,1,1}; // in cm

              // 0 to 3D depending on start and endpos (e.g. to display ScrollingText on one side of a cube)
              leds->projectionDimension = 0;
              if (sizeAdjusted.x > 1) leds->projectionDimension++;
              if (sizeAdjusted.y > 1) leds->projectionDimension++;
              if (sizeAdjusted.z > 1) leds->projectionDimension++;

              Projection *projection = nullptr;
              if (leds->projectionNr < projections.size())
                projection = projections[leds->projectionNr];

              mdl->getValueRowNr = rowNr; //run projection functions in the right rowNr context

              //using cached virtual class methods!
              // Only call adjustSizeAndPixelCached once if leds->size = 0,0,0? Not sure if other projections will need to adjust multiple times.
              // if (projection && leds->size == Coord3D({0,0,0})) 
              (projection->*leds->adjustSizeAndPixelCached)(*leds, sizeAdjusted, pixelAdjusted, midPosAdjusted);
              
              if (leds->size == Coord3D{0,0,0}) { // first
                ppf("projectAndMap first leds[%d] size:%d,%d,%d s:%d,%d,%d e:%d,%d,%d\n", rowNr, sizeAdjusted.x, sizeAdjusted.y, sizeAdjusted.z, startPosAdjusted.x, startPosAdjusted.y, startPosAdjusted.z, endPosAdjusted.x, endPosAdjusted.y, endPosAdjusted.z);
                leds->size = sizeAdjusted;
              }

              //calculate the indexV to add to current physical led to
              stackUnsigned16 indexV = UINT16_MAX;

              Coord3D mapped;

              //using cached virtual class methods!
              if (projection) (projection->*leds->adjustMappedCached)(*leds, mapped, sizeAdjusted, pixelAdjusted, midPosAdjusted);
              indexV = leds->XYZUnprojected(mapped);
                  
              leds->nrOfLeds = leds->size.x * leds->size.y * leds->size.z;

              if (indexV != UINT16_MAX) {
                if (indexV >= leds->nrOfLeds || indexV >= NUM_VLEDS_Max) {
                  ppf("dev pre [%d] indexV too high %d>=%d or %d (m:%d p:%d) p:%d,%d,%d s:%d,%d,%d\n", rowNr, indexV, leds->nrOfLeds, NUM_VLEDS_Max, leds->mappingTable.size(), indexP, pixel.x, pixel.y, pixel.z, leds->size.x, leds->size.y, leds->size.z);
                }
                else {
                  Trigo trigo(leds->size.x-1); // 8 bits trigo with period leds->size.x-1 (currentl Float trigo as same performance)
                  //post processing: 
                  switch(leds->projectionNr) {
                  case p_DistanceFromPoint:
                    switch (leds->effectDimension) {
                    case _2D: 
                      switch (leds->projectionDimension) {
                      case _2D: //2D2D: inverse mapping
                        float minDistance = 10;
                        // ppf("checking indexV %d\n", indexV);
                        for (forUnsigned16 x=0; x<leds->size.x && minDistance > 0.5f; x++) {
                          // float xFactor = x * TWO_PI / (float)(leds->size.x-1); //between 0 .. 2PI

                          float xNew = trigo.sin(leds->size.x, x);
                          float yNew = trigo.cos(leds->size.y, x);

                          for (forUnsigned16 y=0; y<leds->size.y && minDistance > 0.5f; y++) {

                            // float yFactor = (leds->size.y-1.0f-y) / (leds->size.y-1.0f); // between 1 .. 0
                            float yFactor = 1 - y / (leds->size.y-1.0f); // between 1 .. 0

                            float x2New = round((yFactor * xNew + leds->size.x) / 2.0f); // 0 .. size.x
                            float y2New = round((yFactor * yNew + leds->size.y) / 2.0f); //  0 .. size.y

                            // ppf(" %d,%d->%f,%f->%f,%f", x, y, sinf(x * TWO_PI / (float)(size.x-1)), cosf(x * TWO_PI / (float)(size.x-1)), xNew, yNew);

                            //this should work (better) but needs more testing
                            // float distance = abs(indexV - xNew - yNew * size.x);
                            // if (distance < minDistance) {
                            //   minDistance = distance;
                            //   indexV = x+y*size.x;
                            // }

                            // if the new XY i
                            if (indexV == leds->XY(x2New, y2New)) { //(unsigned8)xNew + (unsigned8)yNew * size.x) {
                              // ppf("  found one %d => %d=%d+%d*%d (%f+%f*%d) [%f]\n", indexV, x+y*size.x, x,y, size.x, xNew, yNew, size.x, distance);
                              indexV = leds->XY(x, y);

                              if (indexV%10 == 0) ppf("."); //show some progress as this projection is slow (Need S007 to optimize ;-)
                                                          
                              minDistance = 0.0f; // stop looking further
                            }
                          }
                        }
                        if (minDistance > 0.5f) indexV = UINT16_MAX;
                        break;
                      }
                      break;
                    }
                    break;
                  }

                  if (indexV != UINT16_MAX) { //can be nulled by inverse mapping 
                    //add physical tables if not present
                    if (indexV >= leds->nrOfLeds || indexV >= NUM_VLEDS_Max) {
                      ppf("dev post [%d] indexV too high %d>=%d or %d (p:%d m:%d) p:%d,%d,%d\n", rowNr, indexV, leds->nrOfLeds, NUM_VLEDS_Max, leds->mappingTable.size(), indexP, pixel.x, pixel.y, pixel.z);
                    }
                    else if (indexP < NUM_LEDS_Max) {
                      //create new physMaps if needed
                      if (indexV >= leds->mappingTable.size()) {
                        for (size_t i = leds->mappingTable.size(); i <= indexV; i++) {
                          // ppf("mapping %d,%d,%d add physMap before %d %d\n", pixel.y, pixel.y, pixel.z, indexV, leds->mappingTable.size());
                          leds->mappingTable.push_back(PhysMap()); //abort() was called at PC 0x40191473 on core 1 std::allocator<unsigned short> >&&)
                        }
                      }

                      leds->mappingTable[indexV].addIndexP(indexP);
                    }
                    else 
                      ppf("dev post [%d] indexP too high %d>=%d or %d (p:%d m:%d) p:%d,%d,%d\n", rowNr, indexP, nrOfLeds, NUM_LEDS_Max, leds->mappingTable.size(), indexP, pixel.x, pixel.y, pixel.z);
                  }
                  // ppf("mapping b:%d t:%d V:%d\n", indexV, indexP, leds->mappingTable.size());
                } //indexV not too high
              } //indexV

              mdl->getValueRowNr = UINT8_MAX; // end of run projection functions in the right rowNr context

            } //if x,y,z between start and endpos
          } //if leds->doMap
          rowNr++;
        } //for listOfLeds
        indexP++; //also increase if no buffer created
      } //if 1D-3D pixel

      else { // end of leds array

        if (doAllocPins) {
          //check if pin already allocated, if so, extend range in details
          PinObject pinObject = pins->pinObjects[currPin];
          char details[32] = "";
          if (pins->isOwner(currPin, "Leds")) { //if owner

            char * after = strtok((char *)pinObject.details, "-");
            if (after != NULL ) {
              char * before;
              before = after;
              after = strtok(NULL, " ");
              stackUnsigned16 startLed = atoi(before);
              stackUnsigned16 nrOfLeds = atoi(after) - atoi(before) + 1;
              print->fFormat(details, sizeof(details)-1, "%d-%d", min(prevIndexP, startLed), max((stackUnsigned16)(indexP - 1), nrOfLeds)); //careful: LedModEffects:loop uses this to assign to FastLed
              ppf("pins extend leds %d: %s\n", currPin, details);
              //tbd: more check

              strncpy(pins->pinObjects[currPin].details, details, sizeof(PinObject::details)-1);  
              pins->pinsChanged = true;
            }
          }
          else {//allocate new pin
            //tbd: check if free
            print->fFormat(details, sizeof(details)-1, "%d-%d", prevIndexP, indexP - 1); //careful: LedModEffects:loop uses this to assign to FastLed
            // ppf("allocatePin %d: %s\n", currPin, details);
            pins->allocatePin(currPin, "Leds", details);
          }

          prevIndexP = indexP;
        }
      }
    }); //starJson.lookFor("leds" (create the right type, otherwise crash)

    if (starJson.deserialize()) { //this will call above function parameter for each led

      //after processing each led
      stackUnsigned8 rowNr = 0;

      for (Leds *leds: listOfLeds) {
        if (leds->doMap) {
          ppf("projectAndMap post leds[%d] fx:%d pro:%d\n", rowNr, leds->fx, leds->projectionNr);

          stackUnsigned16 nrOfMappings = 0;
          stackUnsigned16 nrOfPixels = 0;

          if (leds->projectionNr == p_Random || leds->projectionNr == p_None) {

            //defaults
            leds->size = fixSize;
            leds->nrOfLeds = nrOfLeds;
            nrOfPixels = nrOfLeds;

          } else {

            // if (leds->mappingTable.size() < leds->size.x * leds->size.y * leds->size.z)
            //   ppf("mapping add extra physMap %d to %d size: %d,%d,%d\n", leds->mappingTable.size(), leds->size.x * leds->size.y * leds->size.z, leds->size.x, leds->size.y, leds->size.z);
            // for (size_t i = leds->mappingTable.size(); i < leds->size.x * leds->size.y * leds->size.z; i++) {
            //   std::vector<unsigned16> physMap;
            //   // physMap.push_back(0);
            //   leds->mappingTable.push_back(physMap);
            // }

            leds->nrOfLeds = leds->mappingTable.size();

            //debug info + summary values
            stackUnsigned16 indexV = 0;
            for (PhysMap &map:leds->mappingTable) {
            // for (auto map=leds->mappingTable.begin(); map!=leds->mappingTable.end(); ++map) {
              if (map.isOneIndex()) {
                  nrOfPixels++;
              }
              else if (map.isMultipleIndexes()) { // && map.indexes->size()
                // if (nrOfMappings < 10 || map.indexes->size() - indexV < 10) //first 10 and last 10
                // if (nrOfMappings%(leds->nrOfLeds/10+1) == 0)
                  // ppf("ledV %d mapping: #ledsP (%d):", indexV, nrOfMappings);

                for (forUnsigned16 indexP:*map.indexes) {
                  // if (nrOfPixels < 10 || map.indexes->size() - indexV < 10)
                  // if (nrOfMappings%(leds->nrOfLeds/10+1) == 0)
                    // ppf(" %d", indexP);
                  nrOfPixels++;
                }

                // if (nrOfPixels < 10 || map.indexes->size() - indexV < 10)
                // if (nrOfMappings%(leds->nrOfLeds/10+1) == 0)
                  // ppf("\n");
              }
              nrOfMappings++;
              // else
              //   ppf("ledV %d no mapping\n", x);
              indexV++;
            }
          }

          ppf("projectAndMap leds[%d] V:%d x %d x %d -> %d (v:%d - p:%d)\n", rowNr, leds->size.x, leds->size.y, leds->size.z, leds->nrOfLeds, nrOfMappings, nrOfPixels);

          // mdl->setValueV("ledsSize", rowNr, "%d x %d x %d = %d", leds->size.x, leds->size.y, leds->size.z, leds->nrOfLeds);
          char buf[32];
          print->fFormat(buf, sizeof(buf)-1,"%d x %d x %d -> %d", leds->size.x, leds->size.y, leds->size.z, leds->nrOfLeds);
          mdl->setValue("ledsSize", JsonString(buf, JsonString::Copied), rowNr);
          // web->sendResponseObject();

          ppf("projectAndMap leds[%d].size = %d + %d\n", rowNr, sizeof(Leds), leds->mappingTable.size()); //44

          leds->doMap = false;
        } //leds->doMap
        rowNr++;
      } // leds

      ppf("projectAndMap fixture P:%dx%dx%d -> %d\n", fixSize.x, fixSize.y, fixSize.z, nrOfLeds);

      mdl->setValue("fixSize", fixSize);
      mdl->setValue("fixCount", nrOfLeds);

    } // if deserialize
  } //if fileName
  else
    ppf("projectAndMap: Filename for fixture %d not found\n", fixtureNr);

  doMap = false;
  ppf("projectAndMap done %d ms\n", millis()-start);
}