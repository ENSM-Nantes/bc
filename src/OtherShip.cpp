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

#include <iostream>
#include <algorithm>
#include <filesystem>
#include <fstream>

#include "IniFile.hpp"
#include "Angles.hpp"
#include "RadarData.hpp"
#include "Constants.hpp"
#include "OtherShip.hpp"
#include "Utilities.hpp"


OtherShip::OtherShip(const std::string& aName, const std::string& aInternalName, const irr::core::vector3df& aLocation, std::vector<Leg> aLegsLoaded, irr::IrrlichtDevice* aDev)
{
  int retShipPrms = -1;
  Json::Value rootJson;
  irr::scene::IMesh *shipMesh = NULL;
  mShipScene = NULL;
  mHeight = 0;
  mSolidHeight = 0;
  mSelector = nullptr;
  mTriangleSelectorEnabled = false;

  irr::scene::ISceneManager* smgr = aDev->getSceneManager();
  mName = aName;

  std::string basePath = "models/Ships/" + mName + "/";
  std::string userFolder = Utilities::getUserDir();
  //Read model from user dir if it exists there.
  if (Utilities::pathExists(userFolder + basePath)) {
    basePath = userFolder + basePath;
  }

  //Fall back to loading from own ship folder if it doesn't exist in Otherships (useful for multiplayer)
  if (!Utilities::pathExists(basePath)) {
    basePath = "models/Ships/" + mName + "/";
    //Read model from user dir if it exists there.
    if (Utilities::pathExists(userFolder + basePath)) {
      basePath = userFolder + basePath;
    }
  }

  std::string shipJsonFilename = basePath + "boat.json";   
  std::filesystem::path boatJson = shipJsonFilename;
  
  if(std::filesystem::exists(boatJson))
    {
      std::ifstream streamJson(boatJson);                
      streamJson >> rootJson;
      retShipPrms = InitShipParams(rootJson);
      
      // get the model file
      mMeshFileName = rootJson["mesh"]["name"].asString();
      mMeshFullPath = basePath + mMeshFileName;
      streamJson.close();
      // Load the model
      shipMesh = smgr->getMesh(mMeshFullPath.c_str());
      mShipScene = smgr->addMeshSceneNode(shipMesh, 0, IDFlag_IsPickable, irr::core::vector3df(0, 0, 0));
    }

  mScaleFactor = rootJson["mesh"]["scaleFactor"].asFloat();
  float yCorrection = rootJson["mesh"]["yCorrection"].asFloat();
  mAngleCorrection = rootJson["mesh"]["angleCorrection"].asFloat();
  mName = rootJson["general"]["boatName"].asString();
  mCallSign = rootJson["general"]["callSign"].asString();
  mDestination = rootJson["general"]["dest"].asString();
  mImo = rootJson["general"]["imo"].asInt();
  mMmsi = rootJson["general"]["mmsi"].asInt();
  mType = rootJson["general"]["type"].asInt();

  //Set mesh vertical correction (world units)
  mHeightCorrection = yCorrection*mScaleFactor;

  //store initial x,y,z positions
  mEta[1] = aLocation.X;
  mEta[0] = aLocation.Z;
  mLegs = aLegsLoaded;

  if (mShipScene == nullptr) {
    aDev->getLogger()->log(("OtherShip: failed to load model for '" + aName + "'").c_str(), irr::ELL_ERROR);
    return;
  }

  mShipScene->setScale(irr::core::vector3df(mScaleFactor,mScaleFactor,mScaleFactor));
  mShipScene->setPosition(irr::core::vector3df(0,mHeightCorrection,0));

  mShipScene->setMaterialFlag(irr::video::EMF_FOG_ENABLE, true);
  mShipScene->setMaterialFlag(irr::video::EMF_NORMALIZE_NORMALS, true); //Normalise normals on scaled meshes, for correct lighting

  //store length and RCS information for radar etc
  mShipScene->updateAbsolutePosition();
  mGeoParams.lPP = mShipScene->getTransformedBoundingBox().getExtent().Z;
  mGeoParams.b = mShipScene->getTransformedBoundingBox().getExtent().X;
  mHeight = mShipScene->getTransformedBoundingBox().getExtent().Y * 0.75; //Assume 3/4 of the mesh is above water
  mGeoParams.d = -1 * mShipScene->getTransformedBoundingBox().MinEdge.Y;
    
  mRcs = 0.005*std::pow(mGeoParams.lPP ,3); //Default RCS, base radar cross section on length^3 (following RCS table Ship_RCS_table.pdf)
  std::string logMessage = "Loading '";
  logMessage.append(mMeshFullPath);
  logMessage.append("' Length (m): ");
  logMessage.append(std::to_string(mGeoParams.lPP));
  aDev->getLogger()->log(logMessage.c_str());

  //Add triangle selector and make pickable
  mShipScene->setID(IDFlag_IsPickable);
  mSelector=smgr->createTriangleSelector(shipMesh, mShipScene);
  //This is applied depending on distance to own ship, for speed
  mTriangleSelectorEnabled=false;
    
  mShipScene->setName(aInternalName.c_str());

  mSolidHeight = mScaleFactor * 5.f * mHeight;

  //Set lighting to use diffuse and ambient, so lighting of untextured models works
  if(mShipScene->getMaterialCount()>0) {
    for(unsigned int mat=0;mat<mShipScene->getMaterialCount();mat++) {
      if (mShipScene->getMaterial(mat).AmbientColor.getAlpha() != 255 || 
	  mShipScene->getMaterial(mat).DiffuseColor.getAlpha() != 255) {
	// Only allow rendering with transparency if required to avoid Z order problems
	mShipScene->getMaterial(mat).MaterialType = irr::video::EMT_TRANSPARENT_VERTEX_ALPHA;
      }
      mShipScene->getMaterial(mat).ColorMaterial = irr::video::ECM_DIFFUSE_AND_AMBIENT;
    }
  }

  //get light locations:
  unsigned int numberOfLights = rootJson["lights"]["numberOfLights"].asInt();
  if (numberOfLights>0)
    {
      for(unsigned int currentLight=0; currentLight<numberOfLights; currentLight++)
	{
	  irr::f32 lightX = rootJson["lights"]["list"][currentLight]["x"].asFloat();
	  irr::f32 lightY = rootJson["lights"]["list"][currentLight]["y"].asFloat();
	  irr::f32 lightZ = rootJson["lights"]["list"][currentLight]["z"].asFloat();

	  unsigned int lightR = rootJson["lights"]["list"][currentLight]["red"].asInt();
	  unsigned int lightG = rootJson["lights"]["list"][currentLight]["green"].asInt();
	  unsigned int lightB = rootJson["lights"]["list"][currentLight]["blue"].asInt();

	  irr::f32 lightStartAngle = rootJson["lights"]["list"][currentLight]["startAngle"].asFloat();
	  irr::f32 lightEndAngle = rootJson["lights"]["list"][currentLight]["endAngle"].asFloat();
	  irr::f32 lightRange = rootJson["lights"]["list"][currentLight]["range"].asFloat();
	  lightRange = lightRange * M_IN_NM; //Convert to metres

	  std::string lightSequence = "";//IniFile::iniFileToString(iniFilename, IniFile::enumerate1("Sequence", currentLight));
	  unsigned int phaseStart = 0;//IniFile::iniFileTou32(iniFilename, IniFile::enumerate1("PhaseStart", currentLight));

	  //add this Nav light into array
	  mNavLights.push_back(new NavLight (mShipScene,smgr,irr::core::vector3df(lightX,lightY,lightZ),irr::video::SColor(255,lightR,lightG,lightB),lightStartAngle,lightEndAngle,lightRange,lightSequence,phaseStart));
	}
    }
}

OtherShip::~OtherShip()
{
  //Drop mNavLights
  for(std::vector<NavLight*>::iterator it = mNavLights.begin(); it != mNavLights.end(); ++it) {
    delete (*it);
  }
  mNavLights.clear();
}

void OtherShip::update(irr::f32 deltaTime, irr::f32 scenarioTime, irr::f32 tideHeight, unsigned int lightLevel)
{
  if (mShipScene == nullptr) return;

  //move according to leg information
  if (mLegs.empty()) {
    //Don't change speed and hdg - may be in secondary mode, where these are set externally
    //Except, use rateOfTurn to update hdg
    mEta[2] += deltaTime * mRateOfTurn; // rateOfTurn in deg/s
  } else {
    //Work out which leg we're on
    std::vector<Leg>::size_type currentLeg = findCurrentLeg(scenarioTime);

    mMu[0] = mLegs[currentLeg].speed*KTS_TO_MPS;
    mEta[2] = mLegs[currentLeg].bearing*irr::core::DEGTORAD;
  }

    
  mEta[1] = mEta[1] + sin(mEta[2])*mMu[0]*deltaTime;
  mEta[0] = mEta[0] + cos(mEta[2])*mMu[0]*deltaTime;
  double yPos = tideHeight+mHeightCorrection;


  /*std::cout << "::::::Pos & Rot::::::" << std::endl;
    std::cout << "mMu[0] : Speed on Z : " << mMu[0] << std::endl;
    std::cout << "mMu[1] :  Speed on X (m/s) : " << mMu[1] << std::endl;
    std::cout << "mMu[2] :  Rate of turn (rad/s) : " << mMu[2] << std::endl;
    std::cout << "mEta[0] : Z position : " << mEta[0] << std::endl;
    std::cout << "mEta[1] : X position : " << mEta[1] << std::endl;
    std::cout << "mEta[2] : Heading (rad) : " << mEta[2] << std::endl;*/

  //Set position & speed by calling ship methods
  //setPosition(irr::core::vector3df(xPos,yPos,zPos));
  mShipScene->setPosition(irr::core::vector3df(mEta[1],yPos,mEta[0]));
  // DEE_DEC22 vvvv allows modelling of trim , list and models derived from other coordinate systems
  //ship->setRotation(irr::core::vector3df(angleCorrectionPitch, hdg+angleCorrection, angleCorrectionRoll)); //Global vectors
  mShipScene->setRotation(Angles::irrAnglesFromYawPitchRoll((mEta[2]+mAngleCorrection)*180/PI, mAngleCorrectionPitch,  mAngleCorrectionRoll)); //Global vectors
  // DEE_DEC22 ^^^^

  //for each light, find range and angle
  for(std::vector<NavLight*>::size_type currentLight = 0; currentLight<mNavLights.size(); currentLight++) {
    mNavLights[currentLight]->update(scenarioTime, lightLevel);
  }

}

irr::f32 OtherShip::getHeight() const
{
  return mHeight;
}

irr::f32 OtherShip::getRCS() const
{
  return mRcs;
}

std::string OtherShip::getName() const
{
  return mName;
}

std::vector<Leg> OtherShip::getLegs() const
{
  return mLegs;
}

void OtherShip::changeLeg(int legNumber, irr::f32 bearing, irr::f32 speed, irr::f32 distance, irr::f32 scenarioTime)
{

  //Check if leg exists, then if we are allowed to change this leg (current or future leg), and not the final 'stop' leg (hence legs.size()-1)
  if (legNumber >=0 && legNumber < ((int)mLegs.size() - 1) && legNumber >= (int)findCurrentLeg(scenarioTime)) {

    //Store old information temporarily
    irr::f32 oldSpeed = mLegs.at(legNumber).speed;

    //Recalculate subsequent start times, only changing from the current point.
    //We can guarantee that there is a next leg, as we checked (legNumber < legs.size() - 1)

    irr::f32 newTimeRemaining = 0;
    if ( legNumber == (int)findCurrentLeg(scenarioTime) ) {
      //On current leg - calculate from current point only
      irr::f32 oldTimeRemaining = mLegs.at(legNumber+1).startTime - scenarioTime;
      if (distance < 0) {distance = fabs(oldSpeed)*oldTimeRemaining/SECONDS_IN_HOUR;} //If leg length is negative, ensure overall leg length doesn't change

      if(speed != 0)
	newTimeRemaining = SECONDS_IN_HOUR * distance / fabs(speed); //The adjusted leg distance starts from now

      mLegs.at(legNumber).startTime = scenarioTime; // New leg effectively starts now
    } else {
      //On subsequent leg - calculate for whole leg
      irr::f32 oldTimeRemaining = mLegs.at(legNumber+1).startTime - mLegs.at(legNumber).startTime;
      if (distance < 0) {distance = fabs(oldSpeed)*oldTimeRemaining/SECONDS_IN_HOUR;} //If leg length is negative, ensure overall leg length doesn't change
      if(speed != 0)
	newTimeRemaining = SECONDS_IN_HOUR * distance / fabs(speed);
      //No need to change start time.
    }

    //Change this leg
    mLegs.at(legNumber).bearing = bearing;
    mLegs.at(legNumber).speed = speed;
    mLegs.at(legNumber).distance = distance; //Store for later reference

    //Set start time of the next leg (guaranteed to exist)
    mLegs.at(legNumber + 1).startTime = mLegs.at(legNumber).startTime + newTimeRemaining;
    //For the remaining legs (which may not exist)
    for (int i = legNumber + 2; i < (int)mLegs.size(); i++) {
      if(mLegs.at(i-1).speed != 0)
        mLegs.at(i).startTime = mLegs.at(i-1).startTime + SECONDS_IN_HOUR*mLegs.at(i-1).distance/mLegs.at(i-1).speed;
    }

  } //Check leg exists & can be changed

}

void OtherShip::addLeg(int afterLegNumber, irr::f32 bearing, irr::f32 speed, irr::f32 distance, irr::f32 scenarioTime)
{
  
  //Check if leg is reasonable, and is before the 'stop leg'
  //A special case allows afterLegNumber to equal -1, for when only a single 'stop leg' exists
  if (afterLegNumber >= -1 && afterLegNumber < ((int)mLegs.size() - 1)) {

    //if we're on the stop leg
    if (findCurrentLeg(scenarioTime) == (mLegs.size()-1)) {

      //If the 'after' leg is the penultimate, add a leg before the stop one, starting now
      if (afterLegNumber == ((int)mLegs.size()-2))  { //This also catches the special case where there is only the 'stop' leg, so the 'afterLegNumber value is -1

	Leg newLeg;
	newLeg.bearing = bearing;
	newLeg.speed = speed;
	newLeg.distance = distance;
	newLeg.startTime = scenarioTime;

	mLegs.insert(mLegs.end()-1, newLeg); //Insert before final leg
      }
      //else check that the 'after' leg is current or future
    } else if (afterLegNumber >=0 && afterLegNumber >= (int)findCurrentLeg(scenarioTime)) { //First check only required in case findCurrentLeg does not return a valid result (>=0)
      Leg newLeg;
      newLeg.bearing = bearing;
      newLeg.speed = speed;
      newLeg.distance = distance;
      newLeg.startTime = mLegs.at(afterLegNumber + 1).startTime; //This leg starts when the next leg would have started

      mLegs.insert(mLegs.begin()+afterLegNumber+1, newLeg); //Insert leg
    }

    //set start time of subsequent mLegs
    //For the remaining mLegs (which may not exist)
    for (int i = afterLegNumber + 2; i < (int)mLegs.size(); i++) {
      if(mLegs.at(i-1).speed != 0)
        mLegs.at(i).startTime = mLegs.at(i-1).startTime + SECONDS_IN_HOUR*mLegs.at(i-1).distance/mLegs.at(i-1).speed;
    }


  } //Check leg exists & can be changed

}

void OtherShip::deleteLeg(int legNumber, irr::f32 scenarioTime)
{

  //Check if leg exists, then if we are allowed to change this leg (current or future leg), and not the final 'stop' leg (hence mLegs.size()-1)
  if (legNumber >=0 && legNumber < ((int)mLegs.size() - 1) && legNumber >= (int)findCurrentLeg(scenarioTime)) {

    //We can guarantee that there is a next leg, as we checked (legNumber < mLegs.size() - 1)

    //Current or future leg?
    if (legNumber == (int)findCurrentLeg(scenarioTime)) {
      //Current leg
      //Set next leg start time to now: Set start time of the next leg (guaranteed to exist)
      mLegs.at(legNumber + 1).startTime = scenarioTime;

    } else {
      //Future leg
      //Set next leg start time to the start time of the leg we're removing
      mLegs.at(legNumber + 1).startTime = mLegs.at(legNumber).startTime;
    }

    //adjust start time of subsequent mLegs
    //For the remaining mLegs (which may not exist)
    for (int i = legNumber + 2; i < (int)mLegs.size(); i++) {
      if(mLegs.at(i-1).speed != 0)
        mLegs.at(i).startTime = mLegs.at(i-1).startTime + SECONDS_IN_HOUR*mLegs.at(i-1).distance/mLegs.at(i-1).speed;
    }

    //Remove this leg
    mLegs.erase(mLegs.begin() + legNumber);

  } //Check leg exists & can be changed

}

void OtherShip::resetLegs(irr::f32 course, irr::f32 speedKts, irr::f32 distanceNm, irr::f32 scenarioTime)
{
  mLegs.clear();

  Leg currentLeg;
  currentLeg.bearing = course;
  currentLeg.speed = speedKts;
  currentLeg.startTime = scenarioTime;
  currentLeg.distance = distanceNm;

  //Use distance to calculate startTime of next leg, and stored for later reference.
  currentLeg.distance = distanceNm;
  irr::f32 mainLegEndTime = 0;

  if(speedKts != 0)
    mainLegEndTime = scenarioTime + SECONDS_IN_HOUR*(distanceNm/fabs(speedKts)); // nm/kts -> hours, so convert to seconds

  mLegs.push_back(currentLeg);

  //Add a stop leg here
  Leg stopLeg;
  stopLeg.bearing=course;
  stopLeg.speed=0;
  stopLeg.distance=0;
  stopLeg.startTime = mainLegEndTime;
  mLegs.push_back(stopLeg);
}

void OtherShip::setRateOfTurn(irr::f32 aRateOfTurn) //Sets the rate of turn (only used in multiplayer mode)
{
  mRateOfTurn = aRateOfTurn;
}

RadarData OtherShip::getRadarData(irr::core::vector3df scannerPosition) const
//Get data for OtherShip (number) relative to scannerPosition
//Similar code in Buoy.cpp
{
  RadarData radarData;

  irr::core::vector3df contactPosition = getPosition();
  irr::core::vector3df relativePosition = contactPosition-scannerPosition;

  radarData.relX = relativePosition.X;
  radarData.relZ = relativePosition.Z;
  radarData.angle = relativePosition.getHorizontalAngle().Y;
  radarData.range = relativePosition.getLength();
  radarData.heading = getHeading();

  radarData.height=getHeight();
  radarData.solidHeight=mSolidHeight;
  //radarData.radarHorizon=99999; //ToDo: Implement when ARPA is implemented
  radarData.length=getLength();
  radarData.width=getBreadth();
  radarData.rcs=getRCS();

  //Calculate angles and ranges to each end of the contact
  irr::f32 relAngle1 = Angles::normaliseAngle(irr::core::RADTODEG*std::atan2( radarData.relX + 0.5*radarData.length*std::sin(irr::core::DEGTORAD*radarData.heading), radarData.relZ + 0.5*radarData.length*std::cos(irr::core::DEGTORAD*radarData.heading) ));
  irr::f32 relAngle2 = Angles::normaliseAngle(irr::core::RADTODEG*std::atan2( radarData.relX - 0.5*radarData.length*std::sin(irr::core::DEGTORAD*radarData.heading), radarData.relZ - 0.5*radarData.length*std::cos(irr::core::DEGTORAD*radarData.heading) ));
  irr::f32 range1 = std::sqrt(std::pow(radarData.relX + 0.5*radarData.length*std::sin(irr::core::DEGTORAD*radarData.heading),2) + std::pow(radarData.relZ + 0.5*radarData.length*std::cos(irr::core::DEGTORAD*radarData.heading),2));
  irr::f32 range2 = std::sqrt(std::pow(radarData.relX - 0.5*radarData.length*std::sin(irr::core::DEGTORAD*radarData.heading),2) + std::pow(radarData.relZ - 0.5*radarData.length*std::cos(irr::core::DEGTORAD*radarData.heading),2));
  radarData.minRange=std::min(range1,range2);
  radarData.maxRange=std::max(range1,range2);
  radarData.minAngle=std::min(relAngle1,relAngle2);
  radarData.maxAngle=std::max(relAngle1,relAngle2);

  //Initial defaults: Fixme: Will need changing with full implementation
  radarData.hidden=false;
  radarData.racon=""; //Racon code if set
  radarData.raconOffsetTime=0.0;
  radarData.SART=false;

  radarData.contact = (void*)this;

  return radarData;
}

std::vector<Leg>::size_type OtherShip::findCurrentLeg(irr::f32 scenarioTime)
{
  std::vector<Leg>::size_type currentLeg;

  for(currentLeg = 0; currentLeg<mLegs.size()-1; currentLeg++) {
    if (mLegs[currentLeg].startTime <=scenarioTime && mLegs[currentLeg+1].startTime > scenarioTime ) {
      break;
    }
  }
  //currentLeg is now the correct leg, or the last leg, which is a 'stopped' leg. (true as we run currentLeg++ once after the check (currentLeg<mLegs.size()-1) if the 'break' isn't reached

  return currentLeg;
}

void OtherShip::enableTriangleSelector(bool aSelectorEnabled)
{
  if (mShipScene == nullptr) return;

  //Only re-set if we need to change the state

  if (aSelectorEnabled && !mTriangleSelectorEnabled) {
    mShipScene->setTriangleSelector(mSelector);
    mTriangleSelectorEnabled = true;
  }

  if (!aSelectorEnabled && mTriangleSelectorEnabled) {
    mShipScene->setTriangleSelector(0);
    mTriangleSelectorEnabled = false;
  }

}
