#include "Propeller.hpp"
#include <cmath>

Propeller::Propeller(void)
{
  Init(0,0,0,0,0,0,0,"right",0,false,0);
  mT << 0, 0, 0;
  mNrps = 0;
  mNrpsSigned = 0;
  mCurrentRotDir = "stop";
}

void Propeller::Init(double aDiam, double aTfactor, double aXp, double aW0fraction, double aK0, double aK1, double aK2, std::string aForwardRotDir, double aBackwardsEff, bool aIsForThruster, double aNrpsMax)
{
  mDiam = aDiam;
  mTfactor = aTfactor;
  mXp = aXp;
  mW0fraction = aW0fraction;
  mK0 = aK0;
  mK1 = aK1;
  mK2 = aK2;
  mForwardRotDir = aForwardRotDir;
  mBackwardsEff = aBackwardsEff;
  mIsForThruster = aIsForThruster;
  mNrpsMax = aNrpsMax;
}

void Propeller::PrintParams(void)
{
  std::cout << "::::::Propeller Parameters::::::" << std::endl;
  std::cout << "Propeller diameter : " << mDiam << std::endl;
  std::cout << "Thrust deduction factor : " << mTfactor << std::endl;
  std::cout << "Longitudinal position of the propeller : " << mXp << std::endl;
  std::cout << "Nominal wake fraction : " << mW0fraction << std::endl;
  std::cout << "Polynomial coefficients : " << mK0 << " ; " << mK1 << " ; " << mK2 << std::endl; 
  std::cout << "Forward Rotation Direction : " << mForwardRotDir << std::endl;
  std::cout << "Backwards efficiency : " << mBackwardsEff << std::endl;
  std::cout << "::::::::::::" << std::endl;
}

void Propeller::SetRevs(const double aNrpsOrder, const double aDt)
{
  //Step the actual (signed) revs towards the order, rate limited by mNrpsMax - same pattern as
  //Rudder::SetDelta. mNrpsMax<=0 means no limit configured (thrusters, unconfigured props): respond instantly.
  if(mNrpsMax > 0 && 0 != aDt)
    {
      double rrSet = (aNrpsOrder - mNrpsSigned) / aDt;
      if(std::abs(rrSet) > mNrpsMax)
	mNrpsSigned += (rrSet/std::abs(rrSet)) * mNrpsMax * aDt;
      else
	mNrpsSigned = aNrpsOrder;
    }
  else
    {
      mNrpsSigned = aNrpsOrder;
    }

  if(mNrpsSigned < 0)
    {
      if(0 == ChangeRotDir("backwards"))
	mNrps = std::abs(mNrpsSigned);
      else
	mNrps = 0;
    }
  else if(mNrpsSigned > 0)
    {
      if(0 == ChangeRotDir("forward"))
	mNrps = mNrpsSigned;
      else
	mNrps = 0;
    }
  else
    {
      ChangeRotDir("stop");
      mNrps = 0;
    }
}

int Propeller::ChangeRotDir(std::string aDir)
{
  int err = 0;

      if("forward" == aDir)
	mCurrentRotDir = mForwardRotDir;
      else if("backwards" == aDir)
	{
	  if("right" == mForwardRotDir)
	    mCurrentRotDir = "left";
	  else if("left" == mForwardRotDir)
	    mCurrentRotDir = "right";
	  else
	    mCurrentRotDir = "stop";
	}
      else
	mCurrentRotDir = "stop";
      
  return err;
}

void Propeller::ComputeT(const Eigen::Vector3d& aMu, const double aRho, const sGeoParams& aGeo)
{
  double u = 0, beta = 0, rp = 0;
  double betap = 0, wp = 0, up = 0, jp = 0;
  double kt = 0, tp = 0, fp = 0;

  // ***** H. Yasukawa and Y. Yoshimura 2015 *******
  u = pow((pow(aMu[0], 2) + pow(aMu[1], 2)), 0.5);

  if(0 != aMu[0] && 0 != u)
    {
      beta = atan(-(aMu[1])/aMu[0]);
      rp = (aMu[2] * aGeo.lPP)/u;
    }
  else
    {
      rp = 0;
      beta = 0;
    }  
  
  betap = beta - (mXp * rp);  /*Equation (15)*/

  wp = mW0fraction * exp(-4 * pow(betap, 2));  /*Equation (12)*/

  //Inflow speed along the unit's own axis: surge for a shaft-aligned propeller,
  //sway for an athwartship thruster
  if(!mIsForThruster)
    up = aMu[0] * (1-wp);
  else
    up = aMu[1] * (1-wp);

  if(0 != mNrps)
    jp = up / (mNrps * mDiam);  /*Equation (11)*/
  else
    jp = 0;
  
  kt = mK0 + (mK1*jp) + (mK2*pow(jp, 2));  /*Equation (10)*/
  tp = aRho * pow(mNrps, 2) * pow(mDiam, 4) * kt;  /*Equation (9)*/
  fp = (1-mTfactor) * tp;  /*Equation (8)*/

  //Add a efficiency factor for backwards
  if(mForwardRotDir != mCurrentRotDir)
    fp = fp*(-mBackwardsEff);

  if(!mIsForThruster)
    mT << fp, 0, 0;
  else
    {
      //Yaw moment from the thruster's sway force acting at its longitudinal lever arm from midship,
      //same N = x*Y pattern as the rudder's normal force (see Rudder::ComputeT)
      double xp = mXp * aGeo.lPP;
      mT << 0, fp, xp*fp;
    }

  //std::cout << "***Propeller mT :" << mT << std::endl;
  //************
}

Eigen::Vector3d Propeller::getT(void) const {return mT;}
double Propeller::getDiameter(void) const {return mDiam;}
double Propeller::getDeductionFactor(void) const {return mTfactor;}
double Propeller::getWakeFraction(void) const {return mW0fraction;}
double Propeller::getLongPos(void) const {return mXp;}
double Propeller::getRevs(void) const {return mNrps;}
double Propeller::getRevsSigned(void) const {return mNrpsSigned;}
std::string Propeller::getForwardRotDir(void) const {return mForwardRotDir;}
std::string Propeller::getCurrentRotDir(void) const {return mCurrentRotDir;}

double Propeller::getPolynomialCoef(unsigned char aCoefNumber) const
{
  if(0 == aCoefNumber) return mK0;
  else if(1 == aCoefNumber) return mK1;
  else if(2 == aCoefNumber) return mK2;
  else return -1;
}
