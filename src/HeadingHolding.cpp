#include "HeadingHolding.hpp"
#include "OwnShip.hpp"
#include "Angles.hpp"

static const float HEADING_GAIN = 1.0;

HeadingHolding::HeadingHolding()
{
  mOwnShip = nullptr;
  mEnabled = false;
  mSetHeading = 0;
}

HeadingHolding::~HeadingHolding()
{

}

void HeadingHolding::load(OwnShip *aOwnShip)
{
  mOwnShip = aOwnShip;
}

void HeadingHolding::update(void)
{
  if(!mEnabled || nullptr == mOwnShip) return;

  float currentHeading = Angles::normaliseAngle(mOwnShip->getHeading()*irr::core::RADTODEG);
  float headingError = mSetHeading - currentHeading;

  if(headingError > 180.0)
    {
      headingError -= 360.0;
    }
  if(headingError <= -180.0)
    {
      headingError += 360.0;
    }

  mOwnShip->setWheel(HEADING_GAIN * headingError);
}

void HeadingHolding::setCommand(bool aEnable, float aHeading)
{
  mEnabled = aEnable;
  mSetHeading = Angles::normaliseAngle(aHeading);
}

bool HeadingHolding::getEnabled(void) const {return mEnabled;}
float HeadingHolding::getSetHeading(void) const {return mSetHeading;}
