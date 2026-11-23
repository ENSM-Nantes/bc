#include "HeadingHolding.hpp"
#include "OwnShip.hpp"
#include "Angles.hpp"
#include <cmath>


static const float KP = 0.18;
static const float KD = 2;
static const float RAMP_SPEED = 0.1666; //deg/s

static float wrap180(float aAngle)
{
  float a = Angles::normaliseAngle(aAngle);
  if(a > 180.0f)
    a -= 360.0f;
  return a;
}

HeadingHolding::HeadingHolding()
{
  mOwnShip = nullptr;
  mEnabled = false;
  mHeadingGoal = 0;
  mHdgGoalRamp = 0;
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

  float rampToGoal = wrap180(mHeadingGoal - mHdgGoalRamp);
  float step = RAMP_SPEED * aTime.deltaTime;
  float rampRate = 0;

  if(std::fabs(rampToGoal) <= step)
    mHdgGoalRamp = mHeadingGoal;
  else
  {
    rampRate = (rampToGoal > 0) ? RAMP_SPEED : -RAMP_SPEED;
    mHdgGoalRamp = Angles::normaliseAngle(mHdgGoalRamp + rampRate * aTime.deltaTime);
  }

  float diffGoal = wrap180(mHdgGoalRamp - currentHeading);
  float rotDeg = mOwnShip->getRateOfTurn() * irr::core::RADTODEG;

  std::cout << "diffGoal : " << diffGoal << std::endl;
  std::cout << "Ramp to Goal : " << rampToGoal << std::endl;
  std::cout << "step : " << step << std::endl;
  std::cout << "rampRate : " << rampRate << std::endl;
  std::cout << "mHdgGoalRamp : " << mHdgGoalRamp << std::endl;
  
  mOwnShip->setWheel(KP * diffGoal - (KD * rotDeg));
}

void HeadingHolding::setCommand(bool aEnable, float aHeading)
{
  if(aEnable && !mEnabled && nullptr != mOwnShip)
    mHdgGoalRamp = Angles::normaliseAngle(mOwnShip->getHeading()*irr::core::RADTODEG);
  mEnabled = aEnable;
  mHeadingGoal = Angles::normaliseAngle(aHeading);
}

bool HeadingHolding::getEnabled(void) const {return mEnabled;}
float HeadingHolding::getSetHeading(void) const {return mHeadingGoal;}
