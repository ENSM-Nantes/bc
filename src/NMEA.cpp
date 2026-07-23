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

#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>
#include "NMEA.hpp"
#include "NMEASentences.hpp"
#include "Constants.hpp"
#include "Utilities.hpp"
#include "AIS.hpp"
#include "MagneticNorth.hpp"

NMEA::NMEA()
{

}

NMEA::NMEA(OwnShip *aOwnShip, OtherShips *aOtherShips, Terrain *aTerrain, Wind *aWind, RadarCalculation *aRadarCalc, unsigned int aIntervalReport):mAutopilot(aOwnShip) 
{
  mOwnShip = aOwnShip;
  mOtherShips = aOtherShips;
  mTerrain = aTerrain;
  mWind = aWind;
  mRadarCalc = aRadarCalc;
  mIntervalReport = aIntervalReport;
}

void NMEA::SetModelData(OwnShip *aOwnShip, OtherShips *aOtherShips, Terrain *aTerrain, Wind *aWind, RadarCalculation *aRadarCalc, unsigned int aIntervalReport)
{
  OtherShips *pOtherShips = (OtherShips*)aOtherShips;
  
  mAutopilot.Init(aOwnShip);
  mOwnShip = aOwnShip;
  mOtherShips = aOtherShips;
  mTerrain = aTerrain;
  mWind = aWind;
  mRadarCalc = aRadarCalc;
  mIntervalReport = aIntervalReport;
  mAIS.Init(pOtherShips->getNumber());
}

void NMEA::Init(unsigned int aStartTime, std::string serialPortName, irr::u32 serialBaudrate, std::string udpHostname, std::string udpPortName, std::string udpListenPortName)
{
  mMessageQueue = {};
  mCurrentMessageType = 0;
  mLastSendEvent = aStartTime;
  
  try
    {
      asio::ip::udp::resolver resolver(mIoService);
      asio::ip::udp::resolver::query query(asio::ip::udp::v4(), udpHostname, udpPortName);
      mReceiverEndpoint = *resolver.resolve(query);
    }
  catch (std::exception& e)
    {
      std::cout << "NMEA::Error : " << e.what() << std::endl;
    }

  asio::ip::tcp::socket sock(mIoService);
  asio::error_code ec;
  sock.connect(asio::ip::tcp::endpoint(mReceiverEndpoint.address(), 22),ec);
  
  if (ec || mReceiverEndpoint.address().to_v4().to_string() == "0.0.0.0") {
    mIsHostAlive=false;
    std::cout << "NMEA::HostALive : " << mReceiverEndpoint.address() << " : " << mIsHostAlive << std::endl;
    return;
  }
  else
    {
      mIsHostAlive=true;
      std::cout << "NMEA::HostALive : " << mReceiverEndpoint.address() << " : "  << mIsHostAlive << std::endl;
    }

  // create send socket
  mSocket = new asio::ip::udp::socket(mIoService);

  // set up listening thread
  terminateNmeaReceive = 0;
  receivedNmeaMessages = std::vector<std::string>();
  std::thread* receiveThreadObject = 0;
  receiveThreadObject = new std::thread(&NMEA::ReceiveThread, this, udpListenPortName);
    
  //Set up serial
  if (!serialPortName.empty() && (serialBaudrate > 0))
    {
      try
        {
	  serial::Timeout timeout = serial::Timeout::simpleTimeout(50);

	  mMySerialPort.setPort(serialPortName);
	  mMySerialPort.setBaudrate(serialBaudrate);
	  mMySerialPort.setTimeout(timeout);

	  mMySerialPort.open();
	  std::cout << "Serial port opened." << std::endl;

        }
      catch (std::exception const& e)
        {
	  std::cout << "NMEA::Error : " << e.what() << std::endl;
        }
    }
}

NMEA::~NMEA()
{

  //Shut down serial port here
  if (mMySerialPort.isOpen())
    {
      try
        {
	  mMySerialPort.close();
        }
      catch (std::exception const& e)
        {
        }
    }

  // stop the NMEA receive thread
  terminateNmeaReceiveMutex.lock();
  terminateNmeaReceive = 1;
  terminateNmeaReceiveMutex.unlock();

}

void NMEA::ReceiveThread(std::string udpListenPortName)
{
    
  // setup socket
  asio::io_context io_context;
  asio::ip::udp::socket rcvSocket(io_context);

  try 
    {
      irr::u16 port = std::stoi(udpListenPortName);
      rcvSocket.open(asio::ip::udp::v4());
      rcvSocket.bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), port));
      std::cout << "Listening for NMEA messages on " << rcvSocket.local_endpoint().address().to_string() << ":" << port << std::endl;
    } catch (std::exception e) 
    {
      std::cerr << e.what() << ". In NMEA::ReceiveThread()" << std::endl;
      return;
    }

  for (;;) 
    {
        
      try
        {
        
	  // terminate thread?
	  terminateNmeaReceiveMutex.lock();
	  if (terminateNmeaReceive != 0)
            {
	      terminateNmeaReceiveMutex.unlock();
	      break;
            }
	  terminateNmeaReceiveMutex.unlock();

	  int bufferSize = 128;
	  char * buf = new char[bufferSize]();
            
	  // set socket timeout as in AISOverUDP
#ifdef WIN32
	  DWORD timeout = 1000;
	  setsockopt(rcvSocket.native_handle(), SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(DWORD))!=0;
#else
	  struct timeval tv = { 1, 0 };
	  setsockopt(rcvSocket.native_handle(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
#endif
            
	  // read from socket
#ifdef WIN32
	  int nread = ::recv(rcvSocket.native_handle(), buf, bufferSize,0);
#else
	  ssize_t nread = ::read(rcvSocket.native_handle(), buf, bufferSize);
#endif

	  if (nread > 0) 
            {
	      // first char $ or !, otherwise ignore
	      if (!(buf[0] == '$' || buf[0] == '!')) continue;

	      // convert char buffer to string and add it to shared vector
	      // subsequent processing is handled by NMEA::receive
	      std::string message(buf);
	      receivedNmeaMessagesMutex.lock();
	      receivedNmeaMessages.push_back(message);
	      receivedNmeaMessagesMutex.unlock();
            }

        } catch (std::exception e) 
        {
	  std::cerr << e.what() << std::endl;
        }
    }
}

void NMEA::Receive(void)
{
  // check the receivedNmeaMessages shared vector for new messages
  // if it is non-empty, parse the messages
  receivedNmeaMessagesMutex.lock();
  if (!receivedNmeaMessages.empty())
    {
      for (int i=0; i < receivedNmeaMessages.size(); i++)
        {
	  std::string message = receivedNmeaMessages[i];

	  // get all sentences and strip \r\n
	  std::vector<std::string> sentences;
	  int last_pos = 0; 
	  int pos = message.find("\r\n");
	  while (pos != std::string::npos)
            {
	      int sentence_len = pos - last_pos;
	      sentences.push_back(message.substr(last_pos, sentence_len));
	      last_pos = pos;
	      pos = message.find("\r\n", pos+3);
            }
            
	  // iterate over sentences and handle them one by one
	  for (int i=0; i < sentences.size(); i++)
            {
	      std::string sentence = sentences[i];

	      if (sentence.length() < 10) continue;
               
	      // parse the provided checksum and verify it
	      irr::u32 providedChecksum;
	      irr::u32 checksum;
	      try 
                {
		  providedChecksum = std::stoi(sentence.substr(sentence.length()-2, 2), 0, 16);
		  checksum = sentence.at(1);
		  for (auto character : sentence.substr(2, sentence.length()-5)) checksum ^= character;
		  if (checksum != providedChecksum) 
                    {
		      std::cerr << "invalid checksum: " << sentence << " expected " << std::hex << checksum << std::endl;
		      continue;
                    }
                } catch (const std::invalid_argument& e) { continue;
	      } catch (const std::out_of_range& e) { continue; }


	      // construct vector of fields
	      std::vector<std::string> fields;
	      char last_char;
	      std::string field = "";
	      for (int i=7; i < sentence.length(); i++) 
                {
		  last_char = sentence[i];
		  if (last_char == '*') break;
		  if (last_char == ',') 
                    {
		      fields.push_back(field);
		      field = "";
                    } else 
                    {
		      field += last_char;
                    }
                }
	      fields.push_back(field);

	      std::string type = sentence.substr(0, 1);

	      if (!type.compare("!")) continue; // AIS sentence

	      if (!type.compare("$")) 
                { // normal sentence
		  if (!sentence.substr(1,1).compare("P"))
                    {
		      // proprietary sentence
		      continue;
                    } else
                    {
		      std::string id = sentence.substr(3, 3);

		      if (!id.compare("APB"))
                        { // autopilot sentence B 

			  if (fields.size() != 14) continue; // we expect exactly 14 fields

			  APB apb;
			  try 
                            {
			      apb.status = fields[0][0];
			      apb.warning = fields[1][0];
			      apb.cross_track_error = std::stof(fields[2]);
			      apb.direction = fields[3][0];
			      apb.cross_track_units = fields[4][0];
			      apb.arrival_circle_entered = fields[5][0];
			      apb.perpendicular_passed = fields[6][0];
			      apb.bearing_orig_to_dest = std::stof(fields[7]);
			      apb.bearing_orig_to_dest_type = fields[8][0];
			      apb.dest_waypoint_id = fields[9];
			      apb.bearing_to_dest = std::stof(fields[10]);
			      apb.bearing_orig_to_dest_type = fields[11][0];
			      apb.heading_to_dest = std::stof(fields[12]);
			      apb.heading_to_dest_type = fields[13][0];
                            } catch (const std::invalid_argument& e)
                            {
			      std::cerr << "error while parsing a float value for APB" << std::endl;
			      continue;
                            }

			  mAutopilot.receiveAPB(apb);

                        } else if (!id.compare("RMB"))
                        { // recommended minimum navigation information B

			  if (fields.size() != 13 && fields.size() != 14) continue; // 13 or 14 fields based on NMEA version

			  RMB rmb;
			  try {
			    rmb.status = fields[0][0];
			    rmb.cross_track_error = std::stof(fields[1]);
			    rmb.direction = fields[2][0];
			    rmb.dest_waypoint_id = fields[3];
			    rmb.orig_waypoint_id = fields[4];
			    rmb.dest_waypoint_latitude = fields[5];
			    rmb.dest_waypoint_latitude_dir = fields[6][0];
			    rmb.dest_waypoint_longitude = fields[7];
			    rmb.dest_waypoint_longitude_dir = fields[8][0];
			    rmb.range_to_dest = std::stof(fields[9]);
			    rmb.bearing_to_dest = std::stof(fields[10]);
			    rmb.dest_closing_velocity = std::stof(fields[11]);
			    rmb.arrival_status = fields[12][0];
			    rmb.faa_mode = '\0';
			    if (fields.size() == 14) rmb.faa_mode = fields[13][0];
			  } catch (const std::invalid_argument& e)
                            {
			      std::cerr << "error while parsing a float value for RMB" << std::endl;
			      continue;
                            }

			  mAutopilot.receiveRMB(rmb);
                        }
                    }
                }
            }
        }
      receivedNmeaMessages.clear();
    }
  receivedNmeaMessagesMutex.unlock();
}


void NMEA::Update(sTime& aTime)
{
  char messageBuffer[MAX_NMEA_SENTENCE_CHARS] = {0};
  bool done = false;
  std::string data = "", messageToSend = "";
  unsigned int now = aTime.currentTime;
  unsigned int osHdg=0, osMmsi=0, osSpeed=0;
  float osPosX=0, osPosZ=0, shipLong=0, shipLat=0;
  
  if(mOtherShips->getNumber() >= 0)
    { 
      std::vector<unsigned int> readyShips = mAIS.GetReadyShips(mOtherShips, now);
      for(auto ship : readyShips)
	{
	  osHdg = (unsigned int) (mOtherShips->getHeading(ship) * 180/PI);
	  osMmsi = mOtherShips->getMMSI(ship);
	  osSpeed = 10.0f * MPS_TO_KTS * mOtherShips->getSpeed(ship);
	  osPosX = mOtherShips->getPosition(ship).X + mOwnShip->getOffsetPos().X;
	  osPosZ = mOtherShips->getPosition(ship).Z + mOwnShip->getOffsetPos().Z;
	  shipLong = mTerrain->xToLong(osPosX);
	  shipLat  = mTerrain->zToLat(osPosZ);
	  
	  //Generate Class A : Message 1
	  data = mAIS.GenerateMessage1(aTime.absoluteTime, osHdg, osMmsi, osSpeed, osPosX, osPosZ, shipLong, shipLat);
	  snprintf(messageBuffer, MAX_NMEA_SENTENCE_CHARS,"!AIVDM,%d,%d,,%c,%s,%d", 1, 1, 'B', data.c_str(), 0);
 
	  messageToSend.append(AddChecksum(std::string(messageBuffer)));
	  mMessageQueue.push_back(messageToSend);
	  messageToSend.clear();
	  data.clear();

	  //Generate Class A : Message 5
	  data = mAIS.GenerateMessage5(mOtherShips, ship);
	  std::string frag1 = data.substr(0, 60);
	  std::string frag2 = data.substr(60);
	  
	  snprintf(messageBuffer, MAX_NMEA_SENTENCE_CHARS,"!AIVDM,%d,%d,,%c,%s,%d", 2, 1, 'B', frag1.c_str(), 0);
	  messageToSend.append(AddChecksum(std::string(messageBuffer)));
	  mMessageQueue.push_back(messageToSend);
	  messageToSend.clear();

	  snprintf(messageBuffer, MAX_NMEA_SENTENCE_CHARS,"!AIVDM,%d,%d,,%c,%s,%d", 2, 2, 'B', frag2.c_str(), 2);
	  messageToSend.append(AddChecksum(std::string(messageBuffer)));
	  mMessageQueue.push_back(messageToSend);
	  messageToSend.clear();
	  
	}
      
      readyShips.clear();
    }

  if(now - mLastSendEvent < mIntervalReport)
    {
      return;
    }

  std::string dateTimeString = Utilities::ttos(aTime.absoluteTime);

  std::string dateString = dateTimeString.substr(0, 8);
  std::string timeString = dateTimeString.substr(8, 6);

  std::string yearString = dateString.substr(0, 4);
  std::string monthString = dateString.substr(4, 2);
  std::string dayString = dateString.substr(6, 2);

  std::string hourString = timeString.substr(0, 4);
  std::string minuteString = timeString.substr(4, 2);
  std::string secondsString = timeString.substr(6, 2);
    
  const char *year = yearString.c_str();
  const char *mon  = monthString.c_str();
  const char *mday = dayString.c_str();

  const char *hour = hourString.c_str();
  const char *min  = minuteString.c_str();
  const char *sec  =secondsString.c_str();

  double rudderAngleS = 99;
  if(mOwnShip->getNumberRud() > 1)
    rudderAngleS = -mOwnShip->getRudder("starboard").getDelta()*180/PI;
  
  double rudderAngleP = -mOwnShip->getRudder("port").getDelta()*180/PI;
  
  double engineRPM[] = {
    mOwnShip->getPropeller("port").getRevs(),  
    mOwnShip->getPropeller("starboard").getRevs() 
  };
    
  float posZ = mOwnShip->getPosition().Z + mOwnShip->getOffsetPos().Z ;
  float posX = mOwnShip->getPosition().X + mOwnShip->getOffsetPos().X ;

  irr::f32 lat = mTerrain->zToLat(posZ);
  irr::f32 lon = mTerrain->xToLong(posX);

  irr::f32 cog = mOwnShip->getHeading()*irr::core::RADTODEG;
  irr::f32 sog = mOwnShip->getSpeed()*MPS_TO_KTS;
  irr::f32 spdWater = mOwnShip->getSpeedThroughWater() * MPS_TO_KTS;
  irr::f32 latSpeed = mOwnShip->getLateralSpeed();
  irr::f32 hdg = mOwnShip->getHeading()*irr::core::RADTODEG;
  irr::f32 rot = mOwnShip->getRateOfTurn()*irr::core::RADTODEG*60;
  irr::f32 pitch = mOwnShip->getPitch() * irr::core::RADTODEG;
  irr::f32 roll = mOwnShip->getRollAngle() * irr::core::RADTODEG;
  irr::f32 depth = mOwnShip->getDepth(mTerrain);
  irr::f32 windDirection = mWind->getTrueDirection();
  irr::f32 windSpeed = mWind->getTrueSpeed();
  irr::f32 apparentWindDir = mWind->getApparentDir() * irr::core::RADTODEG;
  irr::f32 apparentWindSpd = mWind->getApparentSpd();
  irr::f32 bowThruster = mOwnShip->getBowThruster()*100;
  irr::f32 sternThruster = mOwnShip->getSternThruster()*100;
  unsigned char hasBowThruster = mOwnShip->getThruster().HasBowThruster() ? 1 : 0;
  unsigned char hasSternThruster = mOwnShip->getThruster().HasSternThruster() ? 1 : 0;  
  
  int signRoll = 0;
  if (mOwnShip->getHull().getInvertRoll())
    signRoll = -1;
  else
    signRoll = 1;

  roll = signRoll*roll;
    
  char eastWest = 0, northSouth = 0;

  if(lat < 0) northSouth = 'S';
  else northSouth = 'N';

  if(lon < 0) eastWest = 'W';
  else eastWest = 'E';
  
  geomag::Vector posMagnetic = geomag::geodetic2ecef(lat, lon, 0);
  geomag::Vector magField = geomag::GeoMag(2026.4, posMagnetic, geomag::WMM2025);
  geomag::Elements outMagn= geomag::magField2Elements(magField, lat, lon);
  irr::f32 hdgMagn = hdg + outMagn.declination;

  if (hdgMagn > 360)
    hdgMagn -= 360.0;
  
  lat = fabs(lat);
  lon = fabs(lon);
  irr::f32 latMinutes = (lat - (int)lat)*60;
  irr::f32 lonMinutes = (lon - (int)lon)*60;
  irr::u8 latDegrees = (int) lat;
  irr::u8 lonDegrees = (int) lon;

  irr::f32 xUp = mOwnShip->getGeoParams().lPP/2 - mOwnShip->getGeoParams().xG;
  irr::f32 xDown = -mOwnShip->getGeoParams().lPP/2 - mOwnShip->getGeoParams().xG;
  irr::f32 latSpeedUp = latSpeed + mOwnShip->getRateOfTurn() * xUp;
  irr::f32 latSpeedDown = latSpeed + mOwnShip->getRateOfTurn() * xDown;

  latSpeedUp = latSpeedUp / 0.51444;
  latSpeedDown = latSpeedDown / 0.51444;

  switch (mCurrentMessageType) { // EN 61162-1:2011
  case RMC: // 8.3.69 Recommended minimum navigation information
    {
      snprintf(messageBuffer,MAX_NMEA_SENTENCE_CHARS,"$GPRMC,%s%s%s.00,A,%02u%06.3f,%c,%03u%06.3f,%c,%.1f,%.1f,%s%s%s,,,A,S",hour,min,sec,latDegrees,latMinutes,northSouth,lonDegrees,lonMinutes,eastWest,sog,cog,year,mon,mday); //FIXME: SOG -> knots, COG->degrees
      mMessageQueue.push_back(AddChecksum(std::string(messageBuffer)));
      break;
    }
  case GLL: // 8.3.36 Geographic position – Latitude/longitude
    {
      snprintf(messageBuffer,MAX_NMEA_SENTENCE_CHARS,"$GPGLL,%02u%06.3f,%c,%03u%06.3f,%c,%s%s%s.00,A,A",latDegrees,latMinutes,northSouth,lonDegrees,lonMinutes,eastWest,hour,min,sec);
      mMessageQueue.push_back(AddChecksum(std::string(messageBuffer)));
      break;
    }
  case GGA: // 8.3.35 Global positioning system (GPS) fix data
    {
      snprintf(messageBuffer,MAX_NMEA_SENTENCE_CHARS,"$GPGGA,%s%s%s.00,%02u%06.3f,%c,%03u%06.3f,%c,1,12,0.0,0.0,M,0.0,M,,",hour,min,sec,latDegrees,latMinutes,northSouth,lonDegrees,lonMinutes,eastWest); //Hardcoded NMEA Quality 8, Satellites 8, HDOP 0.9
      mMessageQueue.push_back(AddChecksum(std::string(messageBuffer)));
      break;
    }
  case RSA: // 8.3.73 Rudder sensor angle
    {    
      snprintf(messageBuffer,MAX_NMEA_SENTENCE_CHARS,"$IIRSA,%.1f,A,%.1f,A,V",rudderAngleS, rudderAngleP);
      mMessageQueue.push_back(AddChecksum(std::string(messageBuffer)));
      break;
    }
  case RPM: // 8.3.72 Revolutions
    {
      std::string messageToSend = "";
      
      snprintf(messageBuffer, MAX_NMEA_SENTENCE_CHARS, "$IIRPM,S,%d,%.1f,%.1f,A", 1, engineRPM[0], mOwnShip->getEngine("port").getRpmMax()/60); // 'S' is for shaft, 
      messageToSend.append(AddChecksum(std::string(messageBuffer)));
      
      mMessageQueue.push_back(messageToSend);
      
      messageToSend.clear();

      if(mOwnShip->getNumberProp() > 1)
	{
	  snprintf(messageBuffer, MAX_NMEA_SENTENCE_CHARS, "$IIRPM,S,%d,%.1f,%.1f,A", 2, engineRPM[1], mOwnShip->getEngine("starboard").getRpmMax()/60); // 'S' is for shaft, 
	  messageToSend.append(AddChecksum(std::string(messageBuffer)));

	  mMessageQueue.push_back(messageToSend);
	}
      break;
    }
  case TTM: // 8.3.85 Tracked target message
    {
      if (mRadarCalc->getARPATracksSize() > 0) {
	std::string messageToSend = "";
	//To think about/add: Lost contacts? Manually aquired contacts?
	for (int i=0; i<mRadarCalc->getARPATracksSize(); i++) {
	  ARPAContact contact = mRadarCalc->getARPAContactFromTrackIndex(i);
	  ARPAEstimatedState state = contact.estimate;
	  snprintf(messageBuffer,MAX_NMEA_SENTENCE_CHARS,"$RATTM,%02d,%.1f,%.1f,T,%.1f,%.1f,T,%.1f,%.1f,N,TGT%02d,T,,%s.00,A",
		   state.displayID - 1,
		   state.range,
		   state.bearing,
		   state.speed,
		   state.absHeading,
		   state.cpa,
		   state.tcpa,
		   state.displayID - 1,
		   timeString.c_str()
		   );
	  messageToSend.append(AddChecksum(std::string(messageBuffer)));
	}
	if (messageToSend != "") mMessageQueue.push_back(messageToSend);
      }
      break;
    }
  case ZDA: // 8.3.106 Time and date
    {
      snprintf(messageBuffer,MAX_NMEA_SENTENCE_CHARS,"$RAZDA,%s%s%s.00,%s,%s,%s,00,00",hour,min,sec,mday,mon,year);
      mMessageQueue.push_back(AddChecksum(std::string(messageBuffer)));
      break;
    }
  case DTM: // 8.3.27 Datum reference
    {
      snprintf(messageBuffer,MAX_NMEA_SENTENCE_CHARS,"$RADTM,W84,,,,,,,");
      mMessageQueue.push_back(AddChecksum(std::string(messageBuffer)));
      break;
    }
  case HEHDT: // 8.3.44 Heading true
    {
      snprintf(messageBuffer,MAX_NMEA_SENTENCE_CHARS,"$HEHDT,%.1f,T",hdg); // T = true north
      mMessageQueue.push_back(AddChecksum(std::string(messageBuffer)));
      break;
    }
  case DPT: //Depth
    {
      snprintf(messageBuffer,MAX_NMEA_SENTENCE_CHARS,"$SDDPT,%.1f,,",depth); //Depth, Offset from transducer: Positive - distance from transducer to water line, or Negative - distance from transducer to keel, max depth measurable
      mMessageQueue.push_back(AddChecksum(std::string(messageBuffer)));
      break;
    }
  case TIROT: // 8.3.71 Rate of turn
    {
      snprintf(messageBuffer,MAX_NMEA_SENTENCE_CHARS,"$TIROT,%.1f,A",rot);  // A = data valid
      mMessageQueue.push_back(AddChecksum(std::string(messageBuffer)));
      break;
    }
  case WIMWV:
    {
      snprintf(messageBuffer,MAX_NMEA_SENTENCE_CHARS,"$IIMWV,%.1f,T,%.1f,N,A", windDirection, windSpeed);
      mMessageQueue.push_back(AddChecksum(std::string(messageBuffer)));
      break;
    }
  case WIMWR:
    {
      snprintf(messageBuffer, MAX_NMEA_SENTENCE_CHARS, "$IIMWV,%.1f,R,%.1f,N,A", apparentWindDir, apparentWindSpd);
      mMessageQueue.push_back(AddChecksum(std::string(messageBuffer)));
      break;
    }
  case VHW:
    {
      snprintf(messageBuffer,MAX_NMEA_SENTENCE_CHARS,"$VDVHW,0,T,0,M,%.1f,N,0,K",spdWater);
      mMessageQueue.push_back(AddChecksum(std::string(messageBuffer)));
      break;
    } 
  case VTG:
    {
      snprintf(messageBuffer,MAX_NMEA_SENTENCE_CHARS,"$VDVTG,0,T,%.1f,M,%.1f,N,%.1f,K",hdgMagn, latSpeedUp, latSpeedDown);
      mMessageQueue.push_back(AddChecksum(std::string(messageBuffer)));
      break;
    }
  case XDR:
    {
      snprintf(messageBuffer,MAX_NMEA_SENTENCE_CHARS,"$IIXDR,A,%.1f,D,ROLL,A,%.1f,D,PITCH", roll, pitch);
      mMessageQueue.push_back(AddChecksum(std::string(messageBuffer)));
      break;
    }
  case THR:
    {
      snprintf(messageBuffer,MAX_NMEA_SENTENCE_CHARS,"$IITHR,BOW,%d,%.1f,STERN,%d,%.1f", hasBowThruster, bowThruster, hasSternThruster, sternThruster);
      mMessageQueue.push_back(AddChecksum(std::string(messageBuffer)));
      break;
    }
  case AIVD0:
    {
      //Generate Class A : Message 1
      data = mAIS.GenerateMessage1(aTime.absoluteTime, hdg, mOwnShip->getMMSI(), sog, posX, posZ, lon, lat);
      snprintf(messageBuffer, MAX_NMEA_SENTENCE_CHARS,"!AIVDO,%d,%d,,%c,%s,%d", 1, 1, 'B', data.c_str(), 0);
  
      messageToSend.append(AddChecksum(std::string(messageBuffer)));
      mMessageQueue.push_back(messageToSend);
      messageToSend.clear();
      data.clear();
      break;
    }
    
  default:
    break;
  }

  mLastSendEvent = now;
  mCurrentMessageType++;
  mCurrentMessageType %= MSG_MAX;
}

void NMEA::ClearQueue()
{
  mMessageQueue.clear();
}

void NMEA::SendSerial()
{
  if (mMySerialPort.isOpen())
    {
      for (auto message : mMessageQueue)
        {
	  mMySerialPort.write(message);
        }
    }
}

void NMEA::SendUdp(void)
{    
  if (!mMessageQueue.empty()) {
    try {
      if (!mSocket->is_open()) {
	mSocket->open(asio::ip::udp::v4());
	mSocket->set_option(asio::socket_base::broadcast(true));
      }
      for (auto message : mMessageQueue)
	{
	  mSocket->send_to(asio::buffer(message), mReceiverEndpoint);
	}
    } catch (std::exception& e)
      {
	std::cout << "NMEA::Error : " << e.what() << std::endl;
      }
  }
}

std::string NMEA::AddChecksum(std::string messageIn)
{
  char checksumBuffer[3];
  //Get checksum
  unsigned char checksum=0;
  irr::u8 s = messageIn.length();
  for(int i = 1; i<s; i++)
    {
      checksum^= messageIn.at(i);
    }
  snprintf(checksumBuffer,sizeof(checksumBuffer),"%02X",checksum);
  return messageIn + "*" + std::string(checksumBuffer) + "\r\n";
}


bool NMEA::GetHostStatus(void)
{
  return mIsHostAlive;
}
