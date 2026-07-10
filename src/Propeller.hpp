#ifndef PROPELLER_HPP
#define PROPELLER_HPP

#include <Eigen/Dense>
#include <vector>
#include "ShipGlobalParams.hpp"

class Propeller
{
public:

  Propeller();

  /*Process*/
  void Init(double aDiam, double aTfactor, double aXp, double aW0fraction, double aK0, double aK1, double aK2, std::string aRotDir, double aBackwardsEff, bool aIsForThruster=false, double aNrpsMax=0);
  void ComputeT(const Eigen::Vector3d& aMu, double aRho, const sGeoParams& aGeo);
  void SetRevs(const double aNrps, const double aDt);
  int ChangeRotDir(std::string aDir);

  /*Getter*/
  Eigen::Vector3d getT(void) const ;
  double getDeductionFactor(void) const ;
  double getDiameter(void) const ;
  double getWakeFraction(void) const ;
  double getLongPos(void) const ;
  double getRevs(void) const ;
  double getRevsSigned(void) const ; //Signed revs/sec: +ve ahead order direction, -ve astern - unlike getRevs(), reflects SetRevs' spool-up/down lag directly
  double getPolynomialCoef(unsigned char aCoefNumber) const ;
  std::string getForwardRotDir(void) const ;
  std::string getCurrentRotDir(void) const ;
  void PrintParams(void);
  
private:

  double mDiam; //Propeller diameter
  double mTfactor; //Thrust deduction factor
  double mXp; //longitudinal position of the propeller
  double mW0fraction; //Nominal wake fraction
  double mK0; //polynomial coefficients 
  double mK1; //Kt(J) = k0 + k1*J + k2*J²
  double mK2;
  double mBackwardsEff; //Backwards efficiency
  std::string mForwardRotDir; //Forward Rotation Direction
  
  double mNrps; //Propeller revolutions per second (magnitude)
  double mNrpsSigned; //Propeller revolutions per second (+ve ahead order direction, -ve astern) - the actual, rate-limited state SetRevs steps towards its order
  double mNrpsMax; //Max rate of change of revs/sec, per second - spool up/down rate limit. <=0 means respond instantly (used by thrusters)
  Eigen::Vector3d mT; //Propeller Force generated
  std::string mCurrentRotDir; //Current Rotation Direction

  bool mIsForThruster;
};

#endif
