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

#include "Autopilot.hpp"
#include "Angles.hpp"
#include "Constants.hpp"
#include "IniFile.hpp"
#include "Utilities.hpp"
#include "OwnShip.hpp"

Autopilot::Autopilot()
{

}

Autopilot::Autopilot(void *aOwnShip)
{
  Init(aOwnShip);
}


void Autopilot::Init(void *aOwnShip)
{
  mOwnShip = aOwnShip;
  currentWaypointPos = {INVALID_LAT, INVALID_LONG};
  currentWaypointId = "";
  currentLegLen = 0;

  // Check if user set Autopilot to be enabled
  std::string userFolder = Utilities::getUserDir();
  std::string iniFilename = "bc5.ini";
  AUTOPILOT_ENABLED = false;
  if (Utilities::pathExists(userFolder + "bc5.ini")) {
    iniFilename = userFolder + "bc5.ini";
  }
  std::string enableAutopilot = IniFile::iniFileToString(iniFilename, "Autopilot_Enable", "false");
  if (!enableAutopilot.compare("true")) AUTOPILOT_ENABLED = true;
}

bool Autopilot::receiveAPB(APB sentence)
{
  // determine where to steer to depending on APB
  if (!AUTOPILOT_ENABLED) return false;

  // how far off-track are we?
  crossTrackError = sentence.cross_track_error;
  if (sentence.cross_track_units == 'N') {
    crossTrackError *= M_IN_NM;
  }
  char directionToTrack = sentence.direction;

  float bearingToSteer = Angles::normaliseAngle(sentence.heading_to_dest);
  float currentHeading = Angles::normaliseAngle(((OwnShip*)mOwnShip)->getHeading()*irr::core::RADTODEG);
  float relativeBearing = 90 - currentHeading;

  if (relativeBearing >= 180.0) {
    relativeBearing -= 360.0;
  }
  if (relativeBearing <= -180.0) {
    relativeBearing += 360.0;
  }
    float wheel = 0.25 * relativeBearing;
  
  // Normal case, just set the wheel
  ((OwnShip*)mOwnShip)->setWheel(wheel);


  return false;
}

bool Autopilot::receiveRMB(RMB sentence)
{
  // determine how much to accelerate/decelerate based on RMB
  if (!AUTOPILOT_ENABLED) return false;
   
  float destWaypointLat = parseNmeaLat(
				       sentence.dest_waypoint_latitude,
				       sentence.dest_waypoint_latitude_dir);
  float destWaypointLong = parseNmeaLong(
					 sentence.dest_waypoint_longitude,
					 sentence.dest_waypoint_longitude_dir);


  
  //((OwnShip*)mOwnShip)->setPortEngine(throttle);
  //((OwnShip*)mOwnShip)->setStbdEngine(throttle);
  return false;
}

Autopilot::~Autopilot()
{
}
