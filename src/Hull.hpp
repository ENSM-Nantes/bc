#ifndef HULL_HPP
#define HULL_HPP

#include <Eigen/Dense>
#include <vector>
#include "ShipGlobalParams.hpp"

class Hull
{
public:
  
  Hull();

  /*Process*/
  void Init(double aXp0, double aXpVV, double aXpVR, double aXpRR, double aXpVVVV, double aYpV, double aYpR, double aYpVVV,
		double aYpVVR, double aYpVRR, double aYpRRR, double aNpV, double aNpR, double aNpVVV, double aNpVVR,
		double aNpVRR, double aNpRRR, double aKpG, double aKpB, double aKpR, double aKpBBG, double aKpBRG,
		double aKpRRG, double aKpBBB, double aKpBBR, double aKpBRR, double aKpRRR, bool aInvertRoll);
  void ComputeT(const Eigen::Vector3d& aMu, const double aRho, const sGeoParams& aGeo, double aRollAngle);

  /*Getter*/
  Eigen::Vector3d& getT(void);
  double getKh(void);
  void PrintParams(void);
  bool getInvertRoll(void);
  
private: 

  double mXp0;
  double mXpVV;
  double mXpVR;
  double mXpRR;
  double mXpVVVV;
  double mYpV;
  double mYpR;
  double mYpVVV;
  double mYpVVR;
  double mYpVRR;
  double mYpRRR;
  double mNpV;
  double mNpR;
  double mNpVVV;
  double mNpVVR;
  double mNpVRR;
  double mNpRRR;
  double mKpG;
  double mKpB;
  double mKpR;
  double mKpBBG;
  double mKpBRG;
  double mKpRRG;
  double mKpBBB;
  double mKpBBR;
  double mKpBRR;
  double mKpRRR;
  bool mInvertRoll;
  
  Eigen::Vector3d mT;
  double mKh;
};

#endif
