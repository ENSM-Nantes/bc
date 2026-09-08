#ifndef THRUSTER_HPP
#define THRUSTER_HPP

#include <string>
#include <iostream>
#include "Propeller.hpp"

class Thruster
{

public:

  Thruster();
  ~Thruster();

  void Init(bool aHasBowThruster, bool aHasSternThruster, std::string aBrand, std::string aType, unsigned int aPower, float aRpmMax, float aFuelCons, float aPropDiameter, float aThrustFactor, float aNominalWake, float aK0, float aK1, float aK2, std::string aForwardRotDir, float aXBow, float aXStern);
  void PrintParams(void);
  void ComputeT(const Eigen::Vector3d& aMu, const double aRho, const sGeoParams& aGeo);

  Propeller& GetBowPropeller(void);
  Propeller& GetSternPropeller(void);

  bool HasThruster(void);
  bool HasBowThruster(void);
  bool HasSternThruster(void);

  Eigen::Vector3d GetT(void);
  float getRpmMax(void) const;

private:

  bool mHasBowThruster;
  bool mHasSternThruster;

  float mBowThrusterPortRpm;
  float mBowThrusterStbdRpm;
  float mSternThrusterPortRpm;
  float mSternThrusterStbdRpm;

  Propeller mBowProp;
  Propeller mSternProp;

  float mXBow;
  float mXStern;
  float mThrustFactor;
  float mNominalWake;
  float mK0;
  float mK1;
  float mK2;
  
  std::string mForwardRotDir;
  std::string mBrand;
  std::string mType;
  unsigned int mPower; //kW
  float mRpmMax; //rpm
  float mFuelConsumption; //g/kWh 100%
  float mPropDiam; //m
};


#endif
