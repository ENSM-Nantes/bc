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

#ifndef __AIS_HPP_INCLUDED__
#define __AIS_HPP_INCLUDED__

#include <string>
#include <vector>

#define TIME_INTERVAL_1 (10000)
#define TIME_INTERVAL_2 (6000)
#define TIME_INTERVAL_3 (2000)

#define SPEED_MAX_INTERVAL_1 (14)
#define SPEED_MAX_INTERVAL_2 (23)

class AIS
{
public:
  AIS();
  ~AIS();
  void Init(unsigned int aNumberShip);
  std::string GenerateMessage1(unsigned long long aTimeStamp, unsigned int aHdg, unsigned int aMmsi, unsigned int aSpeed, float aPosX, float aPosZ, float aLong, float aLat);
  std::string GenerateMessage5(void *aOtherShips, unsigned int aShip);
  std::vector<unsigned int> GetReadyShips(void *aOtherShips, unsigned int aNow);

private:
  std::string BitsToArmoredASCII(std::vector<bool> aBits);
  unsigned char AsciiToAis6(char aChar);
  std::vector<unsigned char> StringToAis6(const std::string& aStrIn);
  
  std::vector<unsigned int> mLastUpdates;
  std::vector<unsigned int> mReadyShips;
};

#endif

