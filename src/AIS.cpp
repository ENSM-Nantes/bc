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
#include <algorithm>

AIS::AIS()
{

}

AIS::~AIS()
{


}


unsigned char AIS::AsciiToAis6(char aChar)
{
    if(aChar >= '@' && aChar <= '_')
      {
        return aChar - 64;
      }
    else if(aChar >= ' ' && aChar <= '?')
      {
        return aChar;
      }
    else
      {
        return 0; //@ in AIS ASCII 6 bits
      }
}

std::vector<unsigned char> AIS::StringToAis6(const std::string& aStrIn)
{
    std::vector<unsigned char> strOut;

    for(char c : aStrIn)
      {
        strOut.push_back(AsciiToAis6(c));
      }

    return strOut;
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
      else if(shipSpeed >=SPEED_MAX_INTERVAL_1 && shipSpeed < SPEED_MAX_INTERVAL_2) reportingInterval = TIME_INTERVAL_2;
      else reportingInterval = TIME_INTERVAL_3;
      
      if(elapsedTime >= reportingInterval)
	{
	  mLastUpdates[ship] = aNow;
	  mReadyShips.push_back(ship);
	}
  }
  return mReadyShips;
}

std::string AIS::GenerateMessage5(void *aOtherShips, unsigned int aShip)
{
  std::vector<bool> classAReport(426, 0);
  OtherShips *pOtherShips = (OtherShips*)aOtherShips;
  unsigned int mmsi = pOtherShips->getMMSI(aShip);
  unsigned int imo = pOtherShips->getIMO(aShip);
  unsigned char type = pOtherShips->getType(aShip);
  std::string callSignStr = pOtherShips->getCallSign(aShip);
  std::string shipNameStr = pOtherShips->getShipName(aShip);
  std::string shipDestStr = pOtherShips->getShipDest(aShip);
  std::vector<unsigned char> callSignAIS = {0};
  std::vector<unsigned char> shipNameAIS = {0};
  std::vector<unsigned char> shipDestAIS = {0};
  unsigned int idx = 0;
  float lpp = pOtherShips->getLength(aShip);
  float breadth = pOtherShips->getBreadth(aShip);
  float draught = pOtherShips->getDraught(aShip);
  unsigned char draughtAIS = 0;
  
  // fill class A report fields    
  // 0-5: message type, set to 0b000101 for normal class A position report
  classAReport[3] = 1;
  classAReport[5] = 1;
    
  // 6-7 repeat indicator, set to 0b11 to signify do not repeat
  classAReport[6] = 1;
  classAReport[7] = 1;

  // 8-37 MMSI, 9-decimal digit in 30 bit field
  for(int i=0; i<30; i++)
    {
      classAReport[8 + 29 - i] = mmsi % 2;
      mmsi >>= 1;
    }

  // 38-39 AIS version indicator
  classAReport[38] = 1;
  classAReport[39] = 1;

  //40-69 IMO number 10-decimal digit in 30 bit field
  for(int i=0; i<30; i++)
    {
      classAReport[40 + 29 - i] = imo % 2;
      imo >>= 1;
    }

  //70-111 Call Sign  
  callSignAIS = AIS::StringToAis6(callSignStr);  
  idx = 70;
  for(int k=0; k<callSignAIS.size(); k++)
    {
      for(int j=5;j>=0;j--)
	{
	  classAReport[idx] = callSignAIS[k] & (1 << j);
	  idx++;
	}
    }

  //112-231 Name  
  shipNameAIS = AIS::StringToAis6(shipNameStr);  
  idx = 112;
  for(int k=0; k<shipNameAIS.size(); k++)
    {
      for(int j=5;j>=0;j--)
	{
	  classAReport[idx] = shipNameAIS[k] & (1 << j);
	  idx++;
	}
    }

  //232-239 Type of ship
  for(int i=0; i<8; i++)
    {
      classAReport[232 + 7 - i] = type % 2;
      type >>= 1;
    }

  //240-269 Overall dimension
  unsigned short A = lpp/2;
  unsigned short B = A;
  unsigned short C = breadth/2;
  unsigned short D = C;
  
  for(int i=0; i<9; i++)
    {
      classAReport[240 + 8 - i] =  A % 2;
      A >>= 1;
    }
  for(int i=0; i<9; i++)
    {
      classAReport[249 + 8 - i] =  B % 2;
      B >>= 1;
    }
  for(int i=0; i<6; i++)
    {
      classAReport[258 + 5 - i] =  C % 2;
      C >>= 1;
    }
  for(int i=0; i<6; i++)
    {
      classAReport[264 + 5 - i] =  D % 2;
      D >>= 1;
    }

  //270-273 Type of electronic
  classAReport[273] = 1; //GPS forced

  //274-293 ETA
  //MM 0 - default - 4bits
  //DD 0 - default - 5 bits
  classAReport[283] = 1; //HH 24 - default
  classAReport[284] = 1;

  classAReport[288] = 1; //MM 60 - default
  classAReport[289] = 1;
  classAReport[290] = 1;
  classAReport[291] = 1;

  //294-303 Draught - 8bits
  draught = std::clamp(draught, 0.0f, 25.5f);
  draughtAIS = static_cast<unsigned char>(std::round(draught * 10.0f));
  
  for(int i=0; i<8; i++)
    {
      classAReport[294 + 7 - i] = draughtAIS & (1 << i);
    }

  //302-421 Destination 
  shipDestAIS = AIS::StringToAis6(shipDestStr);  
  idx = 302;
  for(int k=0; k<shipDestAIS.size(); k++)
    {
      for(int j=5;j>=0;j--)
	{
	  classAReport[idx] = shipDestAIS[k] & (1 << j);
	  idx++;
	}
    }
  
  //422 DTE
  classAReport[422] = 1;

  //423 Spare
  classAReport[423] = 0;

  return BitsToArmoredASCII(classAReport);
}


std::string AIS::GenerateMessage1(unsigned long long aTimeStamp, unsigned int aHdg, unsigned int aMmsi, unsigned int aSpeed, float aPosX, float aPosZ, float aLong, float aLat)
{
  std::vector<bool> classAReport(168, 0);
  unsigned int timestamp = aTimeStamp % 60;
  
  // fill class A report fields
    
  // 0-5: message type, set to 0b000001 for normal class A position report
  classAReport[5] = 1;
    
  // 6-7 repeat indicator, set to 0b11 to signify do not repeat
  classAReport[6] = 1;
  classAReport[7] = 1;

  // 8-37 MMSI, 9-decimal digit in 30 bit field
  for (int i=0; i < 30; i++) {
    classAReport[8 + 29 - i] = aMmsi % 2;
    aMmsi >>= 1;
  }

  // 38-41 navigation status
  // set to 0b0000 for underway using engine
  classAReport[38] = 0;
  classAReport[39] = 0;
  classAReport[40] = 0;
  classAReport[41] = 0;
  if (aSpeed == 0) {
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
    classAReport[50 + 9 - i] = aSpeed % 2;
    aSpeed >>= 1;
  }

  // 60 position accuracy, set to 0b1 to indicate DGPS-quality fix, since
  // shipLong and shipLat have 5 decimals giving a 1m resolution.
  classAReport[60] = 1;

  // 61-88 longitude in a 28-bit field encoding a signed integer representing a float with a
  // resolution of 0.0001 corresponding to the longitude in minutes
  int longitude = (int) 600000.0f * aLong;
  bool longIsNeg = longitude < 0;
  for (int i=0; i < 28; i++) {
    classAReport[61 + 27 - i] = longitude % 2;
    longitude >>= 1;
  }
  if (longIsNeg) classAReport[61] = 1; // set the sign bit
    
  // 89-115 latitude in a 27-bit field encoding a signed integer representing a float with a
  // resolution of 0.0001 corresponding to the latitude in minutes
  int latitude = (int) 600000.0f * aLat;
  bool latIsNeg = latitude < 0;
  for (int i=0; i < 27; i++) {
    classAReport[89 + 26 - i] = latitude % 2;
    latitude >>= 1;
  }
  if (latIsNeg) classAReport[89] = 1; // set the sign bit

  // 116-127 course over ground, 12 bit field, unsigned int representing a float with
  // a resolution of 0.1 corresponding to the course over ground in degrees relative to true north
  unsigned int cog = 10 * aHdg;
  for (int i=0; i < 12; i++) {
    classAReport[116 + 11 - i] = cog % 2;
    cog >>= 1;
  }

  // 128-136 true heading, 9 bit field, unsigned int
  for (int i=0; i < 9; i++) {
    classAReport[128 + 8 - i] = aHdg % 2;
    aHdg >>= 1;
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
