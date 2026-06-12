/*   Bridge Command 5.0 Ship Simulator
     Copyright (C) 2014 James Packer

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

#include <iostream> //debugging
#include "OtherShips.hpp"
#include "Constants.hpp"
#include "OtherShip.hpp"
#include "IniFile.hpp"
#include "RadarData.hpp"
#include "ScenarioDataStructure.hpp"
#include "Water.hpp"
#include "Terrain.hpp"


//using namespace irr;

OtherShips::OtherShips()
{

}

OtherShips::~OtherShips()
{
    for(std::vector<OtherShip*>::iterator it = mOtherShips.begin(); it != mOtherShips.end(); ++it) {
        delete (*it);
    }
    mOtherShips.clear();
}

void OtherShips::load(std::vector<OtherShipData> aOtherShipsData, irr::f32 aScenarioStartTime, Terrain *aTerrain, Water *aWater, OperatingMode::Mode aMode, irr::IrrlichtDevice* aDev)
{
  irr::scene::ISceneManager* smgr = aDev->getSceneManager();

  mTerrain = aTerrain;
  mWater = aWater;
  
    for(irr::u32 i=0;i<aOtherShipsData.size();i++)
    {
        //Get ship type and construct filename
        std::string otherShipName = aOtherShipsData.at(i).shipName;
        //Get initial position
        irr::f32 shipX = mTerrain->longToX(aOtherShipsData.at(i).initialLong);
        irr::f32 shipZ = mTerrain->latToZ(aOtherShipsData.at(i).initialLat);

        //Load leg information
        std::vector<Leg> legs;
        irr::f32 legStartTime = aScenarioStartTime;
        if (aMode==OperatingMode::Normal) { //Only load leg information in normal mode
            for(irr::u32 j=0; j<aOtherShipsData.at(i).legs.size(); j++){
                //go through each leg (if any), and load
                Leg currentLeg;
                currentLeg.bearing = aOtherShipsData.at(i).legs.at(j).bearing;
                currentLeg.speed = aOtherShipsData.at(i).legs.at(j).speed;
                currentLeg.startTime = legStartTime;

                //Use distance to calculate startTime of next leg, and stored for later reference.
                irr::f32 distance = aOtherShipsData.at(i).legs.at(j).distance;
                currentLeg.distance = distance;

                legs.push_back(currentLeg);

                //find the start time for the next leg
                legStartTime = legStartTime + SECONDS_IN_HOUR*(distance/fabs(currentLeg.speed)); // nm/kts -> hours, so convert to seconds
            }
            //add a final 'stop' leg, which the ship will remain on after it has passed the other legs.
            Leg stopLeg;
            stopLeg.bearing=0;
            stopLeg.speed=0;
            stopLeg.distance=0;
            stopLeg.startTime = legStartTime;
            legs.push_back(stopLeg);
        }

        //Create otherShip and load into vector
        std::string internalName = "OtherShip_";
        internalName.append(std::to_string(i));
        mOtherShips.push_back(new OtherShip (otherShipName,internalName,irr::core::vector3df(shipX,0.0f,shipZ),legs, aDev));
    }

}

void OtherShips::update(sTime& aTime, irr::f32 tideHeight, irr::u32 lightLevel, irr::core::vector3df ownShipPosition, irr::f32 ownShipLength)
{
  float deltaTime = aTime.deltaTime;
  float scenarioTime = aTime.scenarioTime;
  
    for(std::vector<OtherShip*>::iterator it = mOtherShips.begin(); it != mOtherShips.end(); ++it) {

        //Find local wave height
        irr::core::vector3df prevPosition = (*it)->getPosition();
        irr::f32 waveHeightFiltered = prevPosition.Y - tideHeight - (*it)->getHeightCorrection(); //Calculate the previous wave height:

        //Apply up/down motion from waves, with some filtering
        irr::f32 timeConstant = 0.5;//Time constant in s; TODO: Make dependent on vessel size
        irr::f32 factor = deltaTime/(timeConstant+deltaTime);
        waveHeightFiltered = (1-factor) * waveHeightFiltered + factor*mWater->getWaveHeight(prevPosition.X,prevPosition.Z); //TODO: Check implementation of simple filter!

        //Special case, if paused, just use the actual wave height. A bit of a bodge, but avoids having to store the previous filter value
        if (deltaTime == 0) {
            waveHeightFiltered = mWater->getWaveHeight(prevPosition.X,prevPosition.Z);
        }

        (*it)->update(deltaTime, scenarioTime, tideHeight+waveHeightFiltered, lightLevel);

        //Set or clear triangle selector depending on distance from own ship
        if ((*it)->getSceneNode()->getAbsolutePosition().getDistanceFrom(ownShipPosition) < (ownShipLength + (*it)->getLength())) {
            (*it)->enableTriangleSelector(true);
        } else {
            (*it)->enableTriangleSelector(false);
        }
    }

}

void OtherShips::enableAllTriangleSelectors()
{
    for(std::vector<OtherShip*>::iterator it = mOtherShips.begin(); it != mOtherShips.end(); ++it) {
        // This will return to normal the next time OtherShips::update is called.
        (*it)->enableTriangleSelector(true);
    }
}

irr::scene::ISceneNode* OtherShips::getSceneNode(int number)
{
    if (number < (int)mOtherShips.size() && number >= 0) {
        return mOtherShips.at(number)->getSceneNode();
    } else {
        return 0;
    }
}

RadarData OtherShips::getRadarData(irr::u32 number, irr::core::vector3df scannerPosition) const
//Get data for OtherShip (number) relative to scannerPosition
{
    RadarData radarData;

    if (number<=mOtherShips.size()) {
        radarData = mOtherShips[number-1]->getRadarData(scannerPosition);
    }
    return radarData;
}

irr::u32 OtherShips::getNumber() const
{
    return mOtherShips.size();
}

irr::core::vector3df OtherShips::getPosition(int number) const
{
    if (number < (int)mOtherShips.size() && number >= 0) {
        return mOtherShips.at(number)->getPosition();
    } else {
        return irr::core::vector3df(0,0,0);
    }
}

irr::f32 OtherShips::getLength(int number) const
{
    if (number < (int)mOtherShips.size() && number >= 0) {
        return mOtherShips.at(number)->getLength();
    } else {
        return 0.0;
    }
}

irr::f32 OtherShips::getBreadth(int number) const
{
    if (number < (int)mOtherShips.size() && number >= 0) {
        return mOtherShips.at(number)->getBreadth();
    } else {
        return 0.0;
    }
}

irr::f32 OtherShips::getDraught(int number) const
{
    if (number < (int)mOtherShips.size() && number >= 0) {
        return mOtherShips.at(number)->getDraught();
    } else {
        return 0.0;
    }
}


irr::f32 OtherShips::getHeading(int number) const
{
    if (number < (int)mOtherShips.size() && number >= 0) {
        return mOtherShips.at(number)->getHeading();
    } else {
        return 0;
    }
}

irr::f32 OtherShips::getSpeed(int number) const
{
    if (number < (int)mOtherShips.size() && number >= 0) {
        return mOtherShips.at(number)->getSpeed();
    } else {
        return 0;
    }
}

irr::f32 OtherShips::getEstimatedDisplacement(int number) const
{
    if (number < (int)mOtherShips.size() && number >= 0) {
        return mOtherShips.at(number)->getEstimatedDisplacement();
    } else {
        return 0;
    }
}

void OtherShips::setSpeed(int number, irr::f32 speed)
{
    if (number < (int)mOtherShips.size() && number >= 0) {
        mOtherShips.at(number)->setSpeed(speed);
    }
}

unsigned char OtherShips::getType(int number) const
{
    if (number < (int)mOtherShips.size() && number >= 0) {
      return 0x3C; //TODO : Set a real type into ShipEditor
    } else {
        return 0;
    }
}

std::string OtherShips::getShipDest(int number) const
{
    if (number < (int)mOtherShips.size() && number >= 0) {
      return "ST NAZAIRE"; //TODO : Set a real destination into ShipEditor
    } else {
        return 0;
	
    }
}

std::string OtherShips::getShipName(int number) const
{
    if (number < (int)mOtherShips.size() && number >= 0) {
      return "SUPER-BOAT"; //TODO : Set a real name into ShipEditor
    } else {
        return 0;
    }
}

std::string OtherShips::getCallSign(int number) const
{
    if (number < (int)mOtherShips.size() && number >= 0) {
      return "DGTH2"; //TODO : Set a real IMO into ShipEditor
    } else {
        return 0;
    }
}

irr::u32 OtherShips::getIMO(int number) const
{
    if (number < (int)mOtherShips.size() && number >= 0) {
      return 4444444; //TODO : Set a real IMO into ShipEditor
    } else {
        return 0;
    }
}

irr::u32 OtherShips::getMMSI(int number) const
{
    if (number < (int)mOtherShips.size() && number >= 0) {
        return 666600003;
    } else {
        return 0;
    }
}

void OtherShips::setMMSI(int number, irr::u32 mmsi)
{
    if (number < (int)mOtherShips.size() && number >= 0) {
        mOtherShips.at(number)->setMMSI(mmsi);
    }
}

void OtherShips::setPos(int number, irr::f32 positionX, irr::f32 positionZ)
{
    if (number < (int)mOtherShips.size() && number >= 0) {
        mOtherShips.at(number)->setPosition(positionX,positionZ);
    }
}

void OtherShips::setHeading(int number, irr::f32 hdg)
{
    if (number < (int)mOtherShips.size() && number >= 0) {
        mOtherShips.at(number)->setHeading(hdg);
    }
}

void OtherShips::setRateOfTurn(int number, irr::f32 rateOfTurn)
{
    if (number < (int)mOtherShips.size() && number >= 0) {
        mOtherShips.at(number)->setRateOfTurn(rateOfTurn);
    }
}

std::vector<Leg> OtherShips::getLegs(int number) const
{
    if (number < (int)mOtherShips.size() && number >= 0) {
        return mOtherShips.at(number)->getLegs();
    } else {
        //Return an empty vector
        std::vector<Leg> legs;
        return legs;
    }
}

void OtherShips::changeLeg(int shipNumber, int legNumber, irr::f32 bearing, irr::f32 speed, irr::f32 distance, irr::f32 scenarioTime)
{
    //Check if ship exists
    if (shipNumber < (int)mOtherShips.size() && shipNumber >= 0) {
        mOtherShips.at(shipNumber)->changeLeg(legNumber, bearing, speed, distance, scenarioTime);
    }
}

void OtherShips::addLeg(int shipNumber, int afterLegNumber, irr::f32 bearing, irr::f32 speed, irr::f32 distance, irr::f32 scenarioTime)
{
    //Check if ship exists
    if (shipNumber < (int)mOtherShips.size() && shipNumber >= 0) {
        mOtherShips.at(shipNumber)->addLeg(afterLegNumber, bearing, speed, distance, scenarioTime);
    }
}

void OtherShips::deleteLeg(int shipNumber, int legNumber, irr::f32 scenarioTime)
{
    //Check if ship exists
    if (shipNumber < (int)mOtherShips.size() && shipNumber >= 0) {
        mOtherShips.at(shipNumber)->deleteLeg(legNumber, scenarioTime);
    }
}

void OtherShips::resetLegs(int shipNumber, irr::f32 course, irr::f32 speedKts, irr::f32 distanceNm, irr::f32 scenarioTime)
{
    //Check if ship exists
    if (shipNumber < (int)mOtherShips.size() && shipNumber >= 0) {
        mOtherShips.at(shipNumber)->resetLegs(course, speedKts, distanceNm, scenarioTime);
    }
}

std::string OtherShips::getName(int number) const
{
    if(number < (int)mOtherShips.size() && number >= 0) {
        return mOtherShips.at(number)->getName();
    } else {
        return "";
    }
}

void OtherShips::moveNode(irr::f32 deltaX, irr::f32 deltaY, irr::f32 deltaZ)
{
    for(std::vector<OtherShip*>::iterator it = mOtherShips.begin(); it != mOtherShips.end(); ++it) {
        (*it)->moveNode(deltaX,deltaY,deltaZ);
    }
}
