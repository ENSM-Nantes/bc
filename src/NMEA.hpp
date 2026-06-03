/*   Bridge Command 5.0 Ship Simulator
     Copyright (C) 2015 James Packer

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

#ifndef __NMEA_HPP_INCLUDED__
#define __NMEA_HPP_INCLUDED__

#include <mutex>
#include <string>
#include <asio.hpp> //For UDP
#include "../lib/serial/serial.h"
#include "Autopilot.hpp"
#include "OtherShips.hpp"
#include "OwnShip.hpp"
#include "Terrain.hpp"
#include "Wind.hpp"
#include "RadarCalculation.hpp"

#define SENSOR_REPORT_INTERVAL (300) //ms
#define MAX_NMEA_SENTENCE_CHARS (81) 

enum eNMEAMessage{
  RMC=0,
  GLL,
  RSA,
  RPM,
  VHW,
  VTG,
  TTM,
  GGA,
  ZDA,
  DTM,
  HEHDT,
  WIMWV,
  WIMWR,
  TIROT,
  DPT,
  XDR,
  MSG_MAX
};


class NMEA {

public:

  NMEA();
  NMEA(OwnShip *aOwnShip, OtherShips *aOtherShips, Terrain *aTerrain, Wind *aWind, RadarCalculation *aRadarCalc);
  ~NMEA();
  void Init(unsigned int aStartTime, std::string serialPortName, irr::u32 serialBaudrate, std::string udpHostname, std::string udpPortName, std::string udpListenPortName);
  void Update(sTime& aTime);
  void SendSerial(void);
  void SendUdp(void);
  void ClearQueue(void);
  void ReceiveThread(std::string udpListenPortName);
  void Receive(void);
  bool GetHostStatus(void);
  void SetModelData(OwnShip *aOwnShip, OtherShips *aOtherShips, Terrain *aTerrain, Wind *aWind, RadarCalculation *aRadarCalc);


private:

  std::string AddChecksum(std::string messageIn);
  
  Autopilot mAutopilot;
  OwnShip *mOwnShip;
  OtherShips *mOtherShips;
  Terrain *mTerrain;
  Wind *mWind;
  RadarCalculation *mRadarCalc;
  
  serial::Serial mMySerialPort;
  unsigned int mLastSendEvent; 
  std::vector<std::string> mMessageQueue;
  int mCurrentMessageType; 

  bool mIsHostAlive;
  asio::io_service mIoService;
  asio::ip::udp::endpoint mReceiverEndpoint;
  asio::ip::udp::socket* mSocket;
  
  unsigned int terminateNmeaReceive;
  std::mutex terminateNmeaReceiveMutex;
  std::vector<std::string> receivedNmeaMessages;
  std::mutex receivedNmeaMessagesMutex;
};

#endif // __NMEA_HPP_INCLUDED__
