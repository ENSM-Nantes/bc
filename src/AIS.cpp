/*   BridgeCommand 5.7 Copyright (C) James Packer
     This file is Copyright (C) 2022 Fraunhofer FKIE

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License version 2 as
     published by the Free Software Foundation

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY Or FITNESS For A PARTICULAR PURPOSE.  See the
     GNU General Public License For more details.

     You should have received a copy of the GNU General Public License along
     with this program; if not, write to the Free Software Foundation, Inc.,
     51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA. */

#include <cassert>
#include <tuple>
#include "AIS.hpp"
#include "Constants.hpp"
#include "OtherShips.hpp"
#include "Terrain.hpp"
#include <iostream>


AIS::AIS()
{

}

AIS::~AIS()
{


}

void AIS::Init(unsigned int aNumberShip)
{
  for(unsigned int i = 0; i<aNumberShip; i++)
    {
      mLastUpdates.push_back(i*1000);
    }
}

std::vector<unsigned int> AIS::GetReadyShips(void *aOtherShips, unsigned int aNow)
{
  OtherShips *pOtherShips = (OtherShips*)aOtherShips;
  unsigned int elapsedTime = 0, reportingInterval = 0;
  float shipSpeed = 0;
  mReadyShips.clear();

  for(unsigned int ship=0; ship < mLastUpdates.size(); ship++)
    {
      if(aNow > mLastUpdates[ship]) elapsedTime = aNow - mLastUpdates[ship];
      else elapsedTime = 0;
    
      shipSpeed = pOtherShips->getSpeed(ship);

      if(shipSpeed >=0 && shipSpeed < SPEED_MAX_INTERVAL_1) reportingInterval = TIME_INTERVAL_1;
      else if(shipSpeed >=SPEED_MAX_INTERVAL_2 && shipSpeed < SPEED_MAX_INTERVAL_2) reportingInterval = TIME_INTERVAL_2;
      else reportingInterval = TIME_INTERVAL_3;
      
      if(elapsedTime >= reportingInterval)
	{
	  mLastUpdates[ship] = aNow;
	  mReadyShips.push_back(ship);
	}
  }
  return mReadyShips;
}

std::string AIS::GenerateClassAReport(void *aOtherShips, float aOffsetPosZ, float aOffsetPosX, void *aTerrain, unsigned long long aTimeStamp, unsigned int aShip)
{
  std::vector<bool> classAReport(168, 0);
  OtherShips *pOtherShips = (OtherShips*)aOtherShips;
  Terrain *pTerrain = (Terrain*)aTerrain;
  bool done = false;
  unsigned int heading = (unsigned int) (pOtherShips->getHeading(aShip) * 180/PI);
  unsigned int mmsi = pOtherShips->getMMSI(aShip);
  unsigned int speed = std::min<int>((int) 10.0f * MPS_TO_KTS * pOtherShips->getSpeed(aShip), 1022);
  float posX=0, posZ=0;

  posX = pOtherShips->getPosition(aShip).X + aOffsetPosX;
  posZ = pOtherShips->getPosition(aShip).Z + aOffsetPosZ;
  
  float shipLong = pTerrain->xToLong(posX);
  float shipLat  = pTerrain->zToLat(posZ);

  unsigned int timestamp = aTimeStamp % 60;


  // fill class A report fields
    
  // 0-5: message type, set to 0b000001 for normal class A position report
  classAReport[5] = 1;
    
  // 6-7 repeat indicator, set to 0b11 to signify do not repeat
  classAReport[6] = 1;
  classAReport[7] = 1;

  // 8-37 MMSI, 9-decimal digit in 30 bit field
  for (int i=0; i < 30; i++) {
    classAReport[8 + 29 - i] = mmsi % 2;
    mmsi >>= 1;
  }

  // 38-41 navigation status
  // set to 0b0000 for underway using engine
  classAReport[38] = 0;
  classAReport[39] = 0;
  classAReport[40] = 0;
  classAReport[41] = 0;
  if (speed == 0) {
    // if not moving, set to 0b0001 for anchored
    classAReport[41] = 1;
  }

  // 42-49 rate of turn, set to 0x80 for no turn information available
  // TODO: add rate of turn of other ships 
  classAReport[42] = 1;
  for (int i=43; i <= 49; i++) {
    classAReport[i] = 0;
  }

  // 50-59 speed over ground, 10 bit field
  for (int i=0; i < 10; i++) {
    classAReport[50 + 9 - i] = speed % 2;
    speed >>= 1;
  }

  // 60 position accuracy, set to 0b1 to indicate DGPS-quality fix, since
  // shipLong and shipLat have 5 decimals giving a 1m resolution.
  classAReport[60] = 1;

  // 61-88 longitude in a 28-bit field encoding a signed integer representing a float with a
  // resolution of 0.0001 corresponding to the longitude in minutes
  int longitude = (int) 600000.0f * shipLong;
  bool longIsNeg = longitude < 0;
  for (int i=0; i < 28; i++) {
    classAReport[61 + 27 - i] = longitude % 2;
    longitude >>= 1;
  }
  if (longIsNeg) classAReport[61] = 1; // set the sign bit
    
  // 89-115 latitude in a 27-bit field encoding a signed integer representing a float with a
  // resolution of 0.0001 corresponding to the latitude in minutes
  int latitude = (int) 600000.0f * shipLat;
  bool latIsNeg = latitude < 0;
  for (int i=0; i < 27; i++) {
    classAReport[89 + 26 - i] = latitude % 2;
    latitude >>= 1;
  }
  if (latIsNeg) classAReport[89] = 1; // set the sign bit

  // 116-127 course over ground, 12 bit field, unsigned int representing a float with
  // a resolution of 0.1 corresponding to the course over ground in degrees relative to true north
  unsigned int cog = 10 * heading;
  for (int i=0; i < 12; i++) {
    classAReport[116 + 11 - i] = cog % 2;
    cog >>= 1;
  }

  // 128-136 true heading, 9 bit field, unsigned int
  for (int i=0; i < 9; i++) {
    classAReport[128 + 8 - i] = heading % 2;
    heading >>= 1;
  }

  // 137-142 timestamp, 6 bit field, unsigned int corresponding to the seconds of current UTC time
  for (int i=0; i < 6; i++) {
    classAReport[137 + 5 - i] = timestamp % 2;
    timestamp >>= 1;
  }

  // 143-144 maneuver indicator, set to 0b00 for no special maneuver
  classAReport[143] = 0;
  classAReport[144] = 1;

  // 145-147 not used
    
  // 148 RAIM flag, set to 0b0 for unset
  classAReport[148] = 0;

  // 149-167 radio status, 19 bit field, unsigned integer for radio diagnostic, leave as 0 for now
    
  // convert bit sequence to armored ASCII
  // number of bits we need to append to get the payload length to a multiple of 6
  // always 0 since we always generate a class A Report of length 168
  // int fillBits = (6 - (168 % 6)) % 6;
  return BitsToArmoredASCII(classAReport);
}

std::string AIS::BitsToArmoredASCII(std::vector<bool> aBits) {
  // must be called with padded payload!
  assert(aBits.size() % 6 == 0);

  int counter = 0;

  std::string payload(aBits.size() / 6, 0);
  int index = 0;

  for (int i=0; i < aBits.size(); i++) {
    payload[index] <<= 1;
    payload[index] |= aBits[i];
    counter += 1;
        
    if (counter % 6 == 0) {
      counter = 0;
      payload[index] += 48;

      if (payload[index] >= 88) {
	payload[index] += 8;
      }
      index += 1;
    }
  }
  return payload;
}
