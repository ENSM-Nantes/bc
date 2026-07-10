#include "Thruster.hpp"

Thruster::Thruster(void)
{

}


Thruster::~Thruster(void)
{

}

void Thruster::Init(bool aHasBowThruster, bool aHasSternThruster, std::string aBrand, std::string aType, unsigned int aPower, float aRpmMax, float aFuelCons, float aPropDiameter, float aThrustFactor, float aNominalWake, float aK0, float aK1, float aK2, std::string aForwardRotDir, float aBackEff, float aXBow, float aXStern)
{
  mHasBowThruster = aHasBowThruster;
  mHasSternThruster = aHasSternThruster;
  mBrand = aBrand;
  mType = aType;
  mPower = aPower;
  mRpmMax = aRpmMax;
  mFuelConsumption = aFuelCons;
  mPropDiam = aPropDiameter;
  mThrustFactor = aThrustFactor;
  mNominalWake = aNominalWake;
  mK0 = aK0;
  mK1 = aK1;
  mK2 = aK2;
  mForwardRotDir = aForwardRotDir;
  mBackEff = aBackEff;
  mXBow = aXBow;
  mXStern = aXStern;

  
  if(mHasBowThruster)
    {
      mBowProp.Init(mPropDiam, mThrustFactor, mXBow, mNominalWake, mK0, mK1, mK2, mForwardRotDir, mBackEff, true);
    }

  if(mHasSternThruster)
    {
      mSternProp.Init(mPropDiam, mThrustFactor, mXStern, mNominalWake, mK0, mK1, mK2, mForwardRotDir, mBackEff, true);
    }
  
}

void Thruster::PrintParams(void)
{
  std::cout << "::::::Thruster Parameters::::::" << std::endl;
  std::cout << "Bow Thruster : " << mHasBowThruster << std::endl;
  std::cout << "Stern Thruster : " << mHasSternThruster << std::endl; 
  std::cout << "Brand : " << mBrand << std::endl;
  std::cout << "Type : " << mType << std::endl;
  std::cout << "Power (kW): " << mPower << std::endl;
  std::cout << "Rpm max (rpm): " << mRpmMax << std::endl;
  std::cout << "Fuel Consumption (g/kWh 100%) : " << mFuelConsumption << std::endl;
  std::cout << "Propeller thruster diameter (m) : " << mPropDiam << std::endl;
  std::cout << "Forward rotation direction : " << mForwardRotDir << std::endl;
  std::cout << "Backward efficiency : " << mBackEff << std::endl;
  std::cout << "Non dimensional longitudinal position from midship (Bow) : " << mXBow << std::endl;
  std::cout << "Non dimensional longitudinal position from midship (Stern : " << mXStern << std::endl;

  std::cout << "::::::::::::" << std::endl;
}

bool Thruster::HasThruster(void)
{
  if(mHasBowThruster || mHasSternThruster)
    return true;
  else
    return false;
}

bool Thruster::HasBowThruster(void)
{
  if(mHasBowThruster)
    return true;
  else
    return false;
}

bool Thruster::HasSternThruster(void)
{
  if(mHasSternThruster)
    return true;
  else
    return false;
}

void Thruster::ComputeT(const Eigen::Vector3d& aMu, const double aRho, const sGeoParams& aGeo)
{
  if(mHasBowThruster)
    {
      mBowProp.ComputeT(aMu, aRho, aGeo);
    }

  if(mHasSternThruster)
    {
      mSternProp.ComputeT(aMu, aRho, aGeo);
    }
}

Eigen::Vector3d Thruster::GetT(void)
{
  Eigen::Vector3d thrusterT;

  thrusterT << 0, 0, 0;
  
  if(mHasBowThruster)
    {
      thrusterT = mBowProp.getT();
    }

  if(mHasSternThruster)
    {
      thrusterT += mSternProp.getT();
    }

  return thrusterT;
}

Propeller& Thruster::GetBowPropeller(void)
{
  return mBowProp;
}


Propeller& Thruster::GetSternPropeller(void)
{
  return mSternProp;
}


float Thruster::getRpmMax(void) const
{
  return mRpmMax;
}
