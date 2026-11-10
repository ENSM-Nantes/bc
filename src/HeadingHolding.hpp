#ifndef __HEADINGHOLDING_HPP_INCLUDED__
#define __HEADINGHOLDING_HPP_INCLUDED__

#include "Time.hpp"

class OwnShip;


class HeadingHolding
{
public:
  HeadingHolding();
  virtual ~HeadingHolding();

  void load(OwnShip *aOwnShip);
  void update(sTime& aTime);

  void setCommand(bool aEnable, float aHeading);
  bool getEnabled(void) const;
  float getSetHeading(void) const;

private:
  OwnShip *mOwnShip;
  bool mEnabled;
  float mSetHeading;
  float mSlopeStarted;
};

#endif
