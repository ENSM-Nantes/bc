#include "HeadingHolding.hpp"
#include "OwnShip.hpp"
#include "Angles.hpp"


static const float KP = 0.18;
static const float KD = 2;

HeadingHolding::HeadingHolding()
{
  mOwnShip = nullptr;
  mEnabled = false;
  mSetHeading = 0;
  mSlopeStarted = 0;
}

HeadingHolding::~HeadingHolding()
{

}

void HeadingHolding::load(OwnShip *aOwnShip)
{
  mOwnShip = aOwnShip;
}

void HeadingHolding::update(sTime& aTime)
{
  if(!mEnabled || nullptr == mOwnShip) return;

  float currentHeading = Angles::normaliseAngle(mOwnShip->getHeading()*irr::core::RADTODEG);
  float headingDiff = 0;
  float slope = 0, instruction = 0, slopeTime = 0, maxSlopeTime = 0;

  if(headingDiff > 180.0)
      headingDiff -= 360.0;
  
  if(headingDiff <= -180.0)
      headingDiff += 360.0;

  
  headingDiff = mSetHeading - currentHeading;
      
  //slope = headingDiff/maxSlopeTime; 

  //instruction = currentHeading + slope * slopeTime; 

  // mOwnShip->setWheel(instruction);

  /*std::cout << "Slope time : " << slopeTime << std::endl;
  std::cout << "mSlopeStarted : " << mSlopeStarted << std::endl;
  std::cout << "maxSlopeTime : " << maxSlopeTime << std::endl;
  std::cout << "instruction : " << instruction << std::endl;*/

  
  mOwnShip->setWheel((KP *  headingDiff) - ((KD * mOwnShip->getRateOfTurn())*180/M_PI));
}

void HeadingHolding::setCommand(bool aEnable, float aHeading)
{
  mEnabled = aEnable;
  mSetHeading = Angles::normaliseAngle(aHeading);
}

bool HeadingHolding::getEnabled(void) const {return mEnabled;}
float HeadingHolding::getSetHeading(void) const {return mSetHeading;}
